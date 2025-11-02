# README-TEX — Writing the LaTeX subset for this viewer (EN)

Authoring guide for the **LaTeX subset** recognized by `tools/tex2ce`. Everything is 7-bit ASCII.

---

## What is supported

* **ASCII text** (no accents/Unicode).
* **Fractions**: `\frac{NUM}{DEN}`
* **Superscript**: `^{...}` or `^X` (without braces → 1 character)
* **Subscript**: `_{...}` or `_X` (without braces → 1 character)
* **Line break**: `\\`
* **Paragraph**: a **blank line** (2+ consecutive `\n`)

> Nesting is allowed: `\frac{a^2}{b_c}`, `\frac{\frac{1}{2}x^2}{3y_1}`, etc.

---

## What is **not** supported

* `\sqrt`, `\sum`, `\alpha`, `\beta`, environments, images, alignment, etc.
* Unicode (accents/symbols). Write plain words like “rho”, “pi”, “epsilon”, “integral”.
* **Breaks inside** `\frac{...}{...}` (no internal wrapping inside fractions).

---

## ASCII aliases (optional)

The converter replaces the commands below by equivalent **ASCII text**:

* `\rho`→`rho`, `\pi`→`pi`, `\varepsilon`/`\verepsilon`→`epsilon`
* `\approx`/`\simeq`→`~=`
* `\Rightarrow`→`=>`, `\rightarrow`/`\to`→`->`
* `\left`/`\right`/`\Big`/`\big`→`` (discarded; use normal parentheses)
* `\int`→`integral`, `\quad`→` ` (one space)
* Extras: `\cdot`→`*`, `\times`→`*`, `\leq`→`<=`, `\geq`→`>=`, `\neq`→`!=`, `\pm`→`+/-`
* `\ ` (backslash + space) becomes a **single space**

> Even with aliases, it is **safe** to just write `rho`, `pi`, `epsilon`, `~=` directly.

---

## Practical rules

1. **Pure ASCII**
   Avoid `áéíóúç`. Use 7-bit only. Any char outside 32..126 becomes `?`.

2. **Fractions**
   **Required** format: `\frac{NUM}{DEN}` with **balanced braces**.
   Do not put `\\` or blank lines **inside** `NUM`/`DEN`.

3. **Superscripts/Subscripts**
   `x^2`, `x^{10}`, `y_1`, `A_{k+1}`.
   Without braces → counts as **one** character.

4. **Breaks**
   `\\` makes an **immediate** line break.
   A **blank line** starts a **new paragraph** (extra vertical space).
   Do not use `$...$`; everything is inline.

5. **Layout on the CE**
   The viewer does **word wrapping** when width (~312 px) is exceeded.
   Still, prefer short sentences and break manually with `\\` at natural points.
   Avoid very dense single lines (especially deeply nested fractions).

6. **Scientific notation**
   Write `3.0e-6`, `8.99e9` (short and clear).

---

## Examples

**Simple sup/sub**

```
This is a test: x^2 + y_1.
Inline: E = mc^2, A_k = \frac{k(k+1)}{2}.
```

**Fractions and nesting**

```
A fraction: \frac{a+b}{c+d}
Nested: \frac{\frac{1}{2}x^2}{3y_1}
```

**Breaks and paragraphs**

```
Manual break with \ here:
line A \ line B after the break.

Paragraph below (blank line above):

Sum of the first n integers:
S_n = \frac{n(n+1)}{2}
```

---

## Quick pipeline test

At the repo root:

```
build_final EX1TEST tools\ex1.tex
```

Expected output:

```
prontos\EX1TEST.8xp
prontos\EX1TEST.8xv
```

Send **both** to the CE and run `EX1TEST`.

---

## Common pitfalls

* **Braces**: `\frac{ ... }{ ... }` must open/close; if one is missing, the converter eats until the end and breaks the doc.
* **Spaces**: are preserved (8×8 font). Many spaces = a lot of width.
* **Long lines**: wrapping helps, but break with `\\` to keep it readable.
* **Commands outside the subset**: will appear as literal text unless they are in the **aliases** list above.


---


# PT-BR version

Guia de escrita do **subset LaTeX** reconhecido por `tools/tex2ce`. Tudo é ASCII 7-bit.

---

## O que é suportado

* **Texto ASCII** (sem acentos/Unicode).
* **Frações**: `\frac{NUM}{DEN}`
* **Expoente**: `^{...}` ou `^X` (sem chaves → 1 caractere)
* **Subscrito**: `_{...}` ou `_X` (sem chaves → 1 caractere)
* **Quebra de linha**: `\\`
* **Parágrafo**: **linha em branco** (2+ quebras `\n` consecutivas)

> Aninhamento permitido: `\frac{a^2}{b_c}`, `\frac{\frac{1}{2}x^2}{3y_1}` etc.

---

## O que **não** é suportado

* `\sqrt`, `\sum`, `\alpha`, `\beta`, ambientes, imagens, alinhamento, etc.
* Unicode (acentos/símbolos). Escreva “rho”, “pi”, “epsilon”, “integral”.
* **Quebras dentro** de `\frac{...}{...}` (não há wrap interno em frações).

---

## Aliases ASCII (opcionais)

O conversor substitui os comandos abaixo por **texto ASCII** equivalente:

* `\rho`→`rho`, `\pi`→`pi`, `\varepsilon`/`\verepsilon`→`epsilon`
* `\approx`/`\simeq`→`~=`
* `\Rightarrow`→`=>`, `\rightarrow`/`\to`→`->`
* `\left`/`\right`/`\Big`/`\big`→`` (descarta; use parênteses normais)
* `\int`→`integral`, `\quad`→` ` (um espaço)
* Extras: `\cdot`→`*`, `\times`→`*`, `\leq`→`<=`, `\geq`→`>=`, `\neq`→`!=`, `\pm`→`+/-`
* `\ ` (barra + espaço) vira **um espaço**

> Mesmo com aliases, é **seguro** escrever diretamente `rho`, `pi`, `epsilon`, `~=`.

---

## Regras práticas

1) **ASCII puro**  
Evite `áéíóúç`. Use apenas 7-bit. Qualquer char fora de 32..126 vira `?`.

2) **Frações**  
Formato **obrigatório** `\frac{NUM}{DEN}` com **chaves balanceadas**.  
Não coloque `\\` ou linhas em branco **dentro** de `NUM`/`DEN`.

3) **Expoentes/Subscritos**  
`x^2`, `x^{10}`, `y_1`, `A_{k+1}`.  
Sem chaves → vale **um** caractere.

4) **Quebras**  
`\\` cria **quebra imediata** de linha.  
**Linha vazia** cria **parágrafo** (linha extra de espaço).  
Não use `$...$`; tudo é inline.

5) **Layout na CE**  
O viewer faz **wrap por palavras** quando a largura (~312 px) estoura.  
Mesmo assim, prefira frases curtas e quebre manualmente com `\\` em pontos naturais.  
Evite expressões muito densas numa única linha (principalmente frações aninhadas extensas).

6) **Notação científica**  
Escreva `3.0e-6`, `8.99e9` (curto e claro).

---

## Exemplos

**Sup/Sub simples**
```

Este e um teste: x^2 + y_1.
Inline: E = mc^2, A_k = \frac{k(k+1)}{2}.

```

**Frações e aninhamento**
```

Uma fracao: \frac{a+b}{c+d}
Aninhada: \frac{\frac{1}{2}x^2}{3y_1}

```

**Quebras e parágrafos**
```

Quebra manual com \ aqui:
linha A \ linha B apos a quebra.

Paragrafo abaixo (linha vazia acima):

Soma dos primeiros n inteiros:
S_n = \frac{n(n+1)}{2}

```

---

## Teste rápido do pipeline

Na raiz:
```

build_final EX1TEST tools\ex1.tex

```

Saída esperada:
```

prontos\EX1TEST.8xp
prontos\EX1TEST.8xv

```

Envie **ambos** para a CE e rode `EX1TEST`.

---

## “Pitfalls” comuns

* **Chaves**: `\frac{ ... }{ ... }` precisa abrir/fechar; se faltar, o conversor consome até o fim e quebra tudo.
* **Espaços**: são preservados (fonte 8×8). Muitos espaços = muita largura.
* **Linhas longas**: o wrap ajuda, mas quebre com `\\` para manter legível.
* **Comandos fora do subset**: aparecerão como texto literal, a menos que estejam na lista de **aliases** acima.

```