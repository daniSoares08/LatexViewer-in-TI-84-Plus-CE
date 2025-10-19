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
#define MARGIN_X 4
#define MARGIN_Y 8

// Forward
static void draw_page_index(int idx, int topline);
static void render_text_line(const char *line, int y);
static void render_markup(const char *s, int x, int y);

// Draw a page starting at line 'start_line' (scroll index)
static void draw_page(int page_idx, int start_line) {
    gfx_FillScreen(255); // white background
    gfx_SetTextFGColor(0); // black text

    const char *p = pages[page_idx];
    if (!p) return;

    // split into lines by \n and render from start_line
    int cur = 0;
    const char *ptr = p;
    int y = MARGIN_Y;
    int line_idx = 0;
    char linebuf[256];
    while (*ptr) {
        // read line
        int k=0;
        while (*ptr && *ptr!='\n' && k < (int)sizeof(linebuf)-1) {
            linebuf[k++]=*ptr++;
        }
        if (*ptr=='\n') ptr++;
        linebuf[k]=0;
        if (line_idx >= start_line && (line_idx - start_line) < LINES_PER_SCREEN) {
            render_text_line(linebuf, y + (line_idx - start_line)*LINE_HEIGHT);
        }
        line_idx++;
    }
    // page indicator
    char footer[64];
    snprintf(footer, sizeof(footer), "Pg %d", page_idx+1);
    gfx_PrintStringXY(footer, 200 - gfx_GetStringWidth(footer), 220);
    gfx_SwapDraw();
}

// render a text line, processing markup tags inline
static void render_text_line(const char *line, int y) {
    // The markup tags supported:
    // <sup>...</sup>, <sub>...</sub>, <frac>num|den</frac>, <sqrt>...</sqrt>
    // We'll parse sequentially and draw.
    int x = MARGIN_X;
    render_markup(line, x, y);
}

// small helper to draw small text (for superscripts)
static void draw_small_text(const char *s, int x, int y) {
    // use gfx_SetTextScale to reduce glyphs if available; else simulate by offset
    // graphx on CE has gfx_SetTextScale(uint8_t sx, uint8_t sy) (not always)
    // we'll approximate: draw at same font but shift up
    gfx_PrintStringXY(s, x, y);
}

// render markup recursively: draws at x,y baseline, returns new x (not used)
static void render_markup(const char *s, int x, int y) {
    const char *p = s;
    char token[512];
    while (*p) {
        if (*p=='<' && strncmp(p, "<sup>", 5)==0) {
            p += 5;
            // collect until </sup>
            const char *q = strstr(p, "</sup>");
            if (!q) q = p + strlen(p);
            int L = (int)(q-p);
            strncpy(token, p, L); token[L]=0;
            // draw superscript slightly smaller and shifted up
            gfx_SetTextScale(1,1);
            gfx_PrintStringXY(token, x, y-8);
            int adv = gfx_GetStringWidth(token);
            x += adv;
            p = q + strlen("</sup>");
        } else if (*p=='<' && strncmp(p, "<sub>",5)==0) {
            p += 5;
            const char *q = strstr(p, "</sub>");
            if (!q) q = p + strlen(p);
            int L = (int)(q-p);
            strncpy(token, p, L); token[L]=0;
            gfx_PrintStringXY(token, x, y+6);
            int adv = gfx_GetStringWidth(token);
            x += adv;
            p = q + strlen("</sub>");
        } else if (*p=='<' && strncmp(p, "<frac>",6)==0) {
            p += 6;
            const char *q = strstr(p, "</frac>");
            if (!q) q = p + strlen(p);
            int L = (int)(q-p);
            strncpy(token, p, L); token[L]=0;
            // token format num|den
            char *sep = strchr(token, '|');
            if (sep) {
                *sep = 0;
                char *num = token;
                char *den = sep + 1;
                int numw = gfx_GetStringWidth(num);
                int denw = gfx_GetStringWidth(den);
                int w = (numw > denw) ? numw : denw;
                // center numerator and denominator at x + w/2
                int cx = x;
                // numerator
                gfx_PrintStringXY(num, cx + (w - numw)/2, y-10);
                // line
                gfx_Line(cx, y+2, cx + w, y+2);
                // denominator
                gfx_PrintStringXY(den, cx + (w - denw)/2, y+6);
                x += w + 4;
            } else {
                // fallback: print token inline
                gfx_PrintStringXY(token, x, y);
                x += gfx_GetStringWidth(token);
            }
            p = q + strlen("</frac>");
        } else if (*p=='<' && strncmp(p, "<sqrt>",6)==0) {
            p += 6;
            const char *q = strstr(p, "</sqrt>");
            if (!q) q = p + strlen(p);
            int L = (int)(q-p);
            strncpy(token, p, L); token[L]=0;
            // draw a simple sqrt: "√(content)"
            gfx_PrintStringXY("√(", x, y);
            x += gfx_GetStringWidth("√(");
            gfx_PrintStringXY(token, x, y);
            x += gfx_GetStringWidth(token);
            gfx_PrintChar(')', x, y);
            x += 8;
            p = q + strlen("</sqrt>");
        } else {
            // plain character until next tag
            const char *next = strchr(p, '<');
            if (!next) next = p + strlen(p);
            int L = (int)(next - p);
            char tmp[512]; strncpy(tmp, p, L); tmp[L]=0;
            gfx_PrintStringXY(tmp, x, y);
            x += gfx_GetStringWidth(tmp);
            p = next;
        }
    }
}

int main(void) {
    gfx_Begin();
    gfx_SetTextTransparentColor(255);
    gfx_SetTextBGColor(255);

    // find number of pages
    int pagecount = 0;
    while (pages[pagecount]) pagecount++;

    int page_idx = 0;
    int top_line = 0;

    while (1) {
        draw_page(page_idx, top_line);

        uint8_t key;
        while (!(key = os_GetCSC()));
        if (key == sk_On) break;
        else if (key == sk_Right) {
            if (page_idx < pagecount-1) { page_idx++; top_line = 0; }
        } else if (key == sk_Left) {
            if (page_idx > 0) { page_idx--; top_line = 0; }
        } else if (key == sk_Down) {
            top_line++;
        } else if (key == sk_Up) {
            if (top_line>0) top_line--;
        } else if (key == sk_Clear) {
            // go to first page
            page_idx = 0; top_line = 0;
        }
        // small delay to avoid key repeat flooding
        delay(120);
    }

    gfx_End();
    return 0;
}
