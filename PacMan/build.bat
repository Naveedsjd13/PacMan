@echo off
echo Building PAC-MAN - Parallel Edition (MPI + OpenMP)...
echo.

:: We use mpic++ instead of g++ to handle MPI dependencies
:: We keep -fopenmp for the Ghost AI threading
mpic++ -std=c++20 main.cpp -o pacman.exe -fopenmp -lsfml-graphics -lsfml-window -lsfml-system -O2

if %errorlevel% == 0 (
    echo.
    echo ========================================
    echo  Build successful!
    echo ========================================
    echo.
    echo NOTE: Since this uses MPI, do NOT just run pacman.exe.
    echo Run the game with: mpiexec -n 2 pacman.exe
    echo.
    echo Make sure these files are in the same folder:
    echo   - pacman.png
    echo   - ghost.png
    echo   - sfml-graphics-3.dll
    echo   - sfml-window-3.dll
    echo   - sfml-system-3.dll
    echo   - msmpi.dll (or libmpi.dll)
    echo   - libgomp-1.dll (OpenMP support)
    echo.
    echo MPI + OpenMP ENABLED!
    echo Rank 0: Display | Rank 1: Ghost AI (Multi-threaded)
    echo.
) else (
    echo.
    echo ========================================
    echo  Build failed!
    echo ========================================
    echo.
    echo Common fixes:
    echo   1. Use the "MSYS2 MinGW 64-bit" terminal to run this script.
    echo   2. Ensure MPI is installed: pacman -S mingw-w64-x86_64-msmpi
    echo   3. Ensure SFML is installed: pacman -S mingw-w64-x86_64-sfml
    echo.
)

pause