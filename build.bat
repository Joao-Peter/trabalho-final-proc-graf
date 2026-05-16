@echo OFF

if "%1"=="" goto erro1
if "%2"=="" goto erro2

g++ %1 stb_image.cpp gl_utils.cpp glfw3dll.a -I ".\include" -L "." -L ".\lib" -lOpenGL32 -lglew32 -lglfw3 -lm -o %2

goto end

:erro1
echo Informe o nome do arquivo a ser compilado
goto end

:erro2
echo Informe o nome do arquivo de saida
goto end

:end