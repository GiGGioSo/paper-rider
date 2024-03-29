@echo off
SetLocal EnableDelayedExpansion

IF "%1"=="release" (
    SET CFLAGS=-O3 -Wall -Wextra
) ELSE (
    SET CFLAGS=-ggdb -Wall -Wextra
)
SET LIBS=-LC:\dev\paper-rider\lib -lUser32.lib -lGdi32.lib -lShell32.lib -lmsvcrt.lib -lopengl32.lib -lglfw3.lib
SET INCLUDES=-IC:\dev\paper-rider\include
SET PREPROCESSOR_DEFINITIONS=-D _CRT_SECURE_NO_WARNINGS

SET EXE=.\bin\paper.exe

SET SRCS_FILES=
FOR /f %%f in ('dir /b /s src\*.c src\*.cpp') do (
    SET SRCS_FILES=!SRCS_FILES! %%f
)

RMDIR /s /q bin
MKDIR bin 

ECHO Compiling the following sources:
ECHO %SRCS_FILES%

clang++ %PREPROCESSOR_DEFINITIONS% %SRCS_FILES% %CFLAGS% -o %EXE% %INCLUDES% %LIBS%
