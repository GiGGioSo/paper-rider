@echo off
SetLocal EnableDelayedExpansion

REM === CONFIGURAZIONE COMPILAZIONE ===
IF "%1"=="release" (
    SET CFLAGS=-O3 -Wall -Wextra -D_CRT_SECURE_NO_WARNINGS
) ELSE (
    SET CFLAGS=-ggdb -Wall -Wextra -D_CRT_SECURE_NO_WARNINGS
)

REM === PATH LIBRERIE E INCLUDE ===
SET LIBS=-LC:\dev\paper-rider\lib -lUser32 -lGdi32 -lShell32 -lopengl32 -lglfw3 -lmsvcrt
SET INCLUDES=-IC:\dev\paper-rider\include

SET EXE=.\bin\paper.exe

REM === RACCOLTA FILE SORGENTI .C ===
SET SRCS_FILES=
FOR /f %%f in ('dir /b /s src\*.c') do (
    SET SRCS_FILES=!SRCS_FILES! %%f
)

REM === PULIZIA E CREAZIONE CARTELLE ===
RMDIR /s /q bin 2>nul
MKDIR bin 

ECHO Compiling the following sources:
ECHO %SRCS_FILES%

REM === COMPILAZIONE ===
clang %SRCS_FILES% %CFLAGS% -std=c11 -o %EXE% %INCLUDES% %LIBS% -Wl,/NODEFAULTLIB:libcmt

IF %ERRORLEVEL% NEQ 0 (
    ECHO Build failed!
    EXIT /B %ERRORLEVEL%
)

ECHO Build succeeded!
