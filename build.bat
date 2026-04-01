@echo off
echo Building PAC-MAN - Parallel Edition (OpenMP)...
echo.

g++ -std=c++20 main.cpp -o pacman.exe -fopenmp -lsfml-graphics -lsfml-window -lsfml-system -O2

if %errorlevel% == 0 (
    echo.
    echo ========================================
    echo  Build successful!
    echo ========================================
    echo.
    echo Run the game with: pacman.exe
    echo.
    echo Make sure these files are in the same folder:
    echo   - pacman.png
    echo   - ghost.png
    echo   - sfml-graphics-3.dll
    echo   - sfml-window-3.dll
    echo   - sfml-system-3.dll
    echo   - openmp dll (libgomp-1.dll if using MinGW)
    echo.
    echo OpenMP is ENABLED - ghosts update in parallel!
    echo.
) else (
    echo.
    echo ========================================
    echo  Build failed!
    echo ========================================
    echo.
    echo Common fixes:
    echo   1. Make sure SFML is installed and in your PATH
    echo   2. Make sure your g++ supports OpenMP (-fopenmp)
    echo      Test with: g++ --version
    echo   3. MinGW users: use MinGW with libgomp support
    echo.
)

pause