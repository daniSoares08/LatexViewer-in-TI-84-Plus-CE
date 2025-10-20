// tex2ce.c
// Conversor simples LaTeX -> C (pages[]) com marcação leve para TI-84+ CE
// Uso no PC:
//   gcc tex2ce.c -o tex2ce
//   ./tex2ce caminho/arquivo.tex
// Gera viewer_pages.c com: const char *pages[] = { "...\n", "...", NULL };

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define OUTNAME "viewer_pages.c"
#define LINEWIDTH 28        // largura em caracteres para quebra de linha
#define MAXPAGECHARS 8192

// ---------- utils ----------
static void trim(char *s) {
    int i = 0, j = (int)strlen(s) - 1;
    while (i <= j && isspace((unsigned char)s[i])) i++;
    while (j >= i && isspace((unsigned char)s[j])) j--;
    if (i == 0 && j == (int)strlen(s) - 1) return;
    int k = 0;
    for (; i <= j; ++i) s[k++] = s[i];
    s[k] = '\0';
}

static void replace_all(char *s, const char *a, const char *b) {
    if (!*a) return;
    char buf[65536];
    buf[0] = 0;
    const char *p = s;
    for (;;) {
        const char *q = strstr(p, a);
        if (!q) { strncat(buf, p, sizeof(buf)-strlen(buf)-1); break; }
        strncat(buf, p, (size_t)(q - p));
        strncat(buf, b, sizeof(buf)-strlen(buf)-1);
        p = q + strlen(a);
    }
    strncpy(s, buf, 65536-1);
    s[65536-1] = '\0';
}

// Extrai {conteudo} respeitando chaves aninhadas; s[i]=='{'
static int extract_braced(const char *s, int i, char *out, int outsz) {
    int depth = 0, k = 0;
    if (s[i] != '{') return i;
    i++; depth = 1;
    while (s[i] && depth > 0) {
        if (s[i] == '{') { depth++; if (depth <= 1) { /*noop*/ } }
        else if (s[i] == '}') { depth--; if (depth == 0) { i++; break; } }
        if (depth > 0 && k < outsz - 1) out[k++] = s[i];
        if (depth > 0) i++;
    }
    out[k] = 0;
    return i;
}

// Remove \cmd{...} mantendo apenas o conteúdo dentro das chaves
static void strip_cmd_braced(char *s, const char *cmd) {
    char out[65536]; out[0] = 0;
    const char *p = s;
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\\%s", cmd);
    while (*p) {
        const char *hit = strstr(p, pattern);
        if (!hit) { strncat(out, p, sizeof(out)-strlen(out)-1); break; }
        strncat(out, p, (size_t)(hit - p));
        const char *q = hit + strlen(pattern);
        if (*q == '{') {
            char inside[4096] = "";
            int idx = (int)(q - s);
            idx = extract_braced(s, idx, inside, sizeof(inside));
            strncat(out, inside, sizeof(out)-strlen(out)-1);
            p = s + idx;
        } else {
            // sem argumento com chaves → copia literal e segue
            strncat(out, hit, strlen(pattern));
            p = hit + strlen(pattern);
        }
    }
    strncpy(s, out, sizeof(out)-1);
    s[sizeof(out)-1] = '\0';
}

// simplified ASCII transliteration for CE default font
static void ascii_simplify(char *s) {
    // dashes
    replace_all(s, "–", "-");
    replace_all(s, "—", "-");
    // Portuguese accents (lowercase)
    replace_all(s, "á", "a"); replace_all(s, "à", "a");
    replace_all(s, "â", "a"); replace_all(s, "ã", "a");
    replace_all(s, "ç", "c");
    replace_all(s, "é", "e"); replace_all(s, "ê", "e");
    replace_all(s, "í", "i");
    replace_all(s, "ó", "o"); replace_all(s, "ô", "o"); replace_all(s, "õ", "o");
    replace_all(s, "ú", "u");
    // uppercase
    replace_all(s, "Á", "A"); replace_all(s, "À", "A");
    replace_all(s, "Â", "A"); replace_all(s, "Ã", "A");
    replace_all(s, "Ç", "C");
    replace_all(s, "É", "E"); replace_all(s, "Ê", "E");
    replace_all(s, "Í", "I");
    replace_all(s, "Ó", "O"); replace_all(s, "Ô", "O"); replace_all(s, "Õ", "O");
    replace_all(s, "Ú", "U");
}

// Converte algumas sequências LaTeX em ASCII
static void latex_words_to_ascii(char *s) {
    // comuns no arquivo de exemplo
    replace_all(s, "\\lfloor", "[");
    replace_all(s, "\\rfloor", "]");
    replace_all(s, "\\lceil", "<");
    replace_all(s, "\\rceil", ">");
    replace_all(s, "\\log", "log");
    replace_all(s, "\\Theta", "Theta");
    // letras gregas -> nomes ASCII (mínimo útil)
    replace_all(s, "\\alpha", "alpha");
    replace_all(s, "\\beta",  "beta");
    replace_all(s, "\\gamma", "gamma");
    // relacionais
    replace_all(s, "\\leq", "<=");
    replace_all(s, "\\geq", ">=");
    // quebras LaTeX -> espaço
    replace_all(s, "\\\\", " ");
}

// normaliza matemática inline simples: $, ^, _, \frac, \sqrt
static void math_normalize(char *s) {
    // 1) remover apenas os cifrões, manter conteúdo
    {
        char tmp[65536]; int ti=0;
        for (int i=0; s[i]; ++i) {
            if (s[i] == '$') continue;
            tmp[ti++] = s[i];
        }
        tmp[ti] = 0;
        strcpy(s, tmp);
    }

    // 2) \sqrt{a} -> <sqrt>a</sqrt>
    {
        char out[65536]; out[0] = 0;
        const char *p = s;
        while (1) {
            const char *hit = strstr(p, "\\sqrt");
            if (!hit) { strncat(out, p, sizeof(out)-strlen(out)-1); break; }
            strncat(out, p, (size_t)(hit - p));
            const char *q = hit + strlen("\\sqrt");
            if (*q != '{') { strncat(out, "\\sqrt", sizeof(out)-strlen(out)-1); p = q; continue; }
            char rad[2048] = "";
            int idx = (int)(q - s);
            idx = extract_braced(s, idx, rad, sizeof(rad));
            char frag[4096];
            snprintf(frag, sizeof(frag), "<sqrt>%s</sqrt>", rad);
            strncat(out, frag, sizeof(out)-strlen(out)-1);
            p = s + idx;
        }
        strncpy(s, out, sizeof(out)-1);
        s[sizeof(out)-1] = '\0';
    }

    // 3) \frac{a}{b} -> <frac>a|b</frac> (com chaves aninhadas)
    {
        char out[65536]; out[0] = 0;
        const char *p = s;
        while (1) {
            const char *hit = strstr(p, "\\frac");
            if (!hit) { strncat(out, p, sizeof(out)-strlen(out)-1); break; }
            strncat(out, p, (size_t)(hit - p));
            const char *q = hit + strlen("\\frac");
            if (*q != '{') { strncat(out, "\\frac", sizeof(out)-strlen(out)-1); p = q; continue; }
            char num[2048] = "", den[2048] = "";
            int idx = (int)(q - s);
            idx = extract_braced(s, idx, num, sizeof(num));   // após '}' do numerador
            if (s[idx] == '{') {
                idx = extract_braced(s, idx, den, sizeof(den));
                char frag[5000];
                snprintf(frag, sizeof(frag), "<frac>%s|%s</frac>", num, den);
                strncat(out, frag, sizeof(out)-strlen(out)-1);
                p = s + idx;
            } else {
                // malformado: retorna literal
                strncat(out, "\\frac", sizeof(out)-strlen(out)-1);
                p = hit + 5;
            }
        }
        strncpy(s, out, sizeof(out)-1);
        s[sizeof(out)-1] = '\0';
    }

    // 4) ^{...} / _{...} com aninhamento, e ^x/_x (1 char)
    {
        char out[65536]; int oi = 0;
        for (int i=0; s[i]; ) {
            if (s[i]=='^' || s[i]=='_') {
                int is_sup = (s[i]=='^'); i++;
                char inside[4096]="";
                if (s[i]=='{') {
                    int ni = extract_braced(s, i, inside, sizeof(inside));
                    i = ni;
                } else if (s[i]) {
                    inside[0] = s[i]; inside[1]=0; i++;
                }
                const char *open  = is_sup ? "<sup>" : "<sub>";
                const char *close = is_sup ? "</sup>" : "</sub>";
                int L = (int)strlen(open);
                memcpy(out+oi, open, L); oi += L;
                L = (int)strlen(inside);
                memcpy(out+oi, inside, L); oi += L;
                L = (int)strlen(close);
                memcpy(out+oi, close, L); oi += L;
            } else {
                out[oi++] = s[i++];
            }
            if (oi >= (int)sizeof(out)-8) break;
        }
        out[oi] = 0;
        strcpy(s, out);
    }

    // 5) remover formatação de texto mantendo conteúdo
    strip_cmd_braced(s, "textbf");
    strip_cmd_braced(s, "emph");
    strip_cmd_braced(s, "textit");
    strip_cmd_braced(s, "mathbf");
    replace_all(s, "\\displaystyle", "");

    // 6) outras palavras/seqs
    latex_words_to_ascii(s);
}

// wrap por largura fixa respeitando quebras de linha explícitas
static void wrap_and_append(const char *src, char *dest) {
    char whole[65536]; strncpy(whole, src, sizeof(whole)-1); whole[sizeof(whole)-1]=0;
    char *p = whole;

    while (*p) {
        // acha fim de linha (ou fim da string)
        char *line_end = strchr(p, '\n');
        if (!line_end) line_end = p + strlen(p);
        char linebuf[4096];
        int seglen = (int)(line_end - p);
        if (seglen >= (int)sizeof(linebuf)) seglen = (int)sizeof(linebuf)-1;
        strncpy(linebuf, p, seglen); linebuf[seglen] = 0;

        // wrap por espaços
        int len = (int)strlen(linebuf);
        int i = 0;
        if (len == 0) {
            strncat(dest, "\n", MAXPAGECHARS - strlen(dest) - 1);
        } else {
            while (i < len) {
                int end = (i + LINEWIDTH < len) ? i + LINEWIDTH : len;
                if (end == len) {
                    strncat(dest, linebuf + i, end - i);
                    strncat(dest, "\n", MAXPAGECHARS - strlen(dest) - 1);
                    break;
                }
                int b = end;
                while (b > i && linebuf[b] != ' ') b--;
                if (b <= i) b = end; // palavra maior que a largura
                strncat(dest, linebuf + i, b - i);
                strncat(dest, "\n", MAXPAGECHARS - strlen(dest) - 1);
                i = (b == end) ? end : b + 1;
            }
        }

        if (*line_end == '\n') {
            // mantém linhas em branco entre segmentos já que adicionamos \n
            p = line_end + 1;
        } else {
            p = line_end;
        }
    }
}

// escapa texto para literal C: \ -> \\, " -> \", \n -> \\n ; descarta controles e normaliza não-ASCII
static void c_escape(const char *src, char *dst, size_t dstsz) {
    dst[0]=0;
    for (size_t i=0; src[i]; ++i) {
        unsigned char ch = (unsigned char)src[i];
        if (ch == '\\')      strncat(dst, "\\\\",   dstsz - strlen(dst) - 1);
        else if (ch == '"')  strncat(dst, "\\\"",   dstsz - strlen(dst) - 1);
        else if (ch == '\n') strncat(dst, "\\n",    dstsz - strlen(dst) - 1);
        else if (ch == '\r') { /*ignore*/ }
        else if (ch < 32)    { /*ignore control*/ }
        else if (ch >= 0x80) {
            // tentativa de reconhecer en-dash UTF-8 (E2 80 93)
            if ((unsigned char)src[i] == 0xE2 &&
                (unsigned char)src[i+1] == 0x80 &&
                (unsigned char)src[i+2] == 0x93) {
                strncat(dst, "-", dstsz - strlen(dst) - 1);
                i += 2;
            } else {
                strncat(dst, "?", dstsz - strlen(dst) - 1);
            }
        } else {
            size_t L = strlen(dst);
            if (L + 1 < dstsz) { dst[L] = (char)ch; dst[L+1] = 0; }
        }
    }
}

static void flush_page(FILE *out, char *pagebuf) {
    if (!pagebuf[0]) return;
    // transliteração ASCII básica (antes do escape)
    ascii_simplify(pagebuf);
    char esc[65536];
    c_escape(pagebuf, esc, sizeof(esc));
    fprintf(out, "    \"%s\",\n", esc);
    pagebuf[0] = 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Uso: %s arquivo.tex\n", argv[0]);
        return 1;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror("fopen"); return 1; }

    FILE *out = fopen(OUTNAME, "wb");
    if (!out) { perror("fopen OUT"); fclose(f); return 1; }

    fprintf(out, "/* viewer_pages.c -- generated by tex2ce */\n\n");
    fprintf(out, "#include <stddef.h>\n\n");
    fprintf(out, "const char *pages[] = {\n");

    char line[4096];
    char accum[65536];    accum[0]=0;            // acumula parágrafo
    char pagebuf[MAXPAGECHARS]; pagebuf[0]=0;    // acumula página
    int in_doc = 0;

    while (fgets(line, sizeof(line), f)) {
        // detecta início/fim do documento
        if (!in_doc && strstr(line, "\\begin{document}")) { in_doc = 1; accum[0]=0; pagebuf[0]=0; continue; }
        if (in_doc && strstr(line, "\\end{document}"))     { break; }

        if (!in_doc) {
            // ignora preâmbulo
            continue;
        }

        // mantém linha original para detectar \section* antes de trim
        if (strstr(line, "\\section*{")) {
            // flush do que houver (parágrafo + página)
            if (accum[0]) {
                char proc[65536]; proc[0]=0;
                math_normalize(accum);
                wrap_and_append(accum, proc);
                strncat(pagebuf, proc, sizeof(pagebuf)-strlen(pagebuf)-1);
                strncat(pagebuf, "\n", sizeof(pagebuf)-strlen(pagebuf)-1);
                accum[0]=0;
            }
            flush_page(out, pagebuf);

            // extrai título da seção e grava como “cabeçalho” (uma página própria)
            char *s = strstr(line, "\\section*{");
            char title[1024]="";
            int i = (int)(s - line) + (int)strlen("\\section*");
            if (line[i] == '{') {
                i = extract_braced(line, i, title, sizeof(title));
            }
            // título + linha em branco
            char head[2048]; head[0]=0;
            strncat(head, title, sizeof(head)-strlen(head)-1);
            strncat(head, "\n\n", sizeof(head)-strlen(head)-1);
            char esc[4096]; c_escape(head, esc, sizeof(esc));
            fprintf(out, "    \"%s\",\n", esc);
            continue;
        }

        // \maketitle = quebra de página (mas ignora conteúdo)
        if (strstr(line, "\\maketitle")) {
            if (accum[0]) {
                char proc[65536]; proc[0]=0;
                math_normalize(accum);
                wrap_and_append(accum, proc);
                strncat(pagebuf, proc, sizeof(pagebuf)-strlen(pagebuf)-1);
                strncat(pagebuf, "\n", sizeof(pagebuf)-strlen(pagebuf)-1);
                accum[0]=0;
            }
            flush_page(out, pagebuf);
            continue;
        }

        // Acúmulo normal
        {
            char work[4096]; strncpy(work, line, sizeof(work)-1); work[sizeof(work)-1]=0;
            trim(work);
            if (!*work) {
                if (accum[0]) {
                    char proc[65536]; proc[0]=0;
                    // trata \item e outras coisinhas
                    replace_all(accum, "\\item", "- ");
                    math_normalize(accum);
                    wrap_and_append(accum, proc);
                    strncat(pagebuf, proc, sizeof(pagebuf)-strlen(pagebuf)-1);
                    strncat(pagebuf, "\n", sizeof(pagebuf)-strlen(pagebuf)-1);
                    accum[0]=0;
                }
                continue;
            }

            // descarta marcadores de ambiente de lista/centro
            if (strstr(work, "\\begin{itemize}") || strstr(work, "\\end{itemize}") ||
                strstr(work, "\\begin{enumerate}") || strstr(work, "\\end{enumerate}") ||
                strstr(work, "\\begin{center}") || strstr(work, "\\end{center}")) {
                continue;
            }

            // quebra de página manual
            if (strstr(work, "\\clearpage")) {
                if (accum[0]) {
                    char proc[65536]; proc[0]=0;
                    math_normalize(accum);
                    wrap_and_append(accum, proc);
                    strncat(pagebuf, proc, sizeof(pagebuf)-strlen(pagebuf)-1);
                    strncat(pagebuf, "\n", sizeof(pagebuf)-strlen(pagebuf)-1);
                    accum[0]=0;
                }
                flush_page(out, pagebuf);
                continue;
            }

            // mantém bullets simples
            if (strstr(work, "\\item")) replace_all(work, "\\item", "- ");

            // converte \\ no fim da linha para quebra explícita
            if (strstr(work, "\\\\")) replace_all(work, "\\\\", "\n");

            if (accum[0]) strncat(accum, " ", sizeof(accum)-strlen(accum)-1);
            strncat(accum, work, sizeof(accum)-strlen(accum)-1);
        }
    }

    // flush final
    if (accum[0]) {
        char proc[65536]; proc[0]=0;
        math_normalize(accum);
        wrap_and_append(accum, proc);
        strncat(pagebuf, proc, sizeof(pagebuf)-strlen(pagebuf)-1);
        strncat(pagebuf, "\n", sizeof(pagebuf)-strlen(pagebuf)-1);
        accum[0]=0;
    }
    flush_page(out, pagebuf);

    fprintf(out, "    NULL\n};\n");
    fclose(f);
    fclose(out);
    printf("Gerado %s com pages[].\n", OUTNAME);
    return 0;
}
