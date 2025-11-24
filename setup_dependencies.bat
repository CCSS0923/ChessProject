@echo off
echo Downloading dependencies...

REM Stockfish
curl -L -o stockfish.zip https://stockfishchess.org/files/stockfish-17-win-x86-64.zip
tar -xf stockfish.zip -C extern/stockfish

REM OpenCV
curl -L -o opencv.zip https://github.com/opencv/opencv/releases/download/4.12.0/opencv-4.12.0-windows.zip
tar -xf opencv.zip -C extern/opencv

echo Done.
pause
