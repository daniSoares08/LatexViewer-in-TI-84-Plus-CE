// tex2ce.c
// Simples conversor LaTeX -> C pages[] markup
// Uso: ./tex2ce arquivo.tex
// Gera viewer_pages.c com const char *pages[].

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define OUTNAME "viewer_pages.c"
#define LINEWIDTH 28   // chars por linha na calculadora (ajuste fino)
#define MAXPAGECHARS 4096

// Helpers
static void trim(char *s) {
    int i = 0, j = (int)strlen(s)-1;
    while (i <= j && isspace((unsigned char)s[i])) i++;
    while (j >= i && isspace((unsigned char)s[j])) j--;
    if (i==0 && j==(int)strlen(s)-1) return;
    int k=0;
    for (; i<=j; ++i) s[k++]=s[i];
    s[k]='\0';
}

static void replace_all(char *s, const char *a, const char *b) {
    char buf[8192];
    char *p;
    buf[0]=0;
    while ((p = strstr(s, a))) {
        *p = '\0';
        strcat(buf, s);
        strcat(buf, b);
        strcpy(s, p + strlen(a));
    }
    strcat(buf, s);
    strcpy(s, buf);
}

// convert simple inline math to our markup
// - converts ^{...} -> <sup>...</sup>, _{...} -> <sub>...</sub>
// - \frac{a}{b} -> <frac>a|b</frac>
// - $...$ -> inline (we unwrap)
// - known commands \Theta -> Θ, greek letters \alpha -> α (subset)
static void math_normalize(char *s) {
    // unwrap $...$ occurrences (remove $)
    char tmp[8192]; tmp[0]=0;
    int i=0, j=0;
    int in_math = 0;
    while (s[i]) {
        if (s[i]=='$') { in_math ^=1; i++; continue; }
        tmp[j++] = s[i++];
    }
    tmp[j]=0; strcpy(s,tmp);

    // replace \frac{a}{b} with <frac>a|b</frac> (simple, single level)
    char *p = s;
    char out[8192]; out[0]=0;
    while ((p = strstr(p, "\\frac{"))) {
        *p = '\0';
        strcat(out, s);
        // parse numerator
        char *q = p + strlen("\\frac{");
        char num[256]="", den[256]="";
        int k=0, brace=1;
        while (*q && brace>0) {
            if (*q=='{') brace++;
            else if (*q=='}') brace--;
            if (brace>0) num[k++]=*q;
            q++;
        }
        num[k]=0;
        if (*q=='\0') break;
        // expect /{den}
        if (*q=='/') q++;
        if (*q=='{') q++;
        k=0; brace=1;
        while (*q && brace>0) {
            if (*q=='{') brace++;
            else if (*q=='}') brace--;
            if (brace>0) den[k++]=*q;
            q++;
        }
        den[k]=0;
        char fracbuf[512];
        snprintf(fracbuf, sizeof(fracbuf), "<frac>%s|%s</frac>", num, den);
        strcat(out, fracbuf);
        strcpy(s, q); p = s;
    }
    if (out[0]) {
        strcat(out, s);
        strcpy(s, out);
    }

    // ^{...} and _{...}
    // naive single-pass replace
    char tmp2[8192]; tmp2[0]=0;
    i=0; j=0;
    while (s[i]) {
        if (s[i]=='^' && s[i+1]=='{') {
            i+=2;
            char inside[128]; int k=0;
            while (s[i] && !(s[i]=='}')) inside[k++]=s[i++]; inside[k]=0;
            i++; // skip }
            strcat(tmp2, "<sup>");
            strcat(tmp2, inside);
            strcat(tmp2, "</sup>");
        } else if (s[i]=='_' && s[i+1]=='{') {
            i+=2;
            char inside[128]; int k=0;
            while (s[i] && !(s[i]=='}')) inside[k++]=s[i++]; inside[k]=0;
            i++;
            strcat(tmp2, "<sub>");
            strcat(tmp2, inside);
            strcat(tmp2, "</sub>");
        } else {
            int len = strlen(tmp2);
            tmp2[len]=s[i];
            tmp2[len+1]=0;
            i++;
        }
    }
    strcpy(s,tmp2);

    // simple token replacements
    replace_all(s, "\\Theta", "Θ");
    replace_all(s, "\\alpha", "α");
    replace_all(s, "\\beta", "β");
    replace_all(s, "\\gamma", "γ");
    replace_all(s, "\\leq", "<=");
    replace_all(s, "\\geq", ">=");
    // remove \\ (linebreaks) and other packages
    replace_all(s, "\\\\", " ");
}

// wrap lines to width, using spaces as breakpoints, returns appended to dest with '\n'
static void wrap_and_append(const char *src, char *dest) {
    int w = LINEWIDTH;
    char buf[4096]; strcpy(buf, src);
    int len = (int)strlen(buf);
    int i=0;
    while (i < len) {
        // find window
        int end = (i + w < len) ? i + w : len;
        if (end == len) {
            strncat(dest, buf + i, end - i);
            strcat(dest, "\n");
            break;
        }
        // backtrack to last space
        int b = end;
        while (b > i && buf[b] != ' ') b--;
        if (b <= i) b = end; // no space found
        strncat(dest, buf + i, b - i);
        strcat(dest, "\n");
        i = (b == end) ? end : b + 1;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { printf("Uso: %s arquivo.tex\n", argv[0]); return 1; }
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror("fopen"); return 1; }

    char line[2048];
    char pagebuf[MAXPAGECHARS+1]; pagebuf[0]=0;
    char outname[256]; snprintf(outname, sizeof(outname), OUTNAME);
    FILE *out = fopen(outname, "w");
    if (!out) { perror("out"); fclose(f); return 1; }

    // collect blocks separated by \section* or \maketitle as page breaks
    fprintf(out, "/* viewer_pages.c -- generated by tex2ce */\n\n");
    fprintf(out, "#include <stddef.h>\n\n");
    fprintf(out, "const char *pages[] = {\n");

    // Read file, do simple processing
    char accum[8192]; accum[0]=0;
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (strlen(line)==0) {
            // paragraph break
            if (strlen(accum)) {
                // process accumulated paragraph
                char proc[8192]; proc[0]=0;
                // replace environments we don't need
                replace_all(accum, "\\\\item", "- ");
                // remove common preamble commands
                if (strstr(accum, "\\maketitle")) {
                    // nothing
                }
                // math normalization
                math_normalize(accum);
                wrap_and_append(accum, proc);

                // append to pagebuf
                strcat(pagebuf, proc);
                strcat(pagebuf, "\n");
                accum[0]=0;
            }
            continue;
        }

        if (strstr(line, "\\section*{") || strstr(line, "\\maketitle") ) {
            // flush current page if any
            if (strlen(pagebuf)) {
                // write page to file (escape quotes)
                char esc[16384]; esc[0]=0;
                for (int i=0;i<(int)strlen(pagebuf);++i) {
                    if (pagebuf[i]=='"') strcat(esc, "\\\"");
                    else if (pagebuf[i]=='\\') strcat(esc, "\\\\");
                    else {
                        int l = strlen(esc);
                        esc[l]=pagebuf[i]; esc[l+1]=0;
                    }
                }
                fprintf(out, "    \"%s\",\n", esc);
                pagebuf[0]=0;
            }
            // write the section title as separate page header
            char *s = NULL;
            if ((s = strstr(line, "\\section*{"))) {
                char *q = s + strlen("\\section*{");
                char title[512]; int k=0;
                while (*q && *q!='}') { title[k++]=*q++; } title[k]=0;
                char tproc[1024]; tproc[0]=0;
                snprintf(tproc, sizeof(tproc), "%s\n\n", title);
                char esc[2048]; esc[0]=0;
                for (int i=0;i<(int)strlen(tproc);++i) {
                    if (tproc[i]=='"') strcat(esc, "\\\"");
                    else if (tproc[i]=='\\') strcat(esc, "\\\\");
                    else {
                        int l = strlen(esc);
                        esc[l]=tproc[i]; esc[l+1]=0;
                    }
                }
                fprintf(out, "    \"%s\",\n", esc);
            } else if (strstr(line, "\\maketitle")) {
                // ignore; title likely in later lines — continue
            }
            continue;
        }

        // otherwise accumulate line
        // handle \item: keep dash
        if (strstr(line, "\\item")) {
            replace_all(line, "\\item", "- ");
        }
        // append to accum with a space
        if (strlen(accum)) strcat(accum, " ");
        strcat(accum, line);
    }

    // flush remaining accum/pagebuf
    if (strlen(accum)) {
        char proc[8192]; proc[0]=0;
        math_normalize(accum);
        wrap_and_append(accum, proc);
        strcat(pagebuf, proc);
        strcat(pagebuf, "\n");
    }
    if (strlen(pagebuf)) {
        char esc[16384]; esc[0]=0;
        for (int i=0;i<(int)strlen(pagebuf);++i) {
            if (pagebuf[i]=='"') strcat(esc, "\\\"");
            else if (pagebuf[i]=='\\') strcat(esc, "\\\\");
            else {
                int l = (int)strlen(esc);
                esc[l]=pagebuf[i]; esc[l+1]=0;
            }
        }
        fprintf(out, "    \"%s\",\n", esc);
    }

    // sentinel
    fprintf(out, "    NULL\n};\n");
    fclose(f); fclose(out);
    printf("Gerado %s com pages[].\n", OUTNAME);
    return 0;
}
