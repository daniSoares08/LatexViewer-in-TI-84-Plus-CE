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

// ---- caracteres especiais (>= 0x80) ----
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

/* ------------ Glifos customizados (grego + acentos) ------------ */

typedef struct {
    u8 w, h;
    const u8 *rows;  // bits menos significativos; largura = w
} CustomGlyph;

typedef struct {
    u8 code;
    char base;
    enum { ACC_ACUTE, ACC_GRAVE, ACC_TILDE, ACC_CIRC, ACC_CEDILLA } type;
} AccentGlyph;

static const u8 gl_alpha[]  = {0x1C,0x02,0x1E,0x22,0x22,0x22,0x3E};
static const u8 gl_beta[]   = {0x3C,0x22,0x3C,0x22,0x22,0x22,0x3C};
static const u8 gl_gamma[]  = {0x3E,0x20,0x20,0x20,0x20,0x20,0x20};
static const u8 gl_delta[]  = {0x0C,0x12,0x21,0x21,0x21,0x12,0x0C};
static const u8 gl_theta[]  = {0x1E,0x21,0x21,0x1E,0x21,0x21,0x1E};
static const u8 gl_lambda[] = {0x08,0x10,0x10,0x28,0x24,0x24,0x3E};
static const u8 gl_mu[]     = {0x21,0x33,0x33,0x2D,0x2D,0x21,0x21};
static const u8 gl_pi[]     = {0x3F,0x24,0x24,0x24,0x24,0x24,0x24};
static const u8 gl_rho[]    = {0x3C,0x22,0x22,0x3C,0x20,0x20,0x20};
static const u8 gl_sigma[]  = {0x3F,0x01,0x02,0x04,0x08,0x10,0x3F};
static const u8 gl_sigmaf[] = {0x1E,0x20,0x20,0x1C,0x02,0x02,0x3C};
static const u8 gl_phi[]    = {0x1E,0x2D,0x2D,0x3F,0x2D,0x2D,0x1E};
static const u8 gl_omega[]  = {0x1E,0x21,0x21,0x21,0x21,0x12,0x33};

static const struct { u8 code; CustomGlyph g; } g_custom_table[] = {
    { GLYPH_ALPHA,  {6,7, gl_alpha  } },
    { GLYPH_BETA,   {6,7, gl_beta   } },
    { GLYPH_GAMMA,  {6,7, gl_gamma  } },
    { GLYPH_DELTA,  {6,7, gl_delta  } },
    { GLYPH_THETA,  {6,7, gl_theta  } },
    { GLYPH_LAMBDA, {6,7, gl_lambda } },
    { GLYPH_MU,     {6,7, gl_mu     } },
    { GLYPH_PI,     {6,7, gl_pi     } },
    { GLYPH_RHO,    {6,7, gl_rho    } },
    { GLYPH_SIGMA,  {6,7, gl_sigma  } },
    { GLYPH_SIGMAF, {6,7, gl_sigmaf } },
    { GLYPH_PHI,    {6,7, gl_phi    } },
    { GLYPH_OMEGA,  {6,7, gl_omega  } },
};

static const AccentGlyph g_accents[] = {
    { GLYPH_a_ACUTE,   'a', ACC_ACUTE   },
    { GLYPH_a_GRAVE,   'a', ACC_GRAVE   },
    { GLYPH_a_TILDE,   'a', ACC_TILDE   },
    { GLYPH_a_CIRC,    'a', ACC_CIRC    },
    { GLYPH_A_ACUTE,   'A', ACC_ACUTE   },
    { GLYPH_A_GRAVE,   'A', ACC_GRAVE   },
    { GLYPH_A_TILDE,   'A', ACC_TILDE   },
    { GLYPH_A_CIRC,    'A', ACC_CIRC    },
    { GLYPH_e_ACUTE,   'e', ACC_ACUTE   },
    { GLYPH_e_CIRC,    'e', ACC_CIRC    },
    { GLYPH_E_ACUTE,   'E', ACC_ACUTE   },
    { GLYPH_E_CIRC,    'E', ACC_CIRC    },
    { GLYPH_i_ACUTE,   'i', ACC_ACUTE   },
    { GLYPH_I_ACUTE,   'I', ACC_ACUTE   },
    { GLYPH_o_ACUTE,   'o', ACC_ACUTE   },
    { GLYPH_o_CIRC,    'o', ACC_CIRC    },
    { GLYPH_o_TILDE,   'o', ACC_TILDE   },
    { GLYPH_O_ACUTE,   'O', ACC_ACUTE   },
    { GLYPH_O_CIRC,    'O', ACC_CIRC    },
    { GLYPH_O_TILDE,   'O', ACC_TILDE   },
    { GLYPH_u_ACUTE,   'u', ACC_ACUTE   },
    { GLYPH_U_ACUTE,   'U', ACC_ACUTE   },
    { GLYPH_c_CEDILLA, 'c', ACC_CEDILLA },
    { GLYPH_C_CEDILLA, 'C', ACC_CEDILLA },
};

static const CustomGlyph* find_custom(u8 code){
    for (size_t i=0;i<sizeof(g_custom_table)/sizeof(g_custom_table[0]);++i){
        if (g_custom_table[i].code == code) return &g_custom_table[i].g;
    }
    return NULL;
}

static const AccentGlyph* find_accent(u8 code){
    for (size_t i=0;i<sizeof(g_accents)/sizeof(g_accents[0]);++i){
        if (g_accents[i].code == code) return &g_accents[i];
    }
    return NULL;
}

static void draw_custom(const CustomGlyph *g, int x, int y){
    for (u8 r=0; r<g->h; ++r){
        u8 row = g->rows[r];
        for (u8 c=0; c<g->w; ++c){
            if (row & (1 << (g->w - 1 - c))) gfx_SetPixel(x + c, y + r);
        }
    }
}

static void draw_accent(const AccentGlyph *a, int x, int y){
    int w = gfx_GetCharWidth(a->base);
    switch (a->type) {
        case ACC_ACUTE:
            gfx_SetPixel(x + w - 3, y); gfx_SetPixel(x + w - 2, y); gfx_SetPixel(x + w - 2, y + 1);
            break;
        case ACC_GRAVE:
            gfx_SetPixel(x + 1, y); gfx_SetPixel(x + 2, y); gfx_SetPixel(x + 1, y + 1);
            break;
        case ACC_TILDE:
            gfx_SetPixel(x + 1, y + 1); gfx_SetPixel(x + 2, y); gfx_SetPixel(x + 3, y + 1); gfx_SetPixel(x + 4, y);
            break;
        case ACC_CIRC:
            gfx_SetPixel(x + w/2, y); gfx_SetPixel(x + w/2 - 1, y + 1); gfx_SetPixel(x + w/2 + 1, y + 1);
            break;
        case ACC_CEDILLA:
            gfx_SetPixel(x + 2, y + 7); gfx_SetPixel(x + 2, y + 6); gfx_SetPixel(x + 3, y + 7);
            break;
    }
}

static int glyph_width(u8 c){
    if (c >= 32 && c <= 126) return gfx_GetCharWidth(c);
    const AccentGlyph *a = find_accent(c); if (a) return gfx_GetCharWidth(a->base);
    const CustomGlyph *g = find_custom(c); if (g) return g->w;
    return gfx_GetCharWidth('?');
}

static void draw_glyph(u8 c, int x, int y){
    if (c >= 32 && c <= 126) { gfx_PrintCharXY(c, x, y); return; }
    const AccentGlyph *a = find_accent(c); if (a) { gfx_PrintCharXY(a->base, x, y); draw_accent(a, x, y); return; }
    const CustomGlyph *g = find_custom(c); if (g) { draw_custom(g, x, y); return; }
    gfx_PrintCharXY('?', x, y);
}

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
static inline int text_w(const char *s){
    int w = 0;
    while (*s) w += glyph_width((u8)*s++);
    return w;
}

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

// mede uma sequencia (somatorio de larguras; define alturas dos nos)
static void measure_seq(Node *seq){
    for (Node *p = seq; p; p = p->next) {
        measure_node(p);
    }
}

// ---- ADICIONE ESTES 2 PROTÓTIPOS AQUI ----
static void draw_seq(Node *seq, int x, int y);
static void draw_one(Node *p, int x, int y);
// ------------------------------------------

// Flag de contexto: estamos dentro de uma fracao?
static int g_in_frac = 0;

// Largura util (margens iguais as do main)
static int g_left  = 8;
static int g_right = 312;

// Quebra e desenha um texto longo respeitando a largura disponivel da linha.
// Atualiza x,y,lineH conforme quebra linhas.
static void draw_text_wrap(const char *s, int *x, int *y, int *lineH){
    while (*s) {
        if (*lineH < text_h()) *lineH = text_h();
        int avail = g_right - *x;
        if (avail <= 0) {
            *x = g_left;
            *y += *lineH + LEADING;
            *lineH = text_h();
            avail = g_right - *x;
        }

        int i = 0, last_space = -1, wacc = 0;
        while (s[i]) {
            int cw = glyph_width((u8)s[i]);
            if (wacc + cw > avail) break;
            if (s[i] == ' ') last_space = i;
            wacc += cw; i++;
        }

        int cut = (!s[i]) ? i : (last_space >= 0 ? last_space : i);
        if (cut <= 0) cut = 1;

        int draw_w = 0;
        for (int k=0; k<cut; ++k) { draw_glyph((u8)s[k], *x + draw_w, *y); draw_w += glyph_width((u8)s[k]); }

        *x += draw_w;
        if (!s[i]) return; // terminou a string

        *x = g_left;
        *y += *lineH + LEADING;
        *lineH = text_h();
        s += cut;
        if (*s == ' ') s++;
    }
}

/* ---------------- Desenho ---------------- */

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

        g_left = left; g_right = right;
        int x = left, y = 24 - scroll;   // “respiro” no topo
        int lineH = text_h();            // altura atual da linha (dinâmica)

        Node *p = doc;
        while (p) {
            if (p->tag == TAG_NL || p->tag == TAG_PAR) {
                x = left;
                y += lineH + LEADING + (p->tag == TAG_PAR ? text_h() : 0);
                lineH = text_h();
                p = p->next;
                continue;
            }

            // --- NOVO: se a linha atual esta inteira acima do topo, pula ela ---
            if (y < 0) {
                // Consome-nos ate o fim da linha (NL/PAR) sem desenhar
                while (p && p->tag != TAG_NL && p->tag != TAG_PAR) {
                    // ainda atualiza o lineH para subir corretamente
                    if (p->h > lineH) lineH = p->h;
                    p = p->next;
                }
                // aplica a quebra de linha uma unica vez
                x = left;
                y += lineH + LEADING;
                lineH = text_h();
                // se chegou num PAR, adiciona a linha extra e avanca esse token
                if (p && p->tag == TAG_PAR) {
                    y += text_h();
                }
                // avanca o token NL/PAR (se havia) e segue
                if (p) p = p->next;
                continue;
            }
            // -------------------------------------------------------------------

            // quebra de linha automatica para itens nao-TEXT
            if (p->tag != TAG_TEXT && x + p->w > right) {
                x = left;
                y += lineH + LEADING;
                lineH = text_h();
            }

            if (y >= 240) break; // abaixo da tela: pare

            // desenha; TAG_TEXT tem wrap interno
            if (p->tag == TAG_TEXT) {
                draw_text_wrap(p->text, &x, &y, &lineH);
            } else {
                draw_one(p, x, y);
                if (p->h > lineH) lineH = p->h;
                x += p->w;
            }

            p = p->next;
        }

        gfx_SwapDraw();
    }
}
