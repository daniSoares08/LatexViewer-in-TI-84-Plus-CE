# README — LatexViewer-in-TI-84-Plus-CE

Uma visão geral de **como usar, buildar e empacotar** o viewer + AppVar de conteúdo para a TI-84 Plus CE (inclui CE Python). Este README reflete **exatamente** as mudanças que aplicamos (sup/sub “dentro da linha”, culling, saída instantânea pelo ON, e nome do AppVar parametrizado via macro).

---

## O que é

* **Ferramentas no PC**

  * `tools/tex2ce.c` → conversor do seu **subset LaTeX** para **bytecode**.
  * `convbin` → empacota o bytecode em um **AppVar `.8xv`**.
* **App na calculadora**

  * `src/main.c` → programa `.8xp` (CEdev) que **lê o AppVar** e **desenha** (frações com barra, expoentes e subscritos, quebras de linha, rolagem, ON sai na hora).

**Você envia 2 arquivos para a calculadora por exercício:**
o **executável** (`.8xp`) e o **AppVar** (`.8xv`). Eles devem compartilhar **o mesmo nome** (ex.: `EX1LAMB.8xp` e `EX1LAMB.8xv`), pois o executável abre o AppVar homônimo.

---

## Estrutura do projeto

```
LatexViewer-in-TI-84-Plus-CE/
  src/
    main.c          # app da calculadora
  tools/
    tex2ce.c        # conversor PC
    tex2ce.exe      # (gerado por você no PC)
    *.tex           # seus textos LaTeX subset (entrada)
    *.8xv           # AppVars gerados (saída intermediária)
  prontos/          # SAÍDA FINAL: pares .8xp + .8xv para enviar à CE
  Makefile          # CEdev (gera o .8xp)
  build_final.bat   # script de automação (gera ambos e copia p/ prontos/)
```

---

## Pré-requisitos

* **CEdev** funcionando (inclui `make`, `convimg`, linker, etc.).
* **convbin** acessível no PATH.
* `tex2ce.exe` compilado a partir de `tools/tex2ce.c` (ou já presente).
* Windows (script `.bat`).

---

## Como usar (workflow típico)

### 1) Escreva o `.tex` (subset)

Coloque seu arquivo (ex.: `ex1.tex`) em `tools\`.

> O subset aceito está no **README-TEX** abaixo.

### 2) Gere os dois arquivos finais de uma vez

Na **raiz do projeto**, rode:

```
build_final EX1LAMB ex1.tex
```

Ele vai:

1. Converter `tools\ex1.tex` → `tools\out.bin` (bytecode).
2. Empacotar `tools\out.bin` → `tools\EX1LAMB.8xv` (AppVar com **nome interno EX1LAMB**).
3. Compilar o **executável** `.8xp` com `NAME=EX1LAMB` e definindo `DOC_NAME=EX1LAMB`.
4. Copiar **ambos** para `prontos\EX1LAMB.8xp` e `prontos\EX1LAMB.8xv`.

> Se você não informar o segundo argumento, o script tenta `tools\%NAME%.tex`, e se não existir, cai em `tools\entrada.tex`.

### 3) Envie para a calculadora

Transfira **os dois arquivos** de `prontos\`:

* `EX1LAMB.8xp` (programa)
* `EX1LAMB.8xv` (AppVar de dados, nome interno **EX1LAMB**)

> **Regra da TI para nomes de AppVar**: até **8 caracteres**, preferencialmente **maiúsculos** e sem espaços.

### 4) Rode

* Abra `EX1LAMB` (no Cesium/PRGM).
* **Setas ↑/↓**: rolagem vertical (trigger em “toque”, sem auto-repeat).
* **ON**: sai **instantaneamente** (sem travar).

---

## O que mudei no código (importante p/ manutenção)

1. **Nome do AppVar parametrizado**

   * No `main.c`:

     ```c
     #ifndef DOC_NAME
     #define DOC_NAME DOC1
     #endif
     #define _S2(x) #x
     #define _S1(x) _S2(x)
     #define DOC_NAME_STR _S1(DOC_NAME)
     ...
     ti_var_t v = ti_Open(DOC_NAME_STR, "r");
     ```
   * O build define `-DDOC_NAME=EX1LAMB` (sem aspas). O macro “stringify” vira `"EX1LAMB"`.

2. **Superscrito/Subscrito “dentro” da linha**

   * `SUP` desenhado em `y + 1px` (fora de fração) e `y + 1px` (dentro de fração), **sem clamp para 0** (acabou o bug de “grudar no topo”).
   * `SUB` desce `6px` e **aumenta** a altura de linha.

3. **Frações reais**

   * Numerador/denominador medidos, centralizados e barra horizontal com espessura `FRAC_BAR`.
   * Aninhamento de `\frac{}` suportado.

4. **Clipping e scroll estável**

   * Ignoro 2 frames (warm-up) para evitar scroll residual na abertura.
   * Culling por **linha** no loop principal + culling por **nó** dentro de sequências.

---

## Script de automação (o que ele faz)

`build_final.bat` (na raiz):

* Gera o AppVar com nome interno **= NAME**:

  ```bat
  pushd tools
  tex2ce "%SRC%" out.bin
  convbin -r -k 8xv -n %NAME% -i out.bin -o "%NAME%.8xv"
  popd
  ```
* Compila o `.8xp` apontando para o mesmo AppVar:

  ```bat
  make clean
  make NAME=%NAME% CFLAGS="-Wall -Wextra -Oz -DDOC_NAME=%NAME%"
  ```
* Copia para **`prontos\`**:

  ```bat
  copy /Y "bin\%NAME%.8xp" "prontos\%NAME%.8xp"
  copy /Y "tools\%NAME%.8xv" "prontos\%NAME%.8xv"
  ```

> Se quiser compilar **vários** de uma vez, basta chamar repetidamente:
>
> ```
> build_final EX1LAMB ex1.tex
> build_final EX2CAMPO ex2.tex
> build_final EX3POT   ex3.tex
> ...
> ```

---

## Dicas / Troubleshooting

* **“AppVar não encontrado” logo ao abrir**

  * Confirme que você enviou **os dois arquivos**.
  * O **nome interno** do AppVar (passado via `-n` no `convbin`) **deve bater** com o `DOC_NAME` do executável.
  * Lembre da regra dos **8 caracteres**.

* **Acentos / símbolos esquisitos**

  * Use **ASCII puro** no `.tex`. O conversor não emite Unicode.

* **Flicker / linhas colando no topo**

  * Já resolvido com o patch de `SUP` e o warm-up de teclado.

* **Frações colidindo com outras linhas**

  * Corrigido (SUP dentro da linha e clipping por linha).
  * Evite colocar `\\` **dentro** de `\frac{...}{...}` (não há quebra interna em frações).

* **`seq_stats` warn “unused”**

  * Pode remover a função ou deixar (warning inofensivo).

* **Só atualizar o conteúdo, sem recompilar o .8xp**

  * Se o executável já aponta para `DOC1`, você pode **regerar apenas** o `DOC1.8xv`.
  * Em builds nomeados (EX1/EX2…), normalmente você mantém os pares sincronizados pelo `build_final`.

---

## Para futuras edições comigo (coisas que vou lembrar)

* O viewer usa **topo de linha** como referência (não baseline).
* `SUP`/`SUB` **não** mudam largura; `SUB` **aumenta** a altura da linha; `SUP` **não**.
* `FRAC`: mede numerador/denominador, define altura `hn + gaps + bar + hd`.
* **Rolagem**: setas ↑/↓ por “toque” (edge trigger), sem auto-repeat.
* **ON**: `kb_EnableOnLatch()` + latch clean + `gfx_End()` → **sai instantaneamente**.
* **Limite de largura** ≈ `right = 312` px; wrap automático quando ultrapassa.
* **Nome do AppVar**: `DOC_NAME` como **token** (sem aspas) e `_S1/_S2` para “stringify”.

---

