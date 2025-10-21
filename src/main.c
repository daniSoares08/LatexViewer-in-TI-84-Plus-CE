// main.c — Leitor com frações (GraphX + FileIOC)
#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef uint8_t  u8;
typedef uint16_t u16;

#define TAG_TEXT   0x01
#define TAG_FRAC   0x02
#define TAG_SUP    0x03
#define TAG_SUB    0x04
#define TAG_NL     0x05
#define TAG_PAR    0x06
#define TAG_END    0xFF

typedef struct { u8 *p; size_t n, i; } Stream;

typedef struct Box Box;
struct Box {
    u8 tag;
    char *text;     // TEXT
    Box *a,*b;      // FRAC: a=num, b=den;  SUP/SUB: a=child
    int w,h;        // tamanho calculado
};

static u16 rd16(Stream *s){ u16 x=s->p[s->i] | (s->p[s->i+1]<<8); s->i+=2; return x; }
static u8  rd8 (Stream *s){ return s->p[s->i++]; }

static Box* parse_seq(Stream *s, size_t lim){
    Box *head=NULL, **cur=&head;
    size_t start = s->i;
    while(s->i < s->n && (lim==0 || s->i-start<lim)){
        u8 tag = rd8(s);
        if(tag==TAG_END) break;
        Box *b = (Box*)calloc(1,sizeof(Box)); b->tag = tag;
        if(tag==TAG_TEXT){
            u16 n = rd16(s); b->text = (char*)malloc(n+1);
            memcpy(b->text, s->p+s->i, n); s->i+=n; b->text[n]=0;
        } else if(tag==TAG_FRAC){
            u16 L1 = rd16(s); Stream s1=*s; s1.n = s1.i + L1;
            b->a = parse_seq(&s1, L1); s->i += L1;
            u16 L2 = rd16(s); Stream s2=*s; s2.n = s2.i + L2;
            b->b = parse_seq(&s2, L2); s->i += L2;
        } else if(tag==TAG_SUP || tag==TAG_SUB){
            u16 L = rd16(s); Stream s1=*s; s1.n = s1.i + L;
            b->a = parse_seq(&s1, L); s->i += L;
        } else if(tag==TAG_NL || tag==TAG_PAR){
            // nada extra
        } else {
            free(b); break;
        }
        *cur = b; cur = &b->b; // encadear por b (truque simples para lista)
    }
    return head;
}

// medidas e render
static int text_w(const char *s){ return gfx_GetStringWidth(s); }
static int line_h(){ return 8; } // fonte 8x8

static void measure(Box *b){
    for(Box *p=b; p; p=p->b){ // b->b como "next"
        if(p->tag==TAG_TEXT){ p->w = text_w(p->text); p->h = line_h(); }
        else if(p->tag==TAG_FRAC){
            measure(p->a);        // num
            measure(p->a->b);     // den está em p->a->b? -> NÃO!
        }
    }
}

// caixas recursivas
static void size_node(Box *n){
    if(!n) return;
    if(n->tag==TAG_TEXT){ n->w=text_w(n->text); n->h=line_h(); return; }
    if(n->tag==TAG_SUP || n->tag==TAG_SUB){
        size_node(n->a);
        n->w = n->a ? n->a->w : 0; n->h = n->a ? n->a->h : 0;
        return;
    }
    if(n->tag==TAG_FRAC){
        // n->a == numerador; n->b == denominador (arrumamos já já)
        // (como encadeamos por b no parse, precisamos localizar filhos reais)
    }
}

// Para simplicidade, vamos definir funções dedicadas de medida/desenho “caixa”
typedef struct Node Node;
struct Node { u8 tag; char *text; Node *num,*den,*child,*next; int w,h; };

static Node* normalize(Box *seq){
    // Converte a lista “b como next” em árvore com campos explícitos
    Node *head=NULL, **nn=&head;
    for(Box *p=seq; p; ){
        Node *n = (Node*)calloc(1,sizeof(Node));
        n->tag=p->tag;
        if(p->tag==TAG_TEXT){ n->text=strdup(p->text); p=p->b; }
        else if(p->tag==TAG_NL || p->tag==TAG_PAR){ p=p->b; }
        else if(p->tag==TAG_SUP || p->tag==TAG_SUB){
            n->child = normalize(p->a);
            p=p->b;
        } else if(p->tag==TAG_FRAC){
            Node *num = normalize(p->a);
            // o segundo payload estava “depois” — use p->b como sentinela para pegar den:
            Box *denB = p->b; // já é a raiz do próximo bloco (que representa den)
            Node *den = normalize(denB);
            n->num=num; n->den=den;
            // saltar dois blocos:
            // 1) FRAC atual (p) já consumido, mas o parser encadeou den em p->b
            //    e den->next deve começar onde denB->b aponta
            p = denB ? denB->b : NULL;
        }
        *nn = n; nn = &n->next;
    }
    return head;
}

static void measure_node(Node *n){
    for(Node *p=n;p;p=p->next){
        if(p->tag==TAG_TEXT){ p->w=text_w(p->text); p->h=line_h(); }
        else if(p->tag==TAG_SUP || p->tag==TAG_SUB){
            if(p->child){ measure_node(p->child); p->w=p->child->w; p->h=p->child->h; }
            else { p->w=p->h=0; }
        } else if(p->tag==TAG_FRAC){
            if(p->num) measure_node(p->num);
            if(p->den) measure_node(p->den);
            int wn = p->num ? p->num->w : 0;
            int wd = p->den ? p->den->w : 0;
            int gap = 2, bar = 1;
            p->w = (wn>wd?wn:wd) + 4;
            p->h = (p->num?p->num->h:0) + (p->den?p->den->h:0) + gap*2 + bar;
        }
    }
}

static void draw_node(Node *n, int x, int y);
static void draw_seq(Node *n, int x, int y){
    for(Node *p=n;p;p=p->next){
        if(p->tag==TAG_TEXT){ gfx_PrintStringXY(p->text, x, y); x += p->w; }
        else if(p->tag==TAG_SUP){ if(p->child){ draw_node(p->child, x, y-6); x+=p->w; } }
        else if(p->tag==TAG_SUB){ if(p->child){ draw_node(p->child, x, y+6); x+=p->w; } }
        else if(p->tag==TAG_FRAC){
            int cx = x + (p->w - (p->num?p->num->w:0))/2;
            int cy = y;
            if(p->num) draw_node(p->num, cx, cy);
            cy += (p->num?p->num->h:0) + 2;
            gfx_HorizLine(x, cy, p->w); // barra
            cy += 2;
            cx = x + (p->w - (p->den?p->den->w:0))/2;
            if(p->den) draw_node(p->den, cx, cy);
            x += p->w;
        }
    }
}

static void draw_node(Node *n, int x, int y){
    if(!n) return;
    measure_node(n);
    draw_seq(n,x,y);
}

static Node* load_doc(void){
    ti_var_t v = ti_Open("DOC1","r");
    if(!v) return NULL;
    size_t sz = ti_GetSize(v);
    u8 *buf = (u8*)malloc(sz);
    ti_Read(buf, sz, 1, v); ti_Close(v);
    Stream s = {.p=buf,.n=sz,.i=0};
    Box *seq = parse_seq(&s,0);
    Node *doc = normalize(seq);
    free(buf);
    // (omitimos frees dos Boxes para simplicidade)
    return doc;
}

int main(void){
    gfx_Begin(); gfx_SetDrawBuffer();
    gfx_SetTextFGColor(0); gfx_SetTextBGColor(255);
    Node *doc = load_doc();
    int scroll=0;
    while(1){
        kb_Scan();
        if(kb_Data[6] & kb_Clear) break;
        if(kb_Data[7] & kb_Down) scroll += 8;
        if(kb_Data[7] & kb_Up)   scroll -= 8;
        if(scroll<0) scroll=0;

        gfx_FillScreen(255);
        gfx_SetTextScale(1,1);
        int x=8, y=8 - scroll, right=312, lineHeight=14;
        // layout simples: quebra quando p->w estourar
        Node *p=doc;
        while(p){
            measure_node(p);
            if(p->tag==TAG_TEXT || p->tag==TAG_SUP || p->tag==TAG_SUB || p->tag==TAG_FRAC){
                if(x + p->w > right){ x=8; y += lineHeight; }
                draw_node(p, x, y);
                x += p->w;
            }
            // NEWLINE implícito quando TEXT contém '\n'? (você pode
            // estender o bytecode para NL aqui se quiser)
            p = p->next;
        }
        gfx_SwapDraw();
    }
    gfx_End(); return 0;
}
