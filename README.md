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

