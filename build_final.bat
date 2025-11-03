@echo off
setlocal EnableDelayedExpansion
REM -------------------------------------------------------------
REM Build final: gera .8xv e .8xp e copia para prontos\ com o mesmo NAME
REM Uso: build_final NAME [texfile]
REM Ex.: build_final EX1LAMB ex1.tex
REM -------------------------------------------------------------

if "%~1"=="" (
  echo Uso: build_final NAME [texfile]
  exit /b 1
)

set "NAME=%~1"
set "SRC=%~2"

if not exist "prontos" mkdir "prontos"

REM ---------- etapa 0: compilar o conversor ----------
echo [0/6] gcc -O2 tools\tex2ce.c -o tools\tex2ce.exe
gcc -O2 tools\tex2ce.c -o tools\tex2ce.exe || (echo [ERRO] gcc falhou ao compilar tex2ce.c & exit /b 1)

REM ---------- gera AppVar ----------
pushd tools

if "%SRC%"=="" (
  if exist "%NAME%.tex" (
    set "SRC=%NAME%.tex"
  ) else if exist "entrada.tex" (
    set "SRC=entrada.tex"
  ) else (
    echo [ERRO] Nao encontrei tools\%NAME%.tex nem tools\entrada.tex
    popd & exit /b 1
  )
)
if not exist "%SRC%" (
  echo [ERRO] tools\%SRC% nao encontrado.
  popd & exit /b 1
)

echo [1/6] tex2ce "%SRC%" -> out.bin
.\tex2ce "%SRC%" out.bin || (echo [ERRO] tex2ce falhou & popd & exit /b 1)

echo [2/6] convbin -> %NAME%.8xv  (AppVar interna "%NAME%")
convbin -r -k 8xv -n %NAME% -i out.bin -o "%NAME%.8xv" || (echo [ERRO] convbin falhou & popd & exit /b 1)

popd

echo [3/6] make clean
make clean

echo [4/6] make NAME=%NAME%  (DOC_NAME=%NAME%)
REM NOTE: sem aspas; no codigo usamos stringify para virar "EX1LAMB"
make NAME=%NAME% CFLAGS="-Wall -Wextra -Oz -DDOC_NAME=%NAME%" || (echo [ERRO] make falhou & exit /b 1)

echo [5/6] copiando para prontos\
copy /Y "bin\%NAME%.8xp" "prontos\%NAME%.8xp" >nul || (echo [ERRO] nao achei bin\%NAME%.8xp & exit /b 1)
copy /Y "tools\%NAME%.8xv" "prontos\%NAME%.8xv" >nul || (echo [ERRO] nao achei tools\%NAME%.8xv & exit /b 1)

echo [6/6] limpando intermediarios em tools\
del /Q "tools\%NAME%.8xv" >nul 2>nul
del /Q "tools\out.bin"    >nul 2>nul

echo.
echo [OK] prontos\%NAME%.8xp  e  prontos\%NAME%.8xv gerados.
echo Envie os dois para a calculadora.
endlocal
