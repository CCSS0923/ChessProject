#include "GuiManager.h"
#include "../Utils/Logger.h"
#include "../ChessCore/Fen.h"

GuiManager::GuiManager() {}

GuiManager::~GuiManager() {
    m_engine.Shutdown();
}

void GuiManager::Initialize(HWND hWnd)
{
    m_hWnd = hWnd;

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    OnSize(rc.right - rc.left, rc.bottom - rc.top);

    m_board.ResetToStartPosition();
    m_renderer.Initialize();

    if (!m_engine.Initialize(L"../extern/stockfish/stockfish-windows-x86-64-avx2.exe")) {
        Log(L"Stockfish 초기화 실패");
    }

    InitGame();
    SetTimer(m_hWnd, TIMER_ANIM, 16, nullptr);
}

void GuiManager::InitGame()
{
    m_isWhiteTurn = true;
    m_pieceSelected = false;
    m_dragging = false;
    m_moveHints.clear();
    m_anim.active = false;
    m_isAIThinking = false;
    m_isPromoting = false;
}

void GuiManager::OnSize(int width, int height)
{
    int minDim = (width < height) ? width : height;
    m_tileSize = (int)((minDim * 0.9) / 8.0);
    if (m_tileSize < 10) m_tileSize = 10;
    Redraw();
}

void GuiManager::Redraw()
{
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void GuiManager::UndoMove()
{
    if (m_isAIThinking || m_isPromoting) return;

    if (m_board.PopState()) {
        m_isWhiteTurn = !m_isWhiteTurn;
        m_pieceSelected = false;
        m_moveHints.clear();

        if (m_blackIsAI && m_isWhiteTurn == false) {
            if (m_board.PopState()) m_isWhiteTurn = !m_isWhiteTurn;
        }
        Redraw();
    }
}

void GuiManager::OnKeyDown(UINT nChar)
{
    if (nChar == VK_BACK) UndoMove();
}

void GuiManager::OnPaint(HDC hdc)
{
    RECT rc;
    GetClientRect(m_hWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
    HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

    HBRUSH bg = CreateSolidBrush(RGB(40, 30, 20));
    FillRect(memDC, &rc, bg);
    DeleteObject(bg);

    m_renderer.DrawBoard(memDC, m_board, m_tileSize,
        m_selX, m_selY, m_pieceSelected,
        m_moveHints, m_anim, m_dragging,
        m_dragScreenX, m_dragScreenY, m_dragPiece);

    // [승급 메뉴 그리기]
    if (m_isPromoting) {
        DrawPromotionMenu(memDC);
    }

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

void GuiManager::DrawPromotionMenu(HDC hdc)
{
    RECT rc;
    GetClientRect(m_hWnd, &rc);
    int cx = (rc.right - rc.left) / 2;
    int cy = (rc.bottom - rc.top) / 2;
    int w = 300, h = 100;

    // 배경 박스
    HBRUSH boxBrush = CreateSolidBrush(RGB(255, 255, 255));
    RECT boxRc = { cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2 };
    FillRect(hdc, &boxRc, boxBrush);
    DeleteObject(boxBrush);

    // 테두리
    FrameRect(hdc, &boxRc, (HBRUSH)GetStockObject(BLACK_BRUSH));

    // 버튼 텍스트 (Q, R, B, N)
    SetBkMode(hdc, TRANSPARENT);

    // [수정] TA_VCENTER 제거 (Win32에 존재하지 않음) -> TA_CENTER만 사용
    SetTextAlign(hdc, TA_CENTER);

    const wchar_t* labels[] = { L"Queen", L"Rook", L"Bishop", L"Knight" };

    // 폰트 높이 대략 계산 (기본 시스템 폰트 기준)하여 수직 중앙 정렬 보정
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    int textY = cy - (tm.tmHeight / 2); // 텍스트 시작 Y좌표 조정

    for (int i = 0; i < 4; ++i) {
        int bx = (cx - w / 2) + i * (w / 4);

        // 가로 중앙(bx + w/8)에 텍스트 출력
        TextOutW(hdc, bx + w / 8, textY, labels[i], lstrlenW(labels[i]));

        // 구분선
        if (i > 0) {
            MoveToEx(hdc, bx, cy - h / 2, nullptr);
            LineTo(hdc, bx, cy + h / 2);
        }
    }
}

void GuiManager::OnLButtonDown(int x, int y, bool withShift)
{
    // 승급 중이면 승급 메뉴 클릭 처리
    if (m_isPromoting) {
        HandlePromotionClick(x, y);
        return;
    }

    if (m_isAIThinking) return;

    int offset = m_tileSize / 3;
    int boardX = (x - offset) / m_tileSize;
    int boardY = (y - offset) / m_tileSize;

    if (boardX < 0 || boardX >= 8 || boardY < 0 || boardY >= 8)
        return;

    HandlePlayerClick(boardX, boardY, withShift);
}

void GuiManager::HandlePromotionClick(int x, int y)
{
    RECT rc;
    GetClientRect(m_hWnd, &rc);
    int cx = (rc.right - rc.left) / 2;
    int cy = (rc.bottom - rc.top) / 2;
    int w = 300, h = 100;

    int startX = cx - w / 2;
    int startY = cy - h / 2;

    if (y < startY || y > startY + h || x < startX || x > startX + w) return; // 박스 밖 클릭 무시

    int idx = (x - startX) / (w / 4);
    if (idx < 0 || idx > 3) return;

    PieceType types[] = { PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight };
    m_pendingPromotionMove.promotion = types[idx];

    // 승급 확정 및 이동 적용
    m_isPromoting = false;
    m_board.PushState(m_isWhiteTurn);

    Piece moving = m_board.GetPiece(m_pendingPromotionMove.sx, m_pendingPromotionMove.sy);
    m_gameLogic.ApplyMove(m_board, m_pendingPromotionMove, m_isWhiteTurn);

    // 애니메이션은 이미 끝난 상태라고 가정하거나 여기서 다시 시작 (보통은 바로 변함)
    // 드래그 드롭의 경우 애니메이션이 필요 없을 수 있음.
    // 여기선 그냥 다시 그림
    m_pieceSelected = false;
    m_moveHints.clear();
    m_isWhiteTurn = !m_isWhiteTurn;
    Redraw();

    CheckAndHandleGameOver(); // 게임 종료 확인
    if (!m_isWhiteTurn && m_blackIsAI) RequestAIMove();
}

void GuiManager::OnLButtonUp(int x, int y)
{
    if (m_isPromoting) return;
    if (!m_dragging) return;

    int offset = m_tileSize / 3;
    int boardX = (x - offset) / m_tileSize;
    int boardY = (y - offset) / m_tileSize;

    m_dragging = false;

    if (boardX < 0 || boardX >= 8 || boardY < 0 || boardY >= 8) {
        m_pieceSelected = false; m_moveHints.clear(); Redraw(); return;
    }

    Move mv;
    mv.sx = m_selX; mv.sy = m_selY; mv.dx = boardX; mv.dy = boardY;

    // 승급 여부 확인 (폰이 끝에 도달)
    Piece p = m_board.GetPiece(m_selX, m_selY);
    if (p.type == PieceType::Pawn && p.color == PieceColor::White && boardY == 0) { // 백 폰 승급
        // 유효한 이동인지 먼저 확인 (기본 로직상)
        Board temp = m_board;
        if (m_gameLogic.ApplyMove(temp, mv, m_isWhiteTurn)) {
            m_isPromoting = true;
            m_pendingPromotionMove = mv;
            Redraw(); // 메뉴 표시
            return;
        }
    }

    Board temp = m_board;
    if (m_gameLogic.ApplyMove(temp, mv, m_isWhiteTurn)) {
        m_board.PushState(m_isWhiteTurn);
        m_gameLogic.ApplyMove(m_board, mv, m_isWhiteTurn);
        StartAnimation(mv.sx, mv.sy, mv.dx, mv.dy, m_dragPiece);
        m_pieceSelected = false; m_moveHints.clear(); m_isWhiteTurn = !m_isWhiteTurn;
        Redraw();

        CheckAndHandleGameOver();
        if (!m_isWhiteTurn && m_blackIsAI) RequestAIMove();
    }
    else {
        m_pieceSelected = false; m_moveHints.clear(); Redraw();
    }
}

void GuiManager::OnMouseMove(int x, int y, bool leftDown)
{
    if (m_isAIThinking || m_isPromoting) return;

    if (m_pieceSelected && leftDown && !m_dragging) {
        m_dragging = true; m_dragX = m_selX; m_dragY = m_selY;
        m_dragScreenX = x; m_dragScreenY = y;
        m_dragPiece = m_board.GetPiece(m_dragX, m_dragY);
        Redraw(); return;
    }
    if (m_dragging) {
        m_dragScreenX = x; m_dragScreenY = y; Redraw();
    }
}

void GuiManager::OnTimer(UINT id)
{
    if (id == TIMER_ANIM) {
        UpdateAnimation();
        CheckAIState();
    }
}

void GuiManager::HandlePlayerClick(int boardX, int boardY, bool withShift)
{
    if (!m_isWhiteTurn || !m_whiteIsHuman) return;

    if (!m_pieceSelected) {
        const Piece& p = m_board.GetPiece(boardX, boardY);
        if (p.type != PieceType::None && p.color == PieceColor::White) {
            m_pieceSelected = true; m_selX = boardX; m_selY = boardY;
            UpdateMoveHints(boardX, boardY);
            Redraw();
        }
    }
    else {
        if (boardX == m_selX && boardY == m_selY) {
            m_pieceSelected = false; m_moveHints.clear(); Redraw(); return;
        }

        Move mv{ m_selX, m_selY, boardX, boardY };

        // 승급 체크 (클릭 이동 시)
        Piece p = m_board.GetPiece(m_selX, m_selY);
        if (p.type == PieceType::Pawn && p.color == PieceColor::White && boardY == 0) {
            Board temp = m_board;
            if (m_gameLogic.ApplyMove(temp, mv, m_isWhiteTurn)) {
                m_isPromoting = true;
                m_pendingPromotionMove = mv;
                Redraw();
                return;
            }
        }

        Board temp = m_board;
        if (m_gameLogic.ApplyMove(temp, mv, m_isWhiteTurn)) {
            m_board.PushState(m_isWhiteTurn);
            Piece moving = m_board.GetPiece(m_selX, m_selY);
            m_gameLogic.ApplyMove(m_board, mv, m_isWhiteTurn);
            StartAnimation(mv.sx, mv.sy, mv.dx, mv.dy, moving);
            m_pieceSelected = false; m_moveHints.clear(); m_isWhiteTurn = !m_isWhiteTurn;
            Redraw();

            CheckAndHandleGameOver();
            if (!m_isWhiteTurn && m_blackIsAI) RequestAIMove();
        }
        else {
            const Piece& target = m_board.GetPiece(boardX, boardY);
            if (target.color == PieceColor::White) {
                m_pieceSelected = true; m_selX = boardX; m_selY = boardY;
                UpdateMoveHints(boardX, boardY); Redraw();
            }
            else {
                m_pieceSelected = false; m_moveHints.clear(); Redraw();
            }
        }
    }
}

void GuiManager::RequestAIMove()
{
    if (m_isAIThinking) return;
    // CheckAndHandleGameOver에서 이미 게임 끝났으면 호출 안됨

    std::string fen = Fen::BoardToFEN(m_board, m_isWhiteTurn);
    m_isAIThinking = true;
    m_aiFuture = std::async(std::launch::async, [this, fen]() {
        return m_engine.GetBestMove(fen);
        });
}

void GuiManager::CheckAIState()
{
    if (!m_isAIThinking) return;

    if (m_aiFuture.valid() && m_aiFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        std::string bestMove = m_aiFuture.get();
        m_isAIThinking = false;

        if (bestMove.size() >= 4)
        {
            Move mv{};
            mv.sx = bestMove[0] - 'a';
            mv.sy = 8 - (bestMove[1] - '0');
            mv.dx = bestMove[2] - 'a';
            mv.dy = 8 - (bestMove[3] - '0');

            // [승급 파싱] e.g. a7a8q
            if (bestMove.size() >= 5) {
                char promoChar = bestMove[4];
                switch (promoChar) {
                case 'q': mv.promotion = PieceType::Queen; break;
                case 'r': mv.promotion = PieceType::Rook; break;
                case 'b': mv.promotion = PieceType::Bishop; break;
                case 'n': mv.promotion = PieceType::Knight; break;
                default: mv.promotion = PieceType::Queen; break;
                }
            }

            m_board.PushState(m_isWhiteTurn);
            Piece moving = m_board.GetPiece(mv.sx, mv.sy);
            if (m_gameLogic.ApplyMove(m_board, mv, m_isWhiteTurn))
            {
                StartAnimation(mv.sx, mv.sy, mv.dx, mv.dy, moving);
                m_isWhiteTurn = !m_isWhiteTurn;
                Redraw();
                CheckAndHandleGameOver();
            }
        }
    }
}

void GuiManager::CheckAndHandleGameOver()
{
    GameState state = m_gameLogic.CheckGameState(m_board, m_isWhiteTurn);
    if (state == GameState::Checkmate) {
        std::wstring msg = m_isWhiteTurn ? L"Checkmate! Black Wins!" : L"Checkmate! White Wins!";
        MessageBoxW(m_hWnd, msg.c_str(), L"Game Over", MB_OK | MB_ICONINFORMATION);
        // 게임 리셋 or 멈춤 로직 추가 가능
    }
    else if (state == GameState::Stalemate) {
        MessageBoxW(m_hWnd, L"Stalemate! Draw!", L"Game Over", MB_OK | MB_ICONINFORMATION);
    }
}

void GuiManager::UpdateMoveHints(int x, int y)
{
    m_moveHints.clear();
    std::vector<Move> moves;
    m_gameLogic.GeneratePseudoLegalMoves(m_board, x, y, m_isWhiteTurn, moves);
    for (const auto& mv : moves) {
        // 힌트 표시할 때, 실제로 둬봐서 체크가 안되는지 확인해야 완벽함
        // GeneratePseudoLegalMoves는 체크를 고려하지 않은 의사 합법수
        // 여기서 체크 필터링
        Board temp = m_board;
        if (m_gameLogic.ApplyMove(temp, mv, m_isWhiteTurn)) {
            MoveHint h; h.x = mv.dx; h.y = mv.dy;
            m_moveHints.push_back(h);
        }
    }
}

void GuiManager::StartAnimation(int sx, int sy, int dx, int dy, const Piece& p)
{
    m_anim.active = true;
    m_anim.fromX = sx; m_anim.fromY = sy; m_anim.toX = dx; m_anim.toY = dy;
    m_anim.progress = 0.0;
    m_anim.startTick = GetTickCount64();
    m_anim.movingPiece = p;
}

void GuiManager::UpdateAnimation()
{
    if (!m_anim.active) return;
    ULONGLONG now = GetTickCount64();
    ULONGLONG elapsed = now - m_anim.startTick;
    const double duration = 200.0;
    m_anim.progress = (double)elapsed / duration;
    if (m_anim.progress >= 1.0) { m_anim.progress = 1.0; m_anim.active = false; }
    Redraw();
}