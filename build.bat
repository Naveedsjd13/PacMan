@echo off
echo Building ParaMan - Static CUDA Edition...
echo.

set MSMPI_INC=C:\Program Files (x86)\Microsoft SDKs\MPI\Include
set MSMPI_LIB=C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64
set CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.2

call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"

REM ── Compile CUDA into object file ─────────────────────────────────────────
echo [Step 1/3] Compiling CUDA kernel...
nvcc -c GhostPathfinding.cu -o GhostPathfinding.obj ^
    -arch=sm_75 ^
    -I"%MSMPI_INC%"

if not %errorlevel% == 0 goto CUDA_FAILED
echo [Step 1/3] CUDA object OK!
echo.

REM ── Create static library from object ─────────────────────────────────────
echo [Step 2/3] Creating static library...
lib /OUT:GhostPathfinding.lib GhostPathfinding.obj

if not %errorlevel% == 0 goto LIB_FAILED
echo [Step 2/3] Static lib OK!
echo.

REM ── Compile main.cpp with g++ linking static CUDA lib ─────────────────────
echo [Step 3/3] Compiling main.cpp...
g++ -std=c++20 main.cpp -o pacman.exe ^
    -fopenmp ^
    -I"%MSMPI_INC%" ^
    -I"%CUDA_PATH%\include" ^
    -L"%MSMPI_LIB%" ^
    -L"%CUDA_PATH%\lib\x64" ^
    -L"." ^
    -lmsmpi ^
    -lcudart ^
    -lGhostPathfinding ^
    -lsfml-graphics -lsfml-window -lsfml-system ^
    -O2

if not %errorlevel% == 0 goto LINK_FAILED

echo.
echo ========================================
echo  Build successful!
echo ========================================
goto END

:CUDA_FAILED
echo [CUDA] Compile failed!
goto END

:LIB_FAILED
echo [LIB] Failed to create static library!
goto END

:LINK_FAILED
echo [LINK] Failed!
goto END

:END
pause