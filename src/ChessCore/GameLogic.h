#pragma once
#include <vector>
#include "Board.h"

enum class GameState
{
    Playing,
    Checkmate,
    Stalemate
};

struct Move
{
    // [수정] 멤버 변수 초기화 (경고 C26495 해결)
    int sx = 0;
    int sy = 0;
    int dx = 0;
    int dy = 0;
    PieceType promotion = PieceType::None;
};

class GameLogic
{
public:
    GameLogic();

    bool ApplyMove(Board& board, const Move& move, bool isWhiteTurn);
    void GeneratePseudoLegalMoves(const Board& board, int x, int y, bool isWhiteTurn, std::vector<Move>& outMoves);
    bool IsKingInCheck(const Board& board, bool isWhiteKing);
    GameState CheckGameState(const Board& board, bool isWhiteTurn);

private:
    bool IsMoveLegalBasic(const Board& board, const Move& move, bool isWhiteTurn);
    bool IsSquareAttacked(const Board& board, int x, int y, bool byWhite);
    bool HasLegalMoves(const Board& board, bool isWhiteTurn);

    void AddPawnMoves(const Board& board, int x, int y, bool isWhiteTurn, std::vector<Move>& outMoves);
    void AddKnightMoves(const Board& board, int x, int y, bool isWhiteTurn, std::vector<Move>& outMoves);
    void AddSlidingMoves(const Board& board, int x, int y, const int dirs[][2], int dirCount, bool isWhiteTurn, std::vector<Move>& outMoves);
};