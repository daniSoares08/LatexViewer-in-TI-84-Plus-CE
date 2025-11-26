READMETEX – LaTeX subset for the TI-84 Plus CE viewer

## ENGLISH VERSION

This text describes the small LaTeX-like language that the program “tex2ce” understands.
The goal is to take a .tex file, convert it on the PC, and show the result on the TI-84 Plus CE using the calculator’s own font (including accents and Greek letters).

1. Character set and encoding

The .tex file must be saved in UTF-8.
The converter accepts:

* normal ASCII characters (letters, digits, punctuation);
* a subset of accented letters that exist in the TI character set: Á À Â Ä á à â ä É È Ê Ë é è ê ë Í Ì Î Ï í ì Ó Ò Ô Ö ó ò ô ö Ú Ù Û Ü ú ù Ç ç;
* a subset of Greek letters that also exist in the TI character set: α β γ δ ε θ λ μ π ρ Σ σ τ φ Ω.

Letters like ã, õ, Ã, Õ do not exist in the calculator’s font; they are automatically simplified to a or o.

You can write words in Portuguese directly, for example “ação”, “média”, “função”, as long as the letters are in the list above.

2. Supported LaTeX structures

The converter recognizes only a very small subset of LaTeX:

* Normal text: anything that is not a command and is not inside special structures is treated as plain text and rendered on a single baseline.
* Fractions: the syntax is “\frac{NUM}{DEN}”. NUM and DEN are blocks that may contain text, superscripts and subscripts, and even other fractions, as long as the braces are balanced.
* Superscript: “^{...}” or “^X”. Without braces, only the next single character is taken as the superscript.
* Subscript: “_{...}” or “_X”. Without braces, only the next single character is taken as the subscript.
* Line break: “\” forces an immediate line break.
* New paragraph: a blank line in the .tex file (two or more consecutive line breaks) starts a new paragraph with extra vertical space.

Nested expressions such as “\frac{a^2}{b_c}” or “\frac{\frac{1}{2}x^2}{3y_1}” are allowed.

3. Things that are not supported

The converter does not implement square roots, sums, integrals as true math operators, environments, alignment, images or anything complex from full LaTeX.

Only the commands listed in the “alias” table below are recognized specially. Any other command (for example “\sqrt”, “\begin{...}”, “\sum”) will appear literally as text, including the backslash.

Line breaks are not allowed inside the two arguments of “\frac{NUM}{DEN}”. The converter does not wrap the content of a fraction internally; all wrapping happens at the outer text level.

4. Command aliases and Greek letters

Some LaTeX commands are automatically replaced by a single Unicode character (which is then converted to the calculator’s internal code) or by a short ASCII sequence. The current mapping is:

* \alpha → α

* \beta → β

* \gamma → γ

* \delta → δ

* \epsilon → ε

* \theta → θ

* \lambda → λ

* \mu → μ

* \pi → π

* \rho → ρ

* \sigma → σ

* \Sigma → Σ

* \tau → τ

* \phi → φ

* \Omega → Ω

* \varepsilon and \verepsilon → ε

* \approx and \simeq → ~=

* \Rightarrow → =>

* \rightarrow and \to → ->

* \Ohm → Ω

* \left, \right, \Big, \big → removed (you should just type normal parentheses)

* \int → the word “integral” (there is no special integral symbol yet)

* \quad → one normal space

* \cdot → *

* \times → *

* \leq → <=

* \geq → >=

* \neq → !=

* \pm → +/-

A backslash followed by a space (“\ ”) turns into a single space.

You can either use these commands or write the words and symbols directly. For example, you can write “α” in the .tex file (UTF-8) or you can write “\alpha”; both result in the same character on the calculator.

5. Practical writing rules

a) Encoding
Always save the .tex file as UTF-8. If you copy and paste Greek letters or accented words, they must remain valid UTF-8 characters.

b) Fractions
Use exactly the syntax “\frac{NUM}{DEN}” with both pairs of braces present. Leaving out a brace makes the parser consume too much text and usually breaks the rest of the document. Do not put manual line breaks (“\”) or blank lines inside NUM or DEN.

c) Superscripts and subscripts
Write them as you would in normal LaTeX: “x^2”, “x^{10}”, “y_1”, “A_{k+1}”.
Without braces, only one character becomes part of the exponent or subscript.

d) Line breaks and paragraphs
Use “\” to force a line break in the same paragraph.
Leave a blank line in the .tex file to start a new paragraph with extra spacing. There is no special math mode: everything is treated as inline text (you do not need “$...$”).

e) Layout on the calculator
The viewer renders text using the TI-OS font and performs word wrapping when the available width (about 312 pixels between margins) is exceeded.

Even with wrapping, it is better to keep formulas and sentences relatively short and to break the line manually with “\” at convenient points, especially if you have deeply nested fractions.

f) Scientific notation
Use normal floating notation like “3.0e-6” or “8.99e9”.

6. Examples (written form)

Simple superscript and subscript:

This is a test: x^2 + y_1.
Inline: E = mc^2, A_k = \frac{k(k+1)}{2}.

Fractions and nesting:

A fraction: \frac{a+b}{c+d}
Nested: \frac{\frac{1}{2}x^2}{3y_1}

Line breaks and paragraphs:

Manual break with backslash here:
line A \ line B after the break.

Blank line above creates a new paragraph:

Sum of the first n integers:
S_n = \frac{n(n+1)}{2}

7. Quick pipeline test

In the repository root, run a command similar to:

build_final EX1TEST tools\ex1.tex

If everything works, you should obtain two files:

prontos\EX1TEST.8xp
prontos\EX1TEST.8xv

Send both files to the TI-84 Plus CE and run the program EX1TEST.

8. Common problems

If a brace is missing in a fraction, the converter may read until the end of the file as part of NUM or DEN and the document will render incorrectly.

Spaces are preserved exactly. Many consecutive spaces produce a lot of horizontal empty space.

Very long lines will be wrapped, but it is better to control breaks manually with “\”.

Commands that are not in the alias list above will show up literally as text (including the backslash). If you see a command printed on the screen instead of behaving like math, it is probably outside the supported subset.

## VERSÃO EM PORTUGUÊS

Este texto explica o pequeno “dialeto LaTeX” que o programa “tex2ce” entende.
A ideia é escrever um arquivo .tex no computador, converter com o tex2ce e depois visualizar o resultado na TI-84 Plus CE, usando a fonte oficial da calculadora (com acentos e letras gregas).

1. Conjunto de caracteres e codificação

O arquivo .tex deve ser salvo em UTF-8.
O conversor aceita:

* caracteres ASCII normais (letras, números, pontuação);
* um subconjunto de letras acentuadas que existem na fonte da TI: Á À Â Ä á à â ä É È Ê Ë é è ê ë Í Ì Î Ï í ì Ó Ò Ô Ö ó ò ô ö Ú Ù Û Ü ú ù Ç ç;
* um subconjunto de letras gregas disponíveis no sistema: α β γ δ ε θ λ μ π ρ Σ σ τ φ Ω.

Não existem ã, õ, Ã, Õ na fonte da calculadora; esses caracteres são simplificados para “a” ou “o”.

Você pode escrever palavras em português diretamente, por exemplo “ação”, “média”, “função”, desde que os acentos estejam na lista suportada.

2. Estruturas LaTeX suportadas

O conversor reconhece apenas um subconjunto bem simples:

* Texto normal: tudo que não for comando especial é tratado como texto e impresso numa única linha base.
* Frações: sintaxe “\frac{NUM}{DEN}”. NUM e DEN podem ter texto, expoentes, subscritos e até outras frações, desde que as chaves estejam balanceadas.
* Expoente (superscript): “^{...}” ou “^X”. Sem chaves, apenas o próximo caractere entra no expoente.
* Subscrito: “_{...}” ou “_X”. Sem chaves, apenas o próximo caractere entra no subscrito.
* Quebra de linha: “\” força uma quebra de linha imediata.
* Novo parágrafo: uma linha em branco no arquivo (duas ou mais quebras de linha consecutivas) cria um parágrafo novo com espaço extra.

É permitido aninhar expressões como “\frac{a^2}{b_c}” e “\frac{\frac{1}{2}x^2}{3y_1}”.

3. O que não é suportado

O conversor não implementa raiz, somatório, integrais como operadores gráficos, ambientes, alinhamento, figuras, etc.

Qualquer comando que não esteja na tabela de aliases é impresso literalmente, incluindo a barra invertida; por exemplo, “\sqrt” aparece como texto “\sqrt”.

Dentro de “\frac{NUM}{DEN}” não é permitido colocar quebras “\” nem linhas em branco. O conteúdo da fração não sofre word wrap interno; a quebra de linha é feita apenas no nível externo do texto.

4. Aliases de comandos e letras gregas

Alguns comandos LaTeX são substituídos automaticamente por um único caractere Unicode (que depois vira o código interno da TI) ou por um pequeno trecho ASCII. Os mapeamentos atuais são:

\alpha → α
\beta → β
\gamma → γ
\delta → δ
\epsilon → ε
\theta → θ
\lambda → λ
\mu → μ
\pi → π
\rho → ρ
\sigma → σ
\Sigma → Σ
\tau → τ
\phi → φ
\Omega → Ω
\varepsilon e \verepsilon → ε

\approx e \simeq → ~=
\Rightarrow → =>
\rightarrow e \to → ->
\Ohm → Ω

\left, \right, \Big, \big → descartados (use parênteses normais)
\int → a palavra “integral”
\quad → um espaço simples

\cdot → *
\times → *
\leq → <=
\geq → >=
\neq → !=
\pm → +/-

Uma barra invertida seguida de espaço (“\ ”) vira apenas um espaço.

Você pode usar esses comandos ou escrever diretamente as letras e símbolos, desde que o arquivo esteja em UTF-8. Por exemplo, tanto “α” quanto “\alpha” geram o mesmo símbolo na tela da calculadora.

5. Regras práticas de escrita

a) Codificação
Sempre salve o .tex como UTF-8. Copiar e colar letras gregas ou palavras acentuadas só funciona se o editor mantiver esses caracteres em UTF-8.

b) Frações
Use sempre “\frac{NUM}{DEN}” com as duas chaves abrindo e fechando. Se faltar uma chave, o parser pode engolir o resto do arquivo como parte da fração e estragar tudo. Não use “\” nem linhas em branco dentro de NUM ou DEN.

c) Expoentes e subscritos
Escreva como no LaTeX normal: “x^2”, “x^{10}”, “y_1”, “A_{k+1}”.
Sem chaves, só um caractere entra no expoente ou subscrito.

d) Quebras de linha e parágrafos
“\” faz uma quebra imediata de linha.
Uma linha vazia no arquivo .tex cria um novo parágrafo, com linha extra de espaço. Não existe modo matemático separado: tudo é tratado como texto inline, não é necessário usar “$...$”.

e) Layout na calculadora
O viewer usa a fonte do sistema da TI e faz quebra de linha automática por palavras quando a largura disponível (cerca de 312 pixels entre margens) acaba.

Mesmo assim, é melhor manter frases e fórmulas relativamente curtas e inserir quebras manuais com “\” em pontos naturais, principalmente quando houver frações aninhadas.

f) Notação científica
Use forma compacta, como “3.0e-6” e “8.99e9”.

6. Exemplos (forma escrita)

Superscrito e subscrito simples:

Este é um teste: x^2 + y_1.
Inline: E = mc^2, A_k = \frac{k(k+1)}{2}.

Frações e aninhamento:

Uma fração: \frac{a+b}{c+d}
Aninhada: \frac{\frac{1}{2}x^2}{3y_1}

Quebras e parágrafos:

Quebra manual com barra aqui:
linha A \ linha B após a quebra.

Linha vazia acima cria um novo parágrafo:

Soma dos primeiros n inteiros:
S_n = \frac{n(n+1)}{2}

7. Teste rápido do pipeline

Na raiz do repositório, execute algo como:

build_final EX1TEST tools\ex1.tex

A saída esperada é:

prontos\EX1TEST.8xp
prontos\EX1TEST.8xv

Envie os dois arquivos para a TI-84 Plus CE e execute o programa EX1TEST.

8. Problemas comuns

Falta de chaves em “\frac{...}{...}” quase sempre quebra o documento, porque o conversor não sabe onde termina a fração.

Espaços são preservados exatamente. Uma sequência longa de espaços cria um grande “buraco” horizontal.

Linhas muito longas são quebradas automaticamente, mas é melhor controlar o layout com quebras “\” para manter o texto legível.

Comandos que não fazem parte do subset aqui descrito aparecem como texto literal, o que é um bom indicativo de que aquela construção não é suportada e precisa ser simplificada.
