#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <future>
#include "../ChessCore/Board.h"
#include "../ChessCore/GameLogic.h"
#include "../Engine/Stockfish.h"
#include "Renderer.h"

#define TIMER_ANIM 1

class GuiManager
{
public:
    GuiManager();
    ~GuiManager();

    void Initialize(HWND hWnd);
    void InitGame();
    void Redraw();

    void OnLButtonDown(int x, int y, bool withShift);
    void OnLButtonUp(int x, int y);
    void OnMouseMove(int x, int y, bool leftDown);
    void OnPaint(HDC hdc);
    void OnTimer(UINT id);
    void OnSize(int width, int height);
    void OnKeyDown(UINT nChar);
    void UndoMove();

private:
    HWND        m_hWnd = nullptr;
    Renderer    m_renderer;
    GameLogic   m_gameLogic;
    Board       m_board;
    StockfishEngine m_engine;

    int m_tileSize = 80;
    bool m_pieceSelected = false;
    int  m_selX = -1;
    int  m_selY = -1;
    bool m_dragging = false;
    int  m_dragX = 0;
    int  m_dragY = 0;
    int  m_dragScreenX = 0;
    int  m_dragScreenY = 0;
    Piece m_dragPiece;

    bool m_isWhiteTurn = true;
    bool m_whiteIsHuman = true;
    bool m_blackIsAI = true;

    std::future<std::string> m_aiFuture;
    bool m_isAIThinking = false;

    std::vector<MoveHint> m_moveHints;
    MoveAnim m_anim;

    // [추가] 승급 선택 UI 관련
    bool m_isPromoting = false; // 승급 선택 중인가?
    Move m_pendingPromotionMove; // 승급 대기 중인 이동 정보

private:
    void HandlePlayerClick(int x, int y, bool withShift);
    void StartAnimation(int sx, int sy, int dx, int dy, const Piece& p);
    void UpdateAnimation();
    void UpdateMoveHints(int selX, int selY);

    void CheckAIState();
    void RequestAIMove();

    // [추가] 게임 상태 확인 및 종료 처리
    void CheckAndHandleGameOver();
    // [추가] 승급 UI 그리기 및 입력 처리
    void DrawPromotionMenu(HDC hdc);
    void HandlePromotionClick(int x, int y);
};