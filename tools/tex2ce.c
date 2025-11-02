// tex2ce.c — conversor .tex -> bytecode p/ CE (AppVar)
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

// ---- ASCII sanitize (mapa 7-bit) ----
static void emit_text_ascii(Vec *out, const char *beg, size_t n){
    if(!n) return;
    char *tmp = (char*)malloc(n);
    size_t m = 0;
    for(size_t i=0;i<n;i++){
        unsigned char ch = (unsigned char)beg[i];
        if (ch == '\t') { tmp[m++] = ' '; continue; }
        if (ch < 32 || ch > 126) { tmp[m++] = '?'; continue; } // 7-bit only
        tmp[m++] = (char)ch;
    }
    put_u8(out,0x01); put_u16(out,(u16)m); vec_put(out,tmp,m); free(tmp);
}

static void parse_group_into(Src *src, Vec *payload){
    if(!match(src,'{')) return;
    int depth=1; Src sub=*src; size_t start=sub.i;
    while(sub.i<sub.n && depth>0){
        char c=sub.s[sub.i++]; if(c=='{') depth++; else if(c=='}') depth--;
    }
    size_t end=sub.i-1;
    Src inner={.s=src->s+start,.i=0,.n=end-start};
    parse_block(&inner,payload);
    src->i=sub.i;
}
static void emit_text(Vec *out, const char *beg, size_t n){ emit_text_ascii(out,beg,n); }

static void parse_block(Src *src, Vec *out){
    const char *tstart = src->s + src->i; size_t tlen = 0;
    while (src->i < src->n) {
        int c = peek(src);

        // --- NOVO: tratar \r / \n do arquivo ---
        if (c == '\r' || c == '\n') {
            // flush texto pendente
            emit_text(out, tstart, tlen); tlen = 0;

            // consumir bloco de CR/LF; contamos só LFs
            int lf_count = 0;
            while (src->i < src->n) {
                char ch = src->s[src->i];
                if (ch == '\r') { src->i++; continue; }
                if (ch == '\n') { src->i++; lf_count++; continue; }
                break;
            }
            if (lf_count >= 2) {
                // parágrafo
                put_u8(out, 0x06); // PARBREAK
            } else if (lf_count == 1) {
                put_u8(out, 0x05); // NEWLINE
            }

            tstart = src->s + src->i;
            continue;
        }
        // --------------------------------------

        if (c == '\\') {                 // comandos (\frac, \\ e aliases)
            emit_text(out, tstart, tlen); tlen = 0; get(src);

            // barra + espaco -> apenas um espaco (evita "\" solto)
            if (peek(src) == ' ') {
                emit_text(out, " ", 1); src->i++;
                tstart = src->s + src->i;
                continue;
            }

            if (strncmp(src->s+src->i, "frac", 4) == 0) {
                src->i += 4;
                Vec num = {0}, den = {0};
                parse_group_into(src, &num);
                parse_group_into(src, &den);
                put_u8(out, 0x02);                       // FRAC
                put_u16(out, (u16)num.len); vec_put(out, num.buf, num.len);
                put_u16(out, (u16)den.len); vec_put(out, den.buf, den.len);
                free(num.buf); free(den.buf);
                tstart = src->s + src->i;

            } else if (match(src,'\\')) {
                put_u8(out, 0x05);                       // NEWLINE
                tstart = src->s + src->i;

            } else {
                // aliases: mapeia alguns \comandos para ASCII
                static const struct { const char *cmd, *subst; } alias[] = {
                    {"rho",        "rho"},
                    {"pi",         "pi"},
                    {"approx",     "~="},
                    {"simeq",      "~="},
                    {"Rightarrow", "=>"},
                    {"rightarrow", "->"},
                    {"to",         "->"},
                    {"varepsilon", "epsilon"},
                    {"verepsilon", "epsilon"}, // tolera o typo
                    {"Ohm",        "ohm"},     // opcional
                    {"left",       ""},        // \left( ... ) => apenas imprime '(' e ')'
                    {"right",      ""},        // idem
                    {"Big",        ""},        // tamanhos nao existem no viewer
                    {"big",        ""},        // idem
                    {"int",        "integral"},// \int -> "integral" (ASCII)
                    {"quad",       " "},       // espaco largo -> 1 espaco (pode usar "  ")
                    // extras uteis (opcionais):
                    {"cdot",       "*"},
                    {"times",      "*"},
                    {"leq",        "<="},
                    {"geq",        ">="},
                    {"neq",        "!="},
                    {"pm",         "+/-"},
                    {NULL, NULL}
                };
                size_t j = src->i;
                while (j < src->n && isalpha((unsigned char)src->s[j])) j++;
                size_t k = j - src->i;
                const char *name = src->s + src->i;
                const char *subst = NULL;
                for (int a=0; alias[a].cmd; ++a){
                    if (k == strlen(alias[a].cmd) && strncmp(name, alias[a].cmd, k)==0){
                        subst = alias[a].subst; break;
                    }
                }
                if (subst) {
                    emit_text(out, subst, strlen(subst));
                } else {
                    // fallback: imprime literal com a barra
                    emit_text(out, "\\", 1);
                    emit_text(out, name, k);
                }
                src->i = j;
                tstart = src->s + src->i;
            }

        } else if (c == '^' || c == '_') {              // sup/sub
            emit_text(out, tstart, tlen); tlen = 0; get(src);
            u8 tag = (c=='^') ? 0x03 : 0x04;
            Vec child = {0};
            if (match(src,'{')) { src->i--; parse_group_into(src, &child); }
            else { put_u8(&child,0x01); put_u16(&child,1); put_u8(&child,(u8)get(src)); }
            put_u8(out, tag); put_u16(out, (u16)child.len); vec_put(out, child.buf, child.len);
            free(child.buf);
            tstart = src->s + src->i;

        } else if (c == '}') {                          // fim de grupo
            break;

        } else {                                        // texto plano
            get(src); tlen++;
        }
    }
    emit_text(out, tstart, tlen);
}


int main(int argc, char **argv){
    if(argc<3){ fprintf(stderr,"uso: %s in.tex out.bin\n", argv[0]); return 1; }
    FILE *f=fopen(argv[1],"rb"); if(!f){perror("in");return 1;}
    fseek(f,0,SEEK_END); long n=ftell(f); rewind(f);
    char *buf=(char*)malloc(n+1); fread(buf,1,n,f); buf[n]=0; fclose(f);

    Src src={.s=buf,.i=0,.n=(size_t)n}; Vec out={0};
    parse_block(&src,&out); put_u8(&out,0xFF);

    FILE *g=fopen(argv[2],"wb"); if(!g){perror("out");return 1;}
    fwrite(out.buf,1,out.len,g); fclose(g);
    fprintf(stderr,"OK: %zu bytes\n", out.len);
    free(out.buf); free(buf);
    return 0;
}
