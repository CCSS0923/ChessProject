#include "Board.h"

Board::Board()
{
    ResetToStartPosition();
}

void Board::ResetToStartPosition()
{
    m_history.clear();
    m_whiteCanCastleK = true; m_whiteCanCastleQ = true;
    m_blackCanCastleK = true; m_blackCanCastleQ = true;
    m_enPassantX = -1; m_enPassantY = -1;

    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            m_board[y][x] = Piece();

    // 백 폰 / 흑 폰
    for (int x = 0; x < 8; ++x) {
        m_board[6][x] = Piece(PieceType::Pawn, PieceColor::White);
        m_board[1][x] = Piece(PieceType::Pawn, PieceColor::Black);
    }

    // 백 기물 (Rank 7)
    PieceColor wc = PieceColor::White;
    m_board[7][0] = Piece(PieceType::Rook, wc);   m_board[7][1] = Piece(PieceType::Knight, wc);
    m_board[7][2] = Piece(PieceType::Bishop, wc); m_board[7][3] = Piece(PieceType::Queen, wc);
    m_board[7][4] = Piece(PieceType::King, wc);   m_board[7][5] = Piece(PieceType::Bishop, wc);
    m_board[7][6] = Piece(PieceType::Knight, wc); m_board[7][7] = Piece(PieceType::Rook, wc);

    // 흑 기물 (Rank 0)
    PieceColor bc = PieceColor::Black;
    m_board[0][0] = Piece(PieceType::Rook, bc);   m_board[0][1] = Piece(PieceType::Knight, bc);
    m_board[0][2] = Piece(PieceType::Bishop, bc); m_board[0][3] = Piece(PieceType::Queen, bc);
    m_board[0][4] = Piece(PieceType::King, bc);   m_board[0][5] = Piece(PieceType::Bishop, bc);
    m_board[0][6] = Piece(PieceType::Knight, bc); m_board[0][7] = Piece(PieceType::Rook, bc);
}

const Piece& Board::GetPiece(int x, int y) const { return m_board[y][x]; }
Piece& Board::GetPiece(int x, int y) { return m_board[y][x]; }
void Board::SetPiece(int x, int y, const Piece& p) { m_board[y][x] = p; }

void Board::MovePieceRaw(int sx, int sy, int dx, int dy)
{
    Piece p = m_board[sy][sx];
    p.hasMoved = true;
    m_board[dy][dx] = p;
    m_board[sy][sx] = Piece();
}

void Board::PushState(bool isWhiteTurn)
{
    BoardState state;
    state.cells = m_board;
    state.whiteCanCastleK = m_whiteCanCastleK;
    state.whiteCanCastleQ = m_whiteCanCastleQ;
    state.blackCanCastleK = m_blackCanCastleK;
    state.blackCanCastleQ = m_blackCanCastleQ;
    state.enPassantX = m_enPassantX;
    state.enPassantY = m_enPassantY;
    state.isWhiteTurn = isWhiteTurn;
    m_history.push_back(state);
}

bool Board::PopState()
{
    if (m_history.empty()) return false;
    BoardState state = m_history.back();
    m_history.pop_back();

    m_board = state.cells;
    m_whiteCanCastleK = state.whiteCanCastleK;
    m_whiteCanCastleQ = state.whiteCanCastleQ;
    m_blackCanCastleK = state.blackCanCastleK;
    m_blackCanCastleQ = state.blackCanCastleQ;
    m_enPassantX = state.enPassantX;
    m_enPassantY = state.enPassantY;
    return true;
}