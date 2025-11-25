// tex2ce.c — conversor .tex -> bytecode p/ CE (AppVar)
// Subset: \frac{A}{B}, ^{X}, _{Y}, \\ (quebra)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;

// ---- caracteres especiais compartilhados com main.c (>=0x80) ----
enum {
    GLYPH_ALPHA = 0x80,
    GLYPH_BETA,
    GLYPH_GAMMA,
    GLYPH_DELTA,
    GLYPH_THETA,
    GLYPH_LAMBDA,
    GLYPH_MU,
    GLYPH_PI,
    GLYPH_RHO,
    GLYPH_SIGMA,
    GLYPH_SIGMAF,
    GLYPH_PHI,
    GLYPH_OMEGA,

    GLYPH_a_ACUTE,
    GLYPH_a_GRAVE,
    GLYPH_a_TILDE,
    GLYPH_a_CIRC,
    GLYPH_A_ACUTE,
    GLYPH_A_GRAVE,
    GLYPH_A_TILDE,
    GLYPH_A_CIRC,
    GLYPH_e_ACUTE,
    GLYPH_e_CIRC,
    GLYPH_E_ACUTE,
    GLYPH_E_CIRC,
    GLYPH_i_ACUTE,
    GLYPH_I_ACUTE,
    GLYPH_o_ACUTE,
    GLYPH_o_CIRC,
    GLYPH_o_TILDE,
    GLYPH_O_ACUTE,
    GLYPH_O_CIRC,
    GLYPH_O_TILDE,
    GLYPH_u_ACUTE,
    GLYPH_U_ACUTE,
    GLYPH_c_CEDILLA,
    GLYPH_C_CEDILLA,
};

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

static char map_codepoint(u16 cp){
    switch (cp) {
        case 0x03B1: return GLYPH_ALPHA;
        case 0x03B2: return GLYPH_BETA;
        case 0x03B3: return GLYPH_GAMMA;
        case 0x03B4: return GLYPH_DELTA;
        case 0x03B8: return GLYPH_THETA;
        case 0x03BB: return GLYPH_LAMBDA;
        case 0x03BC: return GLYPH_MU;
        case 0x03C0: return GLYPH_PI;
        case 0x03C1: return GLYPH_RHO;
        case 0x03C3: return GLYPH_SIGMA;
        case 0x03C2: return GLYPH_SIGMAF;
        case 0x03C6: return GLYPH_PHI;
        case 0x03C9: return GLYPH_OMEGA;

        case 0x00E1: return GLYPH_a_ACUTE;
        case 0x00E0: return GLYPH_a_GRAVE;
        case 0x00E3: return GLYPH_a_TILDE;
        case 0x00E2: return GLYPH_a_CIRC;
        case 0x00C1: return GLYPH_A_ACUTE;
        case 0x00C0: return GLYPH_A_GRAVE;
        case 0x00C3: return GLYPH_A_TILDE;
        case 0x00C2: return GLYPH_A_CIRC;
        case 0x00E9: return GLYPH_e_ACUTE;
        case 0x00EA: return GLYPH_e_CIRC;
        case 0x00C9: return GLYPH_E_ACUTE;
        case 0x00CA: return GLYPH_E_CIRC;
        case 0x00ED: return GLYPH_i_ACUTE;
        case 0x00CD: return GLYPH_I_ACUTE;
        case 0x00F3: return GLYPH_o_ACUTE;
        case 0x00F4: return GLYPH_o_CIRC;
        case 0x00F5: return GLYPH_o_TILDE;
        case 0x00D3: return GLYPH_O_ACUTE;
        case 0x00D4: return GLYPH_O_CIRC;
        case 0x00D5: return GLYPH_O_TILDE;
        case 0x00FA: return GLYPH_u_ACUTE;
        case 0x00DA: return GLYPH_U_ACUTE;
        case 0x00E7: return GLYPH_c_CEDILLA;
        case 0x00C7: return GLYPH_C_CEDILLA;
        default: return 0;
    }
}

// ---- Sanitização com suporte a UTF-8 ----
static void emit_text(Vec *out, const char *beg, size_t n){
    if(!n) return;
    char *tmp = (char*)malloc(n*2);
    size_t m = 0, i = 0;
    while (i < n) {
        unsigned char ch = (unsigned char)beg[i];
        if (ch == '\t') { tmp[m++] = ' '; i++; continue; }
        if (ch >= GLYPH_ALPHA && ch <= GLYPH_C_CEDILLA) {
            tmp[m++] = (char)ch; i++; continue;
        }
        if (ch < 0x80) {
            if (ch < 32 || ch > 126) tmp[m++] = '?';
            else tmp[m++] = (char)ch;
            i++;
        } else if ((ch & 0xE0) == 0xC0 && i + 1 < n) {
            u16 cp = (u16)((ch & 0x1F) << 6) | (u16)(beg[i+1] & 0x3F);
            char mapped = map_codepoint(cp);
            tmp[m++] = mapped ? mapped : '?';
            i += 2;
        } else if ((ch & 0xF0) == 0xE0 && i + 2 < n) {
            u16 cp = (u16)((ch & 0x0F) << 12) | (u16)((beg[i+1] & 0x3F) << 6) | (u16)(beg[i+2] & 0x3F);
            char mapped = map_codepoint(cp);
            tmp[m++] = mapped ? mapped : '?';
            i += 3;
        } else {
            tmp[m++] = '?';
            i++;
        }
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
                static const struct { const char *cmd; const char *subst; } alias[] = {
                    {"alpha",      "\x80"},
                    {"beta",       "\x81"},
                    {"gamma",      "\x82"},
                    {"delta",      "\x83"},
                    {"theta",      "\x84"},
                    {"lambda",     "\x85"},
                    {"mu",         "\x86"},
                    {"pi",         "\x87"},
                    {"rho",        "\x88"},
                    {"sigma",      "\x89"},
                    {"varsigma",   "\x8A"},
                    {"phi",        "\x8B"},
                    {"omega",      "\x8C"},
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
