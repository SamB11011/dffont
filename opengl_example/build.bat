@echo off

set GLFW_DIR=C:\projects\dependencies\msvc\glfw_3.3.4
set GLFW3_DLL=%GLFW_DIR%\lib\glfw3.dll
set GLFW3_LIB=%GLFW_DIR%\lib\glfw3dll.lib
set GLFW3_INC=%GLFW_DIR%\include

set GLAD_DIR=.\src\glad
set GLAD_SRC=%GLAD_DIR%\glad.c
set GLAD_INC=%GLAD_DIR%\include

set SRC=.\src\*.c
set EXE_NAME=example.exe
set DEBUG_PARAMS=

if not exist .\build (
    mkdir .\build
)

if not exist .\build\%GLFW3_DLL% (
    copy %GLFW3_DLL% .\build
)

cl ^
    /Fe.\build\%EXE_NAME% ^
    %GLAD_SRC% %SRC% ^
    /I%GLFW3_INC% /I%GLAD_INC% ^
    /link /libpath %GLFW3_LIB%

if %errorlevel%==0 (
    del *.obj
)
