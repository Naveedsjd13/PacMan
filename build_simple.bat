@echo off
echo Building ParaMan - MPI + OpenMP Edition (no CUDA)...
echo.

set MSMPI_INC=C:\Program Files (x86)\Microsoft SDKs\MPI\Include
set MSMPI_LIB=C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64

echo Compiling with g++...
g++ -std=c++20 main.cpp -o pacman.exe ^
    -fopenmp ^
    -I"%MSMPI_INC%" ^
    -L"%MSMPI_LIB%" ^
    -lmsmpi ^
    -lsfml-graphics -lsfml-window -lsfml-system ^
    -O2

if not %errorlevel% == 0 goto FAILED

echo.
echo ========================================
echo  Build successful!
echo ========================================
echo Run with: mpiexec -n 2 pacman.exe
echo.
goto END

:FAILED
echo Build failed!

:END
pause