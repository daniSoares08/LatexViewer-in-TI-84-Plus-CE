# README — LatexViewer-in-TI-84-Plus-CE

Overview of **how to use, build and package** the viewer + content AppVar for TI-84 Plus CE (includes CE Python).
This README reflects the current project state: **word wrapping**, **anti-flicker** at top, sup/sub "within line",
parameterized AppVar name and `tex2ce` converter with **ASCII sanitization + aliases**.

---

## What is it

* **PC Tools**
  * `tools/tex2ce.c` → converter from **LaTeX subset** to viewer's **bytecode**.
  * `convbin` → packages bytecode into an **AppVar `.8xv`**.
* **Calculator App**
  * `src/main.c` → `.8xp` program that **reads the AppVar** and **renders**: real fractions, inline sup/sub, line breaks,
    arrow key scrolling, instant ON exit, clipping and **stable scroll (no flicker)**.

You send **2 files** per exercise to the calculator:
the **executable** (`.8xp`) and the **AppVar** (`.8xv`) with the **same name** (e.g., `EX1.8xp` and `EX1.8xv`).

---

## Structure

LatexViewer-in-TI-84-Plus-CE/
- `src/`
  - `main.c`          # calculator app
- `tools/`
  - `tex2ce.c`        # converter (source)
  - `tex2ce.exe`      # build output
  - `*.tex`           # LaTeX subset texts (input)
  - `*.8xv`           # generated AppVars (intermediate; .bat cleans at end)
- `prontos/`          # FINAL OUTPUT: .8xp + .8xv pairs to send to CE
- `Makefile`          # CEdev (generates .8xp)
- `build_final.bat`   # automation (compiles tex2ce, generates .8xv/.8xp, copies and cleans)

---

## Prerequisites

- Working **CEdev** (make, convimg, linker, etc.).
- **convbin** in PATH.
- **gcc** to compile `tools/tex2ce.c`.
- Windows (`.bat` script).

---

## How to use

### 1) Write the `.tex` (subset)
Put your file in `tools\`.  
Accepted subset is in **README-TEX** (includes list of useful **ASCII aliases**).

### 2) Generate `.8xv` + `.8xp`
In the **root**, run:
```
build_final FILE_NAME tools\file.tex
```
Example:
```
build_final EX1 tools\EX1.tex
```

The script does:
1) **gcc** compiles `tools\tex2ce.c` → `tools\tex2ce.exe`  
2) `tools\tex2ce.exe` converts `.tex` → `tools\out.bin`  
3) `convbin` packages `out.bin` → `tools\FILE_NAME.8xv` (AppVar with internal name = `FILE_NAME`)  
4) `make` compiles `src/main.c` → `bin\FILE_NAME.8xp` with `-DDOC_NAME=FILE_NAME`  
5) Copies `bin\FILE_NAME.8xp` and `tools\FILE_NAME.8xv` → `prontos\`  
6) **Cleans** `tools\FILE_NAME.8xv` and `tools\out.bin`

> Without second argument, `.bat` tries `tools\%NAME%.tex` and, if missing, uses `tools\entrada.tex`.

### 3) Send to calculator
Transfer **both** from `prontos\`:
- `FILE_NAME.8xp` (program)
- `FILE_NAME.8xv` (AppVar with internal name **FILE_NAME**)

> **TI Rule**: AppVar name up to **8 characters**, preferably **UPPERCASE** and no spaces.

### 4) Run
- Open `FILE_NAME` (Cesium/PRGM).
- **Arrow keys ↑/↓**: vertical scrolling (edge trigger).
- **ON**: exits instantly.

---

## What's already implemented

**Viewer (`src/main.c`)**
- **Inline** Superscript/Subscript (SUP drops 1 px; SUB drops 6 px and increases line height).
- **Real fractions** with measurement and centering (`FRAC_BAR` thickness), nested support.
- **Word wrapping** for `TAG_TEXT` (prevents "overwriting").
- **Anti-flicker at top**: lines above top are entirely skipped.
- **Fraction context** (`g_in_frac`) for SUP positioning within fractions.
- Stable scrolling and ON latch exit.

**Converter (`tools/tex2ce.c`)**
- **7-bit ASCII** (any char outside 32..126 becomes `?`).
- **Natively supported commands**: `\frac{A}{B}`, `^{...}`/`^X`, `_{...}`/`_X`, `\\`.
- **ASCII Aliases** (mapped as plain text):
  - `\rho`→`rho`, `\pi`→`pi`, `\varepsilon`/`\verepsilon`→`epsilon`
  - `\approx`/`\simeq`→`~=`
  - `\Rightarrow`→`=>`, `\rightarrow`/`\to`→`->`
  - `\left`/`\right`/`\Big`/`\big`→`` (discarded; real parentheses remain)
  - `\int`→`integral`, `\quad`→` ` (one space)
  - extras: `\cdot`→`*`, `\times`→`*`, `\leq`→`<=`, `\geq`→`>=`, `\neq`→`!=`, `\pm`→`+/-`
- `\ ` (backslash + space) becomes **one space**.
- Line breaks: 1 LF → break (`TAG_NL`); 2+ LFs → paragraph (`TAG_PAR`).

---

## Tips / Troubleshooting

- **"AppVar not found"**: send **.8xp + .8xv**; confirm convbin's `-n` (internal name) **matches** `.8xp`'s `DOC_NAME`. Max 8 chars.
- **Literal LaTeX commands on screen**: use only the listed **subset** and **aliases**; avoid Unicode.
- **Long lines overlapping**: word wrap prevents this; still, prefer breaking with `\\` at natural points.
- **Top flicker**: already mitigated (lines above top are skipped).
- **Content update only**: if `DOC_NAME` hasn't changed, you can regenerate just the `.8xv` (`.8xp` already points to same name).



---


# PT-BR version

# README — LatexViewer-in-TI-84-Plus-CE

Visão geral de **como usar, buildar e empacotar** o viewer + AppVar de conteúdo para a TI-84 Plus CE (inclui CE Python).
Este README reflete o estado atual do projeto: **wrap por palavras**, **anti-flicker** no topo, sup/sub “dentro da linha”,
nome de AppVar parametrizado e conversor `tex2ce` com **sanitização ASCII + aliases**.

---

## O que é

* **Ferramentas no PC**
  * `tools/tex2ce.c` → conversor do **subset LaTeX** para **bytecode** do viewer.
  * `convbin` → empacota o bytecode em um **AppVar `.8xv`**.
* **App na calculadora**
  * `src/main.c` → programa `.8xp` que **lê o AppVar** e **desenha**: frações reais, sup/sub inline, quebras, rolagem
    por setas, saída imediata com ON, clipping e **scroll estável (sem “piscar”)**.

Você envia **2 arquivos** por exercício para a calculadora:
o **executável** (`.8xp`) e o **AppVar** (`.8xv`) com o **mesmo nome** (ex.: `EX1.8xp` e `EX1.8xv`).

---

## Estrutura

LatexViewer-in-TI-84-Plus-CE/
- `src/`
  - `main.c`          # app da calculadora
- `tools/`
  - `tex2ce.c`        # conversor (fonte)
  - `tex2ce.exe`      # gerado pelo build
  - `*.tex`           # textos LaTeX subset (entrada)
  - `*.8xv`           # AppVars gerados (intermediários; o .bat limpa ao final)
- `prontos/`          # SAÍDA FINAL: pares .8xp + .8xv para enviar à CE
- `Makefile`          # CEdev (gera o .8xp)
- `build_final.bat`   # automação (compila tex2ce, gera .8xv/.8xp, copia e limpa)

---

## Pré-requisitos

- **CEdev** funcionando (make, convimg, linker, etc.).
- **convbin** no PATH.
- **gcc** para compilar `tools/tex2ce.c`.
- Windows (script `.bat`).

---

## Como usar

### 1) Escreva o `.tex` (subset)
Coloque seu arquivo em `tools\`.  
O subset aceito está em **README-TEX** (inclui a lista de **aliases ASCII** úteis).

### 2) Gere `.8xv` + `.8xp`
Na **raiz**, rode:
```

build_final NOME_ARQ tools\arquivo.tex

```
Exemplo:
```

build_final EX1 tools\EX1.tex

```

O script faz:
1) **gcc** compila `tools\tex2ce.c` → `tools\tex2ce.exe`  
2) `tools\tex2ce.exe` converte `.tex` → `tools\out.bin`  
3) `convbin` empacota `out.bin` → `tools\NOME_ARQ.8xv` (AppVar com nome interno = `NOME_ARQ`)  
4) `make` compila `src/main.c` → `bin\NOME_ARQ.8xp` com `-DDOC_NAME=NOME_ARQ`  
5) Copia `bin\NOME_ARQ.8xp` e `tools\NOME_ARQ.8xv` → `prontos\`  
6) **Limpa** `tools\NOME_ARQ.8xv` e `tools\out.bin`

> Sem o segundo argumento, o `.bat` tenta `tools\%NAME%.tex` e, se faltar, usa `tools\entrada.tex`.

### 3) Envie para a calculadora
Transfira **ambos** de `prontos\`:
- `NOME_ARQ.8xp` (programa)
- `NOME_ARQ.8xv` (AppVar com nome interno **NOME_ARQ**)

> **Regra TI**: nome de AppVar até **8 caracteres**, preferencialmente **MAIÚSCULO** e sem espaços.

### 4) Rode
- Abra `NOME_ARQ` (Cesium/PRGM).
- **Setas ↑/↓**: rolagem vertical (edge trigger).
- **ON**: sai instantaneamente.

---

## O que já está implementado

**Viewer (`src/main.c`)**
- Superscrito/Subscrito **inline** (SUP desce 1 px; SUB desce 6 px e aumenta altura de linha).
- **Frações reais** com medição e centralização (barra espessura `FRAC_BAR`), aninhamento suportado.
- **Wrap por palavras** para `TAG_TEXT` (evita “voltar e escrever por cima”).
- **Anti-flicker no topo**: linhas acima do topo são puladas integralmente.
- **Contexto de fração** (`g_in_frac`) para posicionamento de SUP dentro de frações.
- Rolagem estável e saída com ON latch.

**Conversor (`tools/tex2ce.c`)**
- **ASCII 7-bit** (qualquer char fora de 32..126 vira `?`).
- **Comandos suportados nativamente**: `\frac{A}{B}`, `^{...}`/`^X`, `_{...}`/`_X`, `\\`.
- **Aliases ASCII** (mapeados como texto plano):
  - `\rho`→`rho`, `\pi`→`pi`, `\varepsilon`/`\verepsilon`→`epsilon`
  - `\approx`/`\simeq`→`~=`
  - `\Rightarrow`→`=>`, `\rightarrow`/`\to`→`->`
  - `\left`/`\right`/`\Big`/`\big`→`` (descarta; parênteses reais ficam)
  - `\int`→`integral`, `\quad`→` ` (um espaço)
  - extras: `\cdot`→`*`, `\times`→`*`, `\leq`→`<=`, `\geq`→`>=`, `\neq`→`!=`, `\pm`→`+/-`
- `\ ` (barra + espaço) vira **um espaço**.
- Nova linha: 1 LF → quebra (`TAG_NL`); 2+ LFs → parágrafo (`TAG_PAR`).

---

## Dicas / Troubleshooting

- **“AppVar não encontrado”**: envie **.8xp + .8xv**; confirme que `-n` do convbin (nome interno) **bate** com `DOC_NAME` do `.8xp`. Máx. 8 caracteres.
- **Comandos LaTeX literais na tela**: use apenas o **subset** e os **aliases** listados; evite Unicode.
- **Linhas longas sobrepondo**: o wrap por palavras já evita; ainda assim, prefira quebrar com `\\` em pontos naturais.
- **Flicker no topo**: já mitigado (linhas acima do topo são puladas).
- **Apenas atualizar conteúdo**: se `DOC_NAME` não mudou, pode regerar só o `.8xv` (o `.8xp` já aponta para o mesmo nome).

