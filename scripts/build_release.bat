@echo off
echo Building Openterface QT in Release mode...

REM Navigate to project root
cd /d "%~dp0\.."

REM Clean and rebuild
qmake6 -makefile -o build\Desktop_Qt_6_5_3_MinGW_64_bit-Release\Makefile openterfaceQT.pro CONFIG+=release

REM Navigate to build directory
cd build\Desktop_Qt_6_5_3_MinGW_64_bit-Release

REM Build the project
jom || mingw32-make || nmake || make

echo Build complete.
pause
