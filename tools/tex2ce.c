// tex2ce.c — conversor .tex -> bytecode p/ CE
// Subset: \frac{A}{B}, ^{X}, _{Y}, \\ (quebra)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;

typedef struct { u8 *buf; size_t len, cap; } Vec;

static void vec_put(Vec *v, const void *src, size_t n){
    if(v->len + n > v->cap){
        v->cap = (v->cap ? v->cap*2 : 1024);
        while(v->len + n > v->cap) v->cap *= 2;
        v->buf = (u8*)realloc(v->buf, v->cap);
    }
    memcpy(v->buf + v->len, src, n); v->len += n;
}
static void put_u8(Vec *v, u8 x){ vec_put(v,&x,1); }
static void put_u16(Vec *v, u16 x){ u8 b[2]={x&0xFF,(x>>8)&0xFF}; vec_put(v,b,2); }

typedef struct { const char *s; size_t i, n; } Src;

static int peek(Src *src){ return (src->i < src->n) ? (unsigned char)src->s[src->i] : -1; }
static int get (Src *src){ return (src->i < src->n) ? (unsigned char)src->s[src->i++] : -1; }
static int match(Src *src, char c){ if(peek(src)==c){src->i++;return 1;} return 0; }

static void parse_block(Src *src, Vec *out); // fwd

// lê {...} e retorna payload codificado
static void parse_group_into(Src *src, Vec *payload){
    if(!match(src,'{')) return;
    // parse até o '}' (permitindo aninhamento simples)
    int depth=1;
    Src sub = *src;
    // coletamos o trecho para parse recursivo
    size_t start = sub.i;
    while(sub.i < sub.n && depth>0){
        char c = sub.s[sub.i++];
        if(c=='{') depth++;
        else if(c=='}') depth--;
    }
    size_t end = sub.i-1;
    // crie um sub-src apenas com o conteúdo interno
    Src inner = { .s=src->s+start, .i=0, .n=end-start };
    parse_block(&inner, payload);
    src->i = sub.i; // avança fonte original
}

static void emit_text(Vec *out, const char *beg, size_t n){
    if(n==0) return;
    put_u8(out, 0x01);            // TEXT
    put_u16(out, (u16)n);
    vec_put(out, beg, n);
}

static void parse_block(Src *src, Vec *out){
    const char *tstart = src->s + src->i; size_t tlen = 0;
    while(src->i < src->n){
        int c = peek(src);
        if(c=='\\'){ // comando
            // flush texto pendente
            emit_text(out, tstart, tlen); tlen=0; get(src);

            // comandos suportados: \frac, \\  (quebra)
            if(strncmp(src->s+src->i, "frac", 4)==0){
                src->i += 4;
                Vec num={0}, den={0};
                parse_group_into(src, &num);  // {numerador}
                parse_group_into(src, &den);  // {denominador}
                put_u8(out,0x02); // FRAC
                put_u16(out,(u16)num.len); vec_put(out,num.buf,num.len);
                put_u16(out,(u16)den.len); vec_put(out,den.buf,den.len);
                free(num.buf); free(den.buf);
                tstart = src->s + src->i;
            } else if(match(src,'\\')) {
                // quebra de linha
                put_u8(out,0x05); // NEWLINE
                tstart = src->s + src->i;
            } else {
                // desconhecido: consome palavra e joga como TEXT literal
                size_t j=src->i;
                while(j<src->n && isalpha((unsigned char)src->s[j])) j++;
                size_t k = j - src->i;
                emit_text(out, "\\", 1);
                emit_text(out, src->s+src->i, k);
                src->i = j;
                tstart = src->s + src->i;
            }
        } else if(c=='^' || c=='_'){
            // flush anterior
            emit_text(out, tstart, tlen); tlen=0; get(src);
            u8 tag = (c=='^')?0x03:0x04;
            Vec child={0};
            if(match(src,'{')){
                // retroceder 1, para usar parse_group_into
                src->i--; parse_group_into(src,&child);
            } else {
                // único char
                put_u8(&child,0x01); put_u16(&child,1); put_u8(&child,(u8)get(src));
            }
            put_u8(out, tag); put_u16(out,(u16)child.len); vec_put(out,child.buf,child.len);
            free(child.buf);
            tstart = src->s + src->i;
        } else if(c=='}'){ // fim do grupo
            break;
        } else {
            get(src); tlen++;
        }
    }
    emit_text(out, tstart, tlen);
}

int main(int argc, char **argv){
    if(argc<3){ fprintf(stderr,"uso: %s in.tex out.bin\n", argv[0]); return 1; }
    // lê arquivo
    FILE *f=fopen(argv[1],"rb"); if(!f){perror("in");return 1;}
    fseek(f,0,SEEK_END); long n=ftell(f); rewind(f);
    char *buf=(char*)malloc(n+1); fread(buf,1,n,f); buf[n]=0; fclose(f);

    // parse
    Src src={.s=buf,.i=0,.n=(size_t)n}; Vec out={0};
    parse_block(&src,&out); put_u8(&out,0xFF); // END

    // grava
    FILE *g=fopen(argv[2],"wb"); if(!g){perror("out");return 1;}
    fwrite(out.buf,1,out.len,g); fclose(g);
    fprintf(stderr,"OK: %zu bytes\n", out.len);
    free(out.buf); free(buf);
    return 0;
}
