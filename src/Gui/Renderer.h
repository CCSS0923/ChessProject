#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <vector>
#include <opencv2/core.hpp>
#include "../ChessCore/Board.h"
#include "../ChessCore/GameLogic.h"
#include "../ChessCore/Piece.h"

// GuiManager_fwd.h 대체
struct MoveHint
{
    int x;
    int y;
};

struct MoveAnim
{
    bool active = false;
    bool isAnimating = false;
    int fromX = 0;
    int fromY = 0;
    int toX = 0;
    int toY = 0;
    double progress = 0.0;
    ULONGLONG startTick = 0; // [변경] DWORD -> ULONGLONG
    Piece movingPiece;
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void Initialize();
    // (x, y 인자 제거됨)
    void DrawBoard(HDC hdc,
        const Board& board,
        int tileSize,
        int selX, int selY, bool hasSelection,
        const std::vector<MoveHint>& hints,
        const MoveAnim& anim,
        bool dragging,
        int dragScreenX, int dragScreenY,
        const Piece& dragPiece);

private:
    std::map<std::wstring, cv::Mat> m_pieceImages;
    cv::Mat m_boardImage;

    void LoadPieceImages();
    void LoadBoardImage();
    void DrawWoodenTiles(HDC hdc, int tileSize);

    void DrawBoardTexture(HDC hdc,
        int tileSize,
        int offsetX, int offsetY);
    void DrawPieces(HDC hdc,
        const Board& board,
        int tileSize,
        const MoveAnim& anim,
        bool dragging,
        int dragScreenX, int dragScreenY,
        const Piece& dragPiece);
    void DrawSelectionAndHints(HDC hdc,
        int tileSize,
        int selX, int selY, bool hasSelection,
        const std::vector<MoveHint>& hints);

    HBITMAP MatToHBITMAP(const cv::Mat& src);
    void DrawMat(HDC hdc, int x, int y, const cv::Mat& mat, int width, int height);
};