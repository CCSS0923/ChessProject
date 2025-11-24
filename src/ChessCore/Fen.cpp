#include "Fen.h"
#include "Board.h"
#include "Piece.h"

std::string Fen::BoardToFEN(const Board& board, bool isWhiteTurn)
{
    std::string fen;

    for (int rank = 0; rank < 8; ++rank)
    {
        int emptyCount = 0;
        for (int file = 0; file < 8; ++file)
        {
            const Piece& p = board.GetPiece(file, rank);
            if (p.type == PieceType::None) {
                emptyCount++;
            }
            else {
                if (emptyCount > 0) {
                    fen += std::to_string(emptyCount);
                    emptyCount = 0;
                }
                char c = ' ';
                switch (p.type) {
                case PieceType::Pawn:   c = 'p'; break;
                case PieceType::Knight: c = 'n'; break;
                case PieceType::Bishop: c = 'b'; break;
                case PieceType::Rook:   c = 'r'; break;
                case PieceType::Queen:  c = 'q'; break;
                case PieceType::King:   c = 'k'; break;
                }
                if (p.color == PieceColor::White) c = toupper(c);
                fen += c;
            }
        }
        if (emptyCount > 0) fen += std::to_string(emptyCount);
        if (rank != 7) fen += '/';
    }

    // 1. 턴
    fen += isWhiteTurn ? " w " : " b ";

    // 2. 캐슬링 권한
    std::string castling = "";
    if (board.m_whiteCanCastleK) castling += 'K';
    if (board.m_whiteCanCastleQ) castling += 'Q';
    if (board.m_blackCanCastleK) castling += 'k';
    if (board.m_blackCanCastleQ) castling += 'q';
    if (castling.empty()) castling = "-";
    fen += castling + " ";

    // 3. 앙파상 타겟
    if (board.m_enPassantX != -1 && board.m_enPassantY != -1) {
        char file = 'a' + board.m_enPassantX;
        int rank = 8 - board.m_enPassantY;
        fen += file + std::to_string(rank);
    }
    else {
        fen += "-";
    }

    // 4. 하프무브/풀무브 (약식으로 0 1)
    fen += " 0 1";
    return fen;
}