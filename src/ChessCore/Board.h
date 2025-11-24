#pragma once
#include <array>
#include <vector>
#include "Piece.h"

// 보드 상태 백업용 구조체 (무르기 구현용)
struct BoardState
{
    std::array<std::array<Piece, 8>, 8> cells;
    bool whiteCanCastleK = true;
    bool whiteCanCastleQ = true;
    bool blackCanCastleK = true;
    bool blackCanCastleQ = true;
    int  enPassantX = -1; // 앙파상 가능 타겟 (없으면 -1)
    int  enPassantY = -1;
    bool isWhiteTurn = true;
};

class Board
{
public:
    Board();

    void ResetToStartPosition();

    const Piece& GetPiece(int x, int y) const;
    Piece& GetPiece(int x, int y);
    void SetPiece(int x, int y, const Piece& p);

    // 단순 이동 (좌표만 변경)
    void MovePieceRaw(int sx, int sy, int dx, int dy);

    // 상태 관리
    void PushState(bool isWhiteTurn); // 현재 상태 저장
    bool PopState(); // 이전 상태 복구 (Undo)

    // 특수 규칙 플래그
    bool m_whiteCanCastleK = true;
    bool m_whiteCanCastleQ = true;
    bool m_blackCanCastleK = true;
    bool m_blackCanCastleQ = true;
    int  m_enPassantX = -1;
    int  m_enPassantY = -1;

private:
    std::array<std::array<Piece, 8>, 8> m_board;
    std::vector<BoardState> m_history; // 히스토리 스택
};