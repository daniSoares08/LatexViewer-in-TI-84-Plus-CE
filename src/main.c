// main.c — Leitor LaTeX simplificado (GraphX + FileIOC) p/ TI-84 Plus CE
#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef uint8_t  u8;
typedef uint16_t u16;

#define LEADING    3   // espaço extra entre linhas
#define SUP_DOWN   1   // quanto o sup desce DENTRO da fração
#define SUP_SHIFT 1   // coloca o superscrito 1px abaixo do topo da linha
#define SUB_SHIFT 6   // coloca o subscrito 6px abaixo do topo da linha
#define FRAC_GAP 2
#define FRAC_BAR 2
#define TAG_TEXT   0x01
#define TAG_FRAC   0x02
#define TAG_SUP    0x03
#define TAG_SUB    0x04
#define TAG_NL     0x05
#define TAG_PAR    0x06
#define TAG_END    0xFF

// DOC_NAME chega como token (ex.: EX1LAMB) e aqui viramos "EX1LAMB"
#ifndef DOC_NAME
#define DOC_NAME DOC1
#endif
#define _S2(x) #x
#define _S1(x) _S2(x)
#define DOC_NAME_STR _S1(DOC_NAME)

typedef struct {
    u8 *p;
    size_t n, i;
} Stream;

static u8  rd8 (Stream *s){ return (s->i < s->n) ? s->p[s->i++] : 0; }
static u16 rd16(Stream *s){
    if (s->i + 1 >= s->n) return 0;
    u16 x = s->p[s->i] | (s->p[s->i+1] << 8);
    s->i += 2;
    return x;
}

typedef struct Node Node;
struct Node {
    u8 tag;
    char *text;      // TEXT
    Node *num, *den; // FRAC
    Node *child;     // SUP/SUB
    Node *next;      // sequência
    int w,h;         // medidas calculadas
};

/* ------------ Parser: Stream -> Node (com next) --------------- */

static Node* parse_seq(Stream *s, size_t lim){
    Node *head = NULL, **cur = &head;
    size_t start = s->i;

    while (s->i < s->n && (lim == 0 || (s->i - start) < lim)) {
        u8 tag = rd8(s);
        if (tag == TAG_END) break;

        Node *n = (Node*)calloc(1, sizeof(Node));
        n->tag = tag;

        if (tag == TAG_TEXT) {
            u16 N = rd16(s);
            if (s->i + N > s->n) N = (u16)(s->n - s->i);
            n->text = (char*)malloc(N+1);
            memcpy(n->text, s->p + s->i, N); n->text[N] = 0;
            s->i += N;

        } else if (tag == TAG_FRAC) {
            u16 L1 = rd16(s);
            Stream s1 = *s; s1.n = s1.i + L1;
            n->num = parse_seq(&s1, L1);
            s->i += L1;

            u16 L2 = rd16(s);
            Stream s2 = *s; s2.n = s2.i + L2;
            n->den = parse_seq(&s2, L2);
            s->i += L2;

        } else if (tag == TAG_SUP || tag == TAG_SUB) {
            u16 L = rd16(s);
            Stream s1 = *s; s1.n = s1.i + L;
            n->child = parse_seq(&s1, L);
            s->i += L;

        } else if (tag == TAG_NL || tag == TAG_PAR) {
            // nada extra

        } else {
            // tag desconhecida: descarta nó
            free(n);
            continue;
        }

        *cur = n;
        cur = &n->next;
    }
    return head;
}

/* --------------- Medidas e Desenho ---------------- */

static inline int text_h(void){ return 8; }
static inline int text_w(const char *s){ return gfx_GetStringWidth(s); }

// mede uma sequência inline (somatório de larguras; altura = máx)
static void measure_seq(Node *seq);

static void measure_node(Node *n){
    if (!n) return;

    if (n->tag == TAG_TEXT) {
        n->w = text_w(n->text);
        n->h = text_h();                 // 8 px (topo da linha)
        return;
    }

    if (n->tag == TAG_SUP) {
        // Fora da fração ele sobe visualmente, mas não aumenta a altura da linha.
        int cw = 0, ch = 0;
        if (n->child) {
            measure_seq(n->child);
            for (Node *p = n->child; p; p = p->next) cw += p->w;
            for (Node *p = n->child; p; p = p->next) if (p->h > ch) ch = p->h;
        }
        n->w = cw;
        n->h = text_h();                 // altura da linha não muda por causa do sup
        return;
    }

    if (n->tag == TAG_SUB) {
        // Sub desce; aumente a altura da linha para dar espaço
        int cw = 0, ch = 0;
        if (n->child) {
            measure_seq(n->child);
            for (Node *p = n->child; p; p = p->next) cw += p->w;
            for (Node *p = n->child; p; p = p->next) if (p->h > ch) ch = p->h;
        }
        n->w = cw;
        n->h = text_h() + SUB_SHIFT;
        return;
    }

    if (n->tag == TAG_FRAC) {
        int wn=0, wd=0, hn=0, hd=0;
        if (n->num) {
            measure_seq(n->num);
            for (Node *p=n->num; p; p=p->next){ wn+=p->w; if (p->h>hn) hn=p->h; }
        }
        if (n->den) {
            measure_seq(n->den);
            for (Node *p=n->den; p; p=p->next){ wd+=p->w; if (p->h>hd) hd=p->h; }
        }
        n->w = ((wn>wd)?wn:wd) + 4;
        n->h = hn + FRAC_GAP + FRAC_BAR + FRAC_GAP + hd;   // tudo pra baixo do topo da linha
        return;
    }

    if (n->tag == TAG_NL || n->tag == TAG_PAR) {
        n->w = 0; n->h = text_h();
        return;
    }
}

// ---- ADICIONE ESTES 2 PROTÓTIPOS AQUI ----
static void draw_seq(Node *seq, int x, int y);
static void draw_one(Node *p, int x, int y);
// ------------------------------------------


static void measure_seq(Node *seq){
    for (Node *p = seq; p; p = p->next) measure_node(p);
}

static void seq_stats(Node *s, int *w, int *hmax){
    int ww = 0, hh = 0;
    for (Node *p = s; p; p = p->next) {
        ww += p->w;
        if (p->h > hh) hh = p->h;
    }
    if (w) *w = ww;
    if (hmax) *hmax = hh;
}

// Flag de contexto: estamos dentro de uma fração?
static int g_in_frac = 0;

static void draw_one(Node *p, int x, int y){
    if (!p) return;

    if (p->tag == TAG_TEXT) {
        gfx_PrintStringXY(p->text, x, y);     // y = topo da linha
        return;
    }

    if (p->tag == TAG_SUP) {
        if (p->child) {
            // Dentro da fração desce 1px; fora dela fica 1px abaixo do topo da linha.
            int yy = y + (g_in_frac ? SUP_DOWN : SUP_SHIFT);
            draw_seq(p->child, x, yy);
        }
        return;
    }

    if (p->tag == TAG_SUB) {
        if (p->child) draw_seq(p->child, x, y + SUB_SHIFT);
        return;
    }

    if (p->tag == TAG_FRAC) {
        int wn=0, hn=0, wd=0, hd=0;
        if (p->num) { for (Node *q=p->num; q; q=q->next){ wn+=q->w; if(q->h>hn) hn=q->h; } }
        if (p->den) { for (Node *q=p->den; q; q=q->next){ wd+=q->w; if(q->h>hd) hd=q->h; } }

        int w  = (wn > wd ? wn : wd);
        int cx = x + (p->w - w)/2;

        g_in_frac++; // --- entra em fração ---

        // Numerador no topo da caixa da linha
        if (p->num) draw_seq(p->num, cx + (w - wn)/2, y);

        // Barra (use w, centrada)
        int bar_y = y + hn + FRAC_GAP;
        gfx_SetColor(0);
        for (int t=0; t<FRAC_BAR; ++t) {
            gfx_Line(cx, bar_y + t, cx + w, bar_y + t);
        }

        // Denominador
        if (p->den) {
            int den_y = bar_y + FRAC_BAR + FRAC_GAP;
            draw_seq(p->den, cx + (w - wd)/2, den_y);
        }

        g_in_frac--; // --- sai da fração ---
        return;
    }

    // NL/PAR não desenham nada
}

static void draw_seq(Node *seq, int x, int y){
    for (Node *p = seq; p; p = p->next) {
        // Pula nós totalmente acima da janela
        if (y + p->h < 0) { 
            x += p->w;
            continue;
        }
        // Para se já passamos do fim da tela
        if (y > 240) break;

        draw_one(p, x, y);
        x += p->w;
    }
}

/* ---------------- Carregamento do AppVar ---------------- */

static Node* load_doc(void){
    ti_var_t v = ti_Open(DOC_NAME_STR,"r");
    if (!v) return NULL;

    size_t sz = ti_GetSize(v);
    if (sz == 0) { ti_Close(v); return NULL; }

    u8 *buf = (u8*)malloc(sz);
    if (!buf) { ti_Close(v); return NULL; }

    ti_Read(buf, sz, 1, v);
    ti_Close(v);

    Stream s = { .p = buf, .n = sz, .i = 0 };
    Node *doc = parse_seq(&s, 0);
    free(buf);
    return doc;
}

/* ---------------------- App ---------------------- */
int main(void){
    gfx_Begin();
    gfx_SetDrawBuffer();
    gfx_SetColor(0);                       // cor de desenho (linhas/retângulos)
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextTransparentColor(255);

    // ON instantâneo
    kb_EnableOnLatch();

    Node *doc = load_doc();
    if (!doc) {
        gfx_FillScreen(255);
        gfx_SetColor(0);
        gfx_PrintStringXY("DOC1 AppVar nao encontrado.", 8, 8);
        gfx_PrintStringXY("Gere DOC1.8xv e envie p/ a CE.", 8, 24);
        gfx_PrintStringXY("ON: sair", 8, 48);
        gfx_SwapDraw();
        while (1) { kb_Scan(); if (kb_On) break; }
        kb_ClearOnLatch();
        kb_DisableOnLatch();
        gfx_End();
        return 0;
    }

    // mede uma vez (precisamos das larguras/alturas)
    measure_seq(doc);
    uint8_t prev7 = 0;
    int warmup = 2; // ignora as teclas nos 2 primeiros frames


    int scroll = 0;
    const int left  = 8;
    const int right = 312;   // 320 - 8 de margem de cada lado

    while (1) {
        kb_Scan();
        if (kb_On) {
            kb_ClearOnLatch();
            kb_DisableOnLatch();
            gfx_End();
            return 0;
        }
        uint8_t cur7 = kb_Data[7];

        if (warmup > 0) {
            warmup--;
        } else {
            uint8_t justDown = (cur7 & kb_Down) & ~(prev7 & kb_Down);
            uint8_t justUp   = (cur7 & kb_Up)   & ~(prev7 & kb_Up);

            if (justDown) scroll += 8;
            if (justUp)   scroll -= 8;
        }
        prev7 = cur7;
        if (scroll < 0) scroll = 0;

        gfx_FillScreen(255);
        gfx_SetColor(0);          // garante preto p/ a barra da fracao

        int x = left, y = 24 - scroll;   // “respiro” no topo
        int lineH = text_h();            // altura atual da linha (dinâmica)

        for (Node *p = doc; p; p = p->next) {
            if (p->tag == TAG_NL || p->tag == TAG_PAR) {
                x = left;
                y += lineH + LEADING + (p->tag == TAG_PAR ? text_h() : 0); // PAR: linha extra
                lineH = text_h();
                continue;
            }

            // quebra de linha automática
            if (x + p->w > right) {
                x = left;
                y += lineH + LEADING;
                lineH = text_h();
            }

            // --- CLIPPING POR LINHA ---
            if (y + lineH <= 0) {              // linha inteira acima da tela
                // só avança o x, sem desenhar
                x += p->w;
                if (p->h > lineH) lineH = p->h;
                continue;
            }
            if (y >= 240) break;               // tudo daqui pra baixo já não aparece
            // ---------------------------

            // desenha apenas ESTE nó
            draw_one(p, x, y);

            if (p->h > lineH) lineH = p->h; // altura da linha = maior item
            x += p->w;

        }

        gfx_SwapDraw();
    }
}
