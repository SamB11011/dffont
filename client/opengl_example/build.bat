@echo off

set GLFW_DIR=C:\projects\libraries\glfw_3.3.4
set GLFW3_DLL=%GLFW_DIR%\lib\glfw3.dll
set GLFW3_LIB=%GLFW_DIR%\lib\glfw3dll.lib
set GLFW3_INC=%GLFW_DIR%\include

set GLAD_DIR=.\src\glad
set GLAD_SRC=%GLAD_DIR%\glad.c
set GLAD_INC=%GLAD_DIR%\include

set STB_INC=.\src

set SRC=.\src\*.c ..\src\dffont_client.c
set EXE_NAME=example.exe

if not exist glfw3.dll (
    copy %GLFW3_DLL% .\
)

cl ^
    /Fe%EXE_NAME% ^
    %GLAD_SRC% %SRC% ^
    /I%GLFW3_INC% /I%GLAD_INC% /I%STB_INC% ^
    /link /libpath %GLFW3_LIB%

if exist *.obj (
    del *.obj
)
