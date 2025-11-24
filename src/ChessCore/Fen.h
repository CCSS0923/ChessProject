#pragma once
#include <string>

class Board;

namespace Fen
{
    std::string BoardToFEN(const Board& board, bool isWhiteTurn);
}
