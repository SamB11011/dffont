@echo off
if not exist .\build (
    mkdir .\build
)
gcc -Wall -o./build/dffont -I./src/truety -I./src/stb ./src/*.c ./src/truety/truety.c
rem cl /Zi .\src\*.c .\src\truety\truety.c /I.\src\truety /I.\src\stb