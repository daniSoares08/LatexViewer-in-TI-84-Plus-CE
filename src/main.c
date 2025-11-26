// main.c — Leitor LaTeX simplificado (GraphX + FileIOC) p/ TI-84 Plus CE
#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fontlibc.h>

typedef uint8_t  u8;
typedef uint16_t u16;

static fontlib_font_t *g_font = NULL;

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
static inline int text_h(void){
    if (!g_font) return 8;
    // altura útil = height + espaço abaixo
    return g_font->height + g_font->space_below;
}

static inline int text_w(const char *s){
    return fontlib_GetStringWidth(s);
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
        int avail = g_right - *x;
        if (avail <= 0) {
            // nova linha
            *x = g_left;
            *y += *lineH + LEADING;
            *lineH = text_h();
            avail = g_right - *x;
        }
        int fullw = gfx_GetStringWidth(s);
        if (fullw <= avail) {
            // cabe inteiro
            gfx_PrintStringXY(s, *x, *y);
            *x += fullw;
            return;
        }
        // procura o ultimo espaco que caiba
        int i = 0, last_space = -1, wacc = 0;
        while (s[i]) {
            int cw = gfx_GetCharWidth(s[i]);
            if (wacc + cw > avail) break;
            if (s[i] == ' ') last_space = i;
            wacc += cw; i++;
        }
        int cut = (last_space >= 0) ? last_space : i; // hard-break se nao houver espaco
        if (cut <= 0) cut = 1; // garante progresso
        // desenha o trecho [0..cut)
        char *chunk = (char*)malloc((size_t)cut + 1);
        memcpy(chunk, s, (size_t)cut);
        chunk[cut] = 0;
        gfx_PrintStringXY(chunk, *x, *y);
        free(chunk);
        // quebra de linha logo apos o trecho
        *x = g_left;
        *y += *lineH + LEADING;
        *lineH = text_h();
        // avanca o ponteiro do texto (pula um espaco se for o caso)
        s += cut;
        if (*s == ' ') s++;
    }
}

/* ---------------- Desenho ---------------- */

static void draw_one(Node *p, int x, int y){
    if (!p) return;

    if (p->tag == TAG_TEXT) {
        fontlib_SetCursorPosition(x, y);
        fontlib_DrawString(p->text);
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

static int init_os_font(void){
    // Pega a primeira fonte dentro do appvar OSLFONT
    g_font = fontlib_GetFontByIndex("OSLFONT", 0);
    if (!g_font) {
        // Se não achou, avisa usando a fonte padrão do graphx
        gfx_FillScreen(255);
        gfx_SetTextFGColor(0);
        gfx_PrintStringXY("OSLFONT.8xv nao encontrado.", 8, 8);
        gfx_PrintStringXY("Envie OSLFONT.8xv p/ a calculadora.", 8, 24);
        gfx_PrintStringXY("Depois rode o programa de novo.", 8, 40);
        gfx_SwapDraw();
        while (1) {
            kb_Scan();
            if (kb_On) return 0;
        }
    }

    fontlib_SetFont(g_font, 0);           // seleciona a fonte
    fontlib_SetForegroundColor(0);        // texto preto
    fontlib_SetBackgroundColor(255);      // fundo branco
    fontlib_SetTransparency(true);        // fundo transparente
    // Se quiser, pode limitar uma janela de texto:
    // fontlib_SetWindow(0, 0, 320, 240);

    return 1;
}

/* ---------------------- App ---------------------- */
int main(void){
    gfx_Begin();
    gfx_SetDrawBuffer();
    gfx_SetColor(0);
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextTransparentColor(255);

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

    // --- NOVO: inicializa fonte OS antes de medir texto ---
    if (!init_os_font()) {
        kb_ClearOnLatch();
        kb_DisableOnLatch();
        gfx_End();
        return 0;
    }

    // mede as larguras/alturas usando FontLib (text_w / text_h)
    measure_seq(doc);

    uint8_t prev7 = 0;
    int warmup = 2;
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
                int w = fontlib_GetStringWidth(p->text);

                // Se não couber na linha, quebra ANTES do texto todo
                if (x + w > right) {
                    x = left;
                    y += lineH + LEADING;
                    lineH = text_h();
                }

                fontlib_SetCursorPosition(x, y);
                fontlib_DrawString(p->text);
                x += w;

                if (text_h() > lineH) lineH = text_h();
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
