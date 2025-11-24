#pragma once
#include <cstdint>

enum class PieceType : uint8_t
{
    None = 0,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

enum class PieceColor : uint8_t
{
    None = 0,
    White,
    Black
};

struct Piece
{
    PieceType  type;
    PieceColor color;
    bool       hasMoved;

    Piece() : type(PieceType::None), color(PieceColor::None), hasMoved(false) {}
    Piece(PieceType t, PieceColor c) : type(t), color(c), hasMoved(false) {}
};
