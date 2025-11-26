// tex2ce.c — conversor .tex -> bytecode p/ CE (AppVar)
// Subset: \frac{A}{B}, ^{X}, _{Y}, \\ (quebra)
#include <stdint.h>
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

// Mapeia um codepoint Unicode para o byte do charset TI-83+/84+/CE
static uint8_t ti_from_unicode(uint32_t cp) {
    // ASCII básico: passa direto
    if (cp < 0x80u) {
        return (uint8_t)cp;
    }

    // Acentos PT-BR que a TI tem
    switch (cp) {
        case 0x00C1: return 0x8A; // Á
        case 0x00C0: return 0x8B; // À
        case 0x00C2: return 0x8C; // Â
        case 0x00C4: return 0x8D; // Ä

        case 0x00E1: return 0x8E; // á
        case 0x00E0: return 0x8F; // à
        case 0x00E2: return 0x90; // â
        case 0x00E4: return 0x91; // ä

        case 0x00C9: return 0x92; // É
        case 0x00C8: return 0x93; // È
        case 0x00CA: return 0x94; // Ê
        case 0x00CB: return 0x95; // Ë

        case 0x00E9: return 0x96; // é
        case 0x00E8: return 0x97; // è
        case 0x00EA: return 0x98; // ê
        case 0x00EB: return 0x99; // ë

        case 0x00CD: return 0x9A; // Í
        case 0x00CC: return 0x9B; // Ì
        case 0x00CE: return 0x9C; // Î
        case 0x00CF: return 0x9D; // Ï

        case 0x00ED: return 0x9E; // í
        case 0x00EC: return 0x9F; // ì

        case 0x00D3: return 0xA2; // Ó
        case 0x00D2: return 0xA3; // Ò
        case 0x00D4: return 0xA4; // Ô
        case 0x00D6: return 0xA5; // Ö

        case 0x00F3: return 0xA6; // ó
        case 0x00F2: return 0xA7; // ò
        case 0x00F4: return 0xA8; // ô
        case 0x00F6: return 0xA9; // ö

        case 0x00DA: return 0xAA; // Ú
        case 0x00D9: return 0xAB; // Ù
        case 0x00DB: return 0xAC; // Û
        case 0x00DC: return 0xAD; // Ü

        case 0x00FA: return 0xAE; // ú
        case 0x00F9: return 0xAF; // ù

        case 0x00C7: return 0xB2; // Ç
        case 0x00E7: return 0xB3; // ç
    }

    // Letras gregas no charset TI
    switch (cp) {
        case 0x03B1: return 0xBB; // α
        case 0x03B2: return 0xBC; // β
        case 0x03B3: return 0xBD; // γ
        case 0x0394: return 0xBE; // Δ
        case 0x03B4: return 0xBF; // δ
        case 0x03B5: return 0xC0; // ε
        case 0x03B8: return 0x5B; // θ
        case 0x03BB: return 0xC2; // λ
        case 0x03BC: return 0xC3; // μ
        case 0x03C0: return 0xC4; // π
        case 0x03C1: return 0xC5; // ρ
        case 0x03A3: return 0xC6; // Σ
        case 0x03C3: return 0xC7; // σ
        case 0x03C4: return 0xC8; // τ
        case 0x03C6: return 0xC9; // φ
        case 0x03A9: return 0xCA; // Ω
    }

    // Coisas que a TI NÃO tem (ã, õ, etc.) – faz “downgrade”
    switch (cp) {
        case 0x00E3: return 'a'; // ã
        case 0x00C3: return 'A'; // Ã
        case 0x00F5: return 'o'; // õ
        case 0x00D5: return 'O'; // Õ
    }

    return 0; // sem mapping
}

static void emit_text_ascii(Vec *out, const char *beg, size_t n){
    if (!n) return;

    // buffer temporário: UTF-8 até 3 bytes → 1 char TI
    uint8_t *tmp = (uint8_t*)malloc(n * 2);
    size_t m = 0;

    const unsigned char *p   = (const unsigned char*)beg;
    const unsigned char *end = p + n;

    while (p < end) {
        uint32_t cp;
        unsigned char c = *p++;

        if (c < 0x80) {
            // ASCII 1 byte
            cp = c;
        } else if ((c & 0xE0) == 0xC0 && p < end) {
            // UTF-8 2 bytes
            cp = ((uint32_t)(c & 0x1F) << 6)
               | ((uint32_t)(*p++ & 0x3F));
        } else if ((c & 0xF0) == 0xE0 && p + 1 < end) {
            // UTF-8 3 bytes
            cp = ((uint32_t)(c & 0x0F) << 12)
               | ((uint32_t)(p[0] & 0x3F) << 6)
               | ((uint32_t)(p[1] & 0x3F));
            p += 2;
        } else {
            // lixo / sequência quebrada -> ?
            cp = '?';
        }

        uint8_t outch;

        // Tab vira espaço, pra não quebrar layout
        if (cp == '\t') {
            outch = ' ';
        } else if (cp < 0x20u && cp != '\n' && cp != '\r') {
            // controla bizarro -> espaço
            outch = ' ';
        } else if (cp < 0x80u) {
            outch = (uint8_t)cp;
        } else {
            uint8_t mapped = ti_from_unicode(cp);
            outch = mapped ? mapped : (uint8_t)'?';
        }

        tmp[m++] = outch;
    }

    put_u8(out, 0x01);
    put_u16(out, (u16)m);
    vec_put(out, tmp, m);
    free(tmp);
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
                    /* Letras gregas principais */
                    {"alpha",      "α"},
                    {"beta",       "β"},
                    {"gamma",      "γ"},
                    {"delta",      "δ"},
                    {"epsilon",    "ε"},
                    {"theta",      "θ"},
                    {"lambda",     "λ"},
                    {"mu",         "μ"},
                    {"pi",         "π"},
                    {"rho",        "ρ"},
                    {"sigma",      "σ"},
                    {"Sigma",      "Σ"},
                    {"tau",        "τ"},
                    {"phi",        "φ"},
                    {"Omega",      "Ω"},
                    {"varepsilon", "ε"},
                    {"verepsilon", "ε"},
                    {"approx",     "~="},
                    {"simeq",      "~="},
                    {"Rightarrow", "=>"},
                    {"rightarrow", "->"},
                    {"to",         "->"},
                    {"Ohm",        "Ω"},

                    {"left",       ""},   // \left( ... ) -> só imprime o parêntese mesmo
                    {"right",      ""},   // idem
                    {"Big",        ""},   // tamanhos ignorados
                    {"big",        ""},

                    {"int",        "integral"}, // TODO mapear ∫
                    {"quad",       " "},

                    // extras uteis
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
