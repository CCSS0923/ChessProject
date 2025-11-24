@echo off
echo ===== Downloading dependencies =====

REM ---------- prepare folders ----------
if not exist extern mkdir extern
if not exist extern\stockfish mkdir extern\stockfish
if not exist extern\opencv mkdir extern\opencv

REM ---------- Stockfish ----------
set STOCK_URL=https://github.com/official-stockfish/Stockfish/releases/latest/download/stockfish-windows-x86-64-avx2.zip
set STOCK_ZIP=stockfish.zip

echo [1/2] Downloading Stockfish...
curl -L -o "%STOCK_ZIP%" "%STOCK_URL%"

echo Extracting Stockfish...
powershell -command "Expand-Archive -Force '%STOCK_ZIP%' 'extern'"
del "%STOCK_ZIP%"

REM ---------- OpenCV ----------
set OPENCV_URL=https://github.com/opencv/opencv/releases/download/4.12.0/opencv-4.12.0-windows.exe
set OPENCV_EXE=opencv.exe

echo [2/2] Downloading OpenCV...
curl -L -o "%OPENCV_EXE%" "%OPENCV_URL%"

echo Extracting OpenCV...
"%OPENCV_EXE%" -o extern/opencv

del "%OPENCV_EXE%"

echo.
echo ===== DONE =====
pause
