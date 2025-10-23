# README-TEX — Escrevendo o subset LaTeX para este viewer

Este README é um **guia de escrita** da sua linguagem de marcação minimalista (subset LaTeX) reconhecida pelo `tools/tex2ce`.

## O que é suportado

* Texto **ASCII** (sem acentos/Unicode).
* **Frações**: `\frac{numerador}{denominador}`
* **Expoente**: `^{...}` ou `^X` (sem chaves → 1 caractere)
* **Subscrito**: `_{...}` ou `_X` (sem chaves → 1 caractere)
* **Quebra de linha**: `\\`
* **Parágrafo**: **linha em branco** (uma ou mais quebras `\n` consecutivas)

> Aninhamento é permitido: `\frac{a^2}{b_c}`, `\frac{\frac{1}{2}x^2}{3y_1}` etc.

## O que **não** é suportado (por enquanto)

* `\sqrt{}`, `\sum`, `\int`, `\alpha`, `\beta`… (símbolos gregos), ambientes, imagens, alinhamento, etc.
* Unicode (acentos): use “e”, “cao”, “integral”, “pi”, etc.
* Quebras **dentro** de `\frac{...}{...}` (o render não cria múltiplas linhas dentro de frações).

## Regras práticas e dicas

1. **ASCII puro**

   * Evite `áéíóúç` — substitua por equivalentes sem acento.
   * Sinais: `*` para multiplicação, `^` para potências, `_` para subscritos.

2. **Frações**

   * Formato **obrigatório**: `\frac{NUM}{DEN}` com **chaves** balanceadas.
   * Pode aninhar: `\frac{ \frac{1}{2}x^2 }{ 3y_1 }`
   * Dentro de `NUM`/`DEN`, evite `\\` e linhas em branco.

3. **Expoentes/Subscritos**

   * `a^2`, `x^{10}`, `y_1`, `A_{k+1}`.
   * Sem chaves → pega **um** caractere: `x^2` OK, mas `x^10` precisa `x^{10}`.

4. **Quebras**

   * `\\` cria quebra **imediata** de linha.
   * **Linha em branco** cria **parágrafo** (ganha uma linha extra de espaçamento).
   * Você não precisa de modo matemático `$...$`; tudo é tratado como inline.

5. **Layout e legibilidade na CE**

   * O viewer faz **quebra automática** quando a largura passa ~312 px.
   * Prefira frases curtas; use `\\` para “guiar” a leitura no display 320×240.
   * Evite expressões muito densas numa única linha (principalmente frações aninhadas).

6. **Números científicos**

   * Escreva como `3.0e-6`, `8.99e9`, etc. (sem `\times 10^{...}` se quiser ser mais conciso).

## Exemplos úteis

### Texto simples com sup/sub

```
Este e um teste: x^2 + y_1.
Texto inline com sup/sub: E = mc^2, A_k = \frac{k(k+1)}{2}.
```

### Frações e aninhamento

```
Uma fracao simples: \frac{a+b}{c+d}
Agora uma fracao aninhada:
\frac{\frac{1}{2}x^2}{3y_1}
```

### Quebras e paragrafos

```
Quebra manual com \\ aqui:
linha A \\ linha B apos a quebra.

Outra linha:

Soma dos primeiros n inteiros:
S_n = \frac{n(n+1)}{2}
```

## Teste rápido do pipeline

Na raiz:

```
build_final EX1TEST ex1.tex
```

Saída esperada:

```
prontos\EX1TEST.8xp
prontos\EX1TEST.8xv
```

Envie os dois para a CE e rode `EX1TEST`.

## Dicas de “pitfall”

* **Chaves balanceadas**: `\frac{ ... }{ ... }` precisa **abrir e fechar**. Se faltar, o conversor segue lendo até o fim e o resultado fica quebrado.
* **Espaços**: são preservados (fonte 8×8). Vários espaços viram “largura” mesmo.
* **Linhas muito longas**: o wrap automático ajuda, mas pode cortar “feio” entre blocos; use `\\` se quiser quebrar antes.

---

Pronto! Com estes dois READMEs você tem:

* O **guia completo** pra montar seus pares `.8xp`/`.8xv` e organizar tudo em `prontos\`.
* O **guia de escrita** do `.tex` para este viewer específico.

Se quiser, escrevo uma versão **em lote** do `.bat` para processar `EX1..EX5` numa tacada só.
