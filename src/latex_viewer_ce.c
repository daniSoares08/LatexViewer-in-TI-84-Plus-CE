// latex_viewer_ce.c
// TI-84 Plus CE (CEdev) viewer for pages[] with simple markup
// Requires graphx, keypadc, tice.

#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "viewer_pages.c" // gerado pelo tex2ce (pages[])

#define LINES_PER_SCREEN 10
#define LINE_HEIGHT 16
#define MARGIN_X 8
#define MARGIN_Y 8
#define SCREEN_W 320
#define SCREEN_H 240

// Forward
static void draw_page(int page_idx, int start_line);
static void render_text_line(const char *line, int y);
static void render_markup(const char *s, int x, int y);
static int  count_lines(const char *p);

// Conta número de linhas (por '\n') de uma página
static int count_lines(const char *p) {
    if (!p || !*p) return 0;
    int n = 1;
    for (const char *q = p; *q; ++q) if (*q == '\n') n++;
    return n;
}

// Desenha uma página iniciando na linha 'start_line' (índice de rolagem)
static void draw_page(int page_idx, int start_line) {
    gfx_FillScreen(255); // branco
    gfx_SetTextFGColor(0); // preto

    const char *p = pages[page_idx];
    if (!p) return;

    // split em linhas por \n e render a partir de start_line
    const char *ptr = p;
    int line_idx = 0;
    char linebuf[256];

    while (*ptr) {
        // lê linha
        int k = 0;
        while (*ptr && *ptr != '\n' && k < (int)sizeof(linebuf)-1) {
            linebuf[k++] = *ptr++;
        }
        if (*ptr == '\n') ptr++;
        linebuf[k] = 0;

        if (line_idx >= start_line && (line_idx - start_line) < LINES_PER_SCREEN) {
            int y = MARGIN_Y + (line_idx - start_line)*LINE_HEIGHT;
            render_text_line(linebuf, y);
        }
        line_idx++;
        if ((line_idx - start_line) >= LINES_PER_SCREEN) break; // evita trabalho extra
    }

    // rodapé: página atual / total
    int total_pages = 0; while (pages[total_pages]) total_pages++;
    char footer[64];
    snprintf(footer, sizeof(footer), "Pg %d/%d", page_idx+1, total_pages);
    gfx_PrintStringXY(footer, SCREEN_W - MARGIN_X - gfx_GetStringWidth(footer), SCREEN_H - 16);

    gfx_SwapDraw();
}

// renderiza uma linha de texto, processando marcação inline
static void render_text_line(const char *line, int y) {
    int x = MARGIN_X;
    render_markup(line, x, y);
}

// renderiza marcação simples: <sup>...</sup>, <sub>...</sub>, <frac>num|den</frac>, <sqrt>...</sqrt>
static void render_markup(const char *s, int x, int y) {
    const char *p = s;
    char token[512];
    while (*p) {
        if (*p=='<' && strncmp(p, "<sup>", 5)==0) {
            p += 5;
            const char *q = strstr(p, "</sup>");
            if (!q) q = p + strlen(p);
            int L = (int)(q-p);
            if (L >= (int)sizeof(token)) L = (int)sizeof(token)-1;
            strncpy(token, p, L); token[L]=0;
            // desloca para cima
            gfx_PrintStringXY(token, x, y-8);
            int adv = gfx_GetStringWidth(token);
            x += adv;
            if (*q) p = q + 6; else p = q;
        } else if (*p=='<' && strncmp(p, "<sub>",5)==0) {
            p += 5;
            const char *q = strstr(p, "</sub>");
            if (!q) q = p + strlen(p);
            int L = (int)(q-p);
            if (L >= (int)sizeof(token)) L = (int)sizeof(token)-1;
            strncpy(token, p, L); token[L]=0;
            // desloca para baixo
            gfx_PrintStringXY(token, x, y+6);
            int adv = gfx_GetStringWidth(token);
            x += adv;
            if (*q) p = q + 6; else p = q;
        } else if (*p=='<' && strncmp(p, "<frac>",6)==0) {
            p += 6;
            const char *q = strstr(p, "</frac>");
            if (!q) q = p + strlen(p);
            int L = (int)(q-p);
            if (L >= (int)sizeof(token)) L = (int)sizeof(token)-1;
            strncpy(token, p, L); token[L]=0;
            // token formato: num|den
            char *sep = strchr(token, '|');
            if (sep) {
                *sep = 0;
                const char *num = token;
                const char *den = sep + 1;
                int numw = gfx_GetStringWidth(num);
                int denw = gfx_GetStringWidth(den);
                int w = (numw > denw) ? numw : denw;
                int cx = x;
                // numerador
                gfx_PrintStringXY(num, cx + (w - numw)/2, y-10);
                // linha
                gfx_Line(cx, y+2, cx + w, y+2);
                // denominador
                gfx_PrintStringXY(den, cx + (w - denw)/2, y+6);
                x += w + 4;
            } else {
                // fallback: imprime inline
                gfx_PrintStringXY(token, x, y);
                x += gfx_GetStringWidth(token);
            }
            if (*q) p = q + 7; else p = q;
        } else if (*p=='<' && strncmp(p, "<sqrt>",6)==0) {
            p += 6;
            const char *q = strstr(p, "</sqrt>");
            if (!q) q = p + strlen(p);
            int L = (int)(q-p);
            if (L >= (int)sizeof(token)) L = (int)sizeof(token)-1;
            strncpy(token, p, L); token[L]=0;
            // desenha "√(" + conteúdo + ")"
            gfx_PrintStringXY("√(", x, y);
            x += gfx_GetStringWidth("√(");
            gfx_PrintStringXY(token, x, y);
            x += gfx_GetStringWidth(token);
            const char rb[2] = ")";
            gfx_PrintStringXY(rb, x, y);
            x += gfx_GetStringWidth(rb);
            if (*q) p = q + 7; else p = q;
        } else {
            // texto plano até o próximo '<'
            const char *next = strchr(p, '<');
            if (!next) next = p + strlen(p);
            int L = (int)(next - p);
            if (L > 0) {
                char tmp[512];
                if (L >= (int)sizeof(tmp)) L = (int)sizeof(tmp)-1;
                strncpy(tmp, p, L); tmp[L]=0;
                gfx_PrintStringXY(tmp, x, y);
                x += gfx_GetStringWidth(tmp);
            }
            p = next;
        }
    }
}

int main(void) {
    gfx_Begin();
    gfx_SetDrawBuffer(); // double buffering para evitar flicker
    gfx_SetTextTransparentColor(255);
    gfx_SetTextBGColor(255);
    gfx_SetTextFGColor(0);

    // quantidade de páginas
    int pagecount = 0;
    while (pages[pagecount]) pagecount++;

    int page_idx = 0;
    int top_line = 0;

    while (true) {
        // garante que top_line não ultrapasse o total de linhas
        int total_lines = count_lines(pages[page_idx]);
        if (top_line + LINES_PER_SCREEN > total_lines) {
            int min_top = (total_lines > LINES_PER_SCREEN) ? (total_lines - LINES_PER_SCREEN) : 0;
            if (top_line > min_top) top_line = min_top;
        }

        draw_page(page_idx, top_line);

        uint8_t key;
        while (!(key = os_GetCSC())) { /* espera tecla */ }
        if (key == sk_Clear) break;
        else if (key == sk_Right) {
            if (page_idx < pagecount-1) { page_idx++; top_line = 0; }
        } else if (key == sk_Left) {
            if (page_idx > 0) { page_idx--; top_line = 0; }
        } else if (key == sk_Down) {
            int total = count_lines(pages[page_idx]);
            if (top_line + LINES_PER_SCREEN < total) top_line++;
        } else if (key == sk_Up) {
            if (top_line>0) top_line--;
        } else if (key == sk_Clear) {
            // ir para primeira página
            page_idx = 0; top_line = 0;
        }
        // pequeno atraso para evitar repetição excessiva
        delay(120);
    }

    gfx_End();
    return 0;
}
