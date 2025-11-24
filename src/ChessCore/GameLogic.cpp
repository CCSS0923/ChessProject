#include "GameLogic.h"
#include <cmath>

GameLogic::GameLogic() {}

bool GameLogic::IsMoveLegalBasic(const Board& board, const Move& move, bool isWhiteTurn)
{
    if (move.sx == move.dx && move.sy == move.dy) return false;
    if (move.sx < 0 || move.sx >= 8 || move.sy < 0 || move.sy >= 8) return false;
    if (move.dx < 0 || move.dx >= 8 || move.dy < 0 || move.dy >= 8) return false;

    const Piece& from = board.GetPiece(move.sx, move.sy);
    const Piece& to = board.GetPiece(move.dx, move.dy);

    if (from.type == PieceType::None) return false;
    if (isWhiteTurn && from.color != PieceColor::White) return false;
    if (!isWhiteTurn && from.color != PieceColor::Black) return false;
    if (to.type != PieceType::None && to.color == from.color) return false;

    return true;
}

bool GameLogic::IsSquareAttacked(const Board& board, int x, int y, bool byWhite)
{
    PieceColor attackerColor = byWhite ? PieceColor::White : PieceColor::Black;

    // 1. 폰 공격 확인
    int pawnDir = byWhite ? -1 : 1; // 백폰(6->0)은 -1 방향으로 공격
    // 공격자가 (x-1, y-dir) 또는 (x+1, y-dir)에 있어야 (x,y)를 공격함
    // (주의: byWhite는 '공격하는 쪽' 기준)
    // 백이 공격하려면 백폰은 (y+1)에 위치해야 y를 공격함 (y좌표가 줄어드는 방향이 전진이므로)
    int attackY = y - pawnDir;
    if (attackY >= 0 && attackY < 8) {
        for (int dx = -1; dx <= 1; dx += 2) {
            int nx = x + dx;
            if (nx >= 0 && nx < 8) {
                const Piece& p = board.GetPiece(nx, attackY);
                if (p.type == PieceType::Pawn && p.color == attackerColor) return true;
            }
        }
    }

    // 2. 나이트 공격 확인
    static const int k_offs[8][2] = { {1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2} };
    for (auto& o : k_offs) {
        int nx = x + o[0], ny = y + o[1];
        if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
            const Piece& p = board.GetPiece(nx, ny);
            if (p.color == attackerColor && p.type == PieceType::Knight) return true;
        }
    }

    // 3. 킹 공격 확인 (인접 8칸)
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
                const Piece& p = board.GetPiece(nx, ny);
                if (p.color == attackerColor && p.type == PieceType::King) return true;
            }
        }
    }

    // 4. 직선(룩, 퀸) 및 대각선(비숍, 퀸) 슬라이딩 공격
    // (상하좌우)
    static const int rookDirs[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
    for (auto& d : rookDirs) {
        int nx = x + d[0], ny = y + d[1];
        while (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
            const Piece& p = board.GetPiece(nx, ny);
            if (p.type != PieceType::None) {
                if (p.color == attackerColor && (p.type == PieceType::Rook || p.type == PieceType::Queen)) return true;
                break;
            }
            nx += d[0]; ny += d[1];
        }
    }
    // (대각선)
    static const int bishopDirs[4][2] = { {1,1},{1,-1},{-1,1},{-1,-1} };
    for (auto& d : bishopDirs) {
        int nx = x + d[0], ny = y + d[1];
        while (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
            const Piece& p = board.GetPiece(nx, ny);
            if (p.type != PieceType::None) {
                if (p.color == attackerColor && (p.type == PieceType::Bishop || p.type == PieceType::Queen)) return true;
                break;
            }
            nx += d[0]; ny += d[1];
        }
    }

    return false;
}

bool GameLogic::IsKingInCheck(const Board& board, bool isWhiteKing)
{
    int kx = -1, ky = -1;
    PieceColor target = isWhiteKing ? PieceColor::White : PieceColor::Black;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const Piece& p = board.GetPiece(x, y);
            if (p.type == PieceType::King && p.color == target) {
                kx = x; ky = y; break;
            }
        }
    }
    // 킹이 없으면(비정상) 체크 아님
    if (kx == -1) return false;
    // 내 킹이 상대방( !isWhiteKing )에 의해 공격받는지 확인
    return IsSquareAttacked(board, kx, ky, !isWhiteKing);
}

bool GameLogic::HasLegalMoves(const Board& board, bool isWhiteTurn)
{
    // 모든 아군 기물에 대해
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const Piece& p = board.GetPiece(x, y);
            if (p.type == PieceType::None) continue;
            if (isWhiteTurn && p.color != PieceColor::White) continue;
            if (!isWhiteTurn && p.color != PieceColor::Black) continue;

            std::vector<Move> moves;
            GeneratePseudoLegalMoves(board, x, y, isWhiteTurn, moves);

            for (const auto& mv : moves) {
                // 수를 둬보고 킹이 안전한지 확인
                Board temp = board;
                if (ApplyMove(temp, mv, isWhiteTurn)) {
                    // ApplyMove 내부에서 체크 검증까지 통과했다면 true
                    return true;
                }
            }
        }
    }
    return false;
}

GameState GameLogic::CheckGameState(const Board& board, bool isWhiteTurn)
{
    // 1. 합법적인 수가 있는지 확인
    if (HasLegalMoves(board, isWhiteTurn)) {
        return GameState::Playing;
    }

    // 2. 합법수가 없으면 체크메이트 아니면 스테일메이트
    if (IsKingInCheck(board, isWhiteTurn)) {
        return GameState::Checkmate;
    }
    else {
        return GameState::Stalemate;
    }
}

bool GameLogic::ApplyMove(Board& board, const Move& move, bool isWhiteTurn)
{
    if (!IsMoveLegalBasic(board, move, isWhiteTurn)) return false;

    Piece p = board.GetPiece(move.sx, move.sy);
    int dx = move.dx - move.sx;
    int dy = move.dy - move.sy;
    int absDx = std::abs(dx);
    int absDy = std::abs(dy);

    // --- 규칙 검사 ---
    if (p.type == PieceType::Pawn)
    {
        int dir = (p.color == PieceColor::White) ? -1 : 1;
        const Piece& target = board.GetPiece(move.dx, move.dy);

        if (absDx == 0) // 전진
        {
            if (dy == dir && target.type == PieceType::None) {}
            else if (!p.hasMoved && dy == 2 * dir && target.type == PieceType::None)
            {
                int midY = move.sy + dir;
                if (board.GetPiece(move.dx, midY).type != PieceType::None) return false;
                board.m_enPassantX = move.sx; board.m_enPassantY = midY;
            }
            else return false;
        }
        else if (absDx == 1 && dy == dir) // 대각선
        {
            if (target.type != PieceType::None && target.color != p.color) {}
            else if (move.dx == board.m_enPassantX && move.dy == board.m_enPassantY) {
                board.SetPiece(move.dx, move.sy, Piece()); // 앙파상 캡처
            }
            else return false;
        }
        else return false;
    }
    else if (p.type == PieceType::King)
    {
        if (absDx == 2 && dy == 0) // 캐슬링
        {
            if (p.hasMoved) return false;
            if (IsKingInCheck(board, isWhiteTurn)) return false; // 체크 상태에선 캐슬링 불가

            int rookX = (dx > 0) ? 7 : 0;
            int rookDx = (dx > 0) ? 5 : 3;
            Piece rook = board.GetPiece(rookX, move.sy);
            if (rook.type != PieceType::Rook || rook.hasMoved) return false;

            int step = (dx > 0) ? 1 : -1;
            // 경로 빈칸 + 경로 공격받는지 확인 (킹이 지나가는 길)
            for (int k = move.sx + step; k != rookX; k += step) {
                if (board.GetPiece(k, move.sy).type != PieceType::None) return false;
                // 킹의 이동 경로(최종 위치 포함 전까지)가 공격받으면 안됨
                // (단순 구현: 이동 전, 이동 중, 이동 후 체크는 호출자가 검증한다고 가정하거나 여기서 체크)
                if (std::abs(k - move.sx) <= 2 && IsSquareAttacked(board, k, move.sy, !isWhiteTurn)) return false;
            }
            board.MovePieceRaw(rookX, move.sy, rookDx, move.sy);
        }
        else if (absDx > 1 || absDy > 1) return false;
    }
    else if (p.type == PieceType::Knight) { if (!((absDx == 1 && absDy == 2) || (absDx == 2 && absDy == 1))) return false; }
    else if (p.type == PieceType::Rook) { if (dx != 0 && dy != 0) return false; }
    else if (p.type == PieceType::Bishop) { if (absDx != absDy) return false; }
    else if (p.type == PieceType::Queen) { if ((dx != 0 && dy != 0) && (absDx != absDy)) return false; }

    if (p.type == PieceType::Rook || p.type == PieceType::Bishop || p.type == PieceType::Queen) {
        int stepX = (dx == 0) ? 0 : (dx > 0 ? 1 : -1);
        int stepY = (dy == 0) ? 0 : (dy > 0 ? 1 : -1);
        int cx = move.sx + stepX; int cy = move.sy + stepY;
        while (cx != move.dx || cy != move.dy) {
            if (board.GetPiece(cx, cy).type != PieceType::None) return false;
            cx += stepX; cy += stepY;
        }
    }

    // --- 실제 이동 적용 ---
    if (p.type != PieceType::Pawn || absDy != 2) {
        board.m_enPassantX = -1; board.m_enPassantY = -1;
    }

    board.MovePieceRaw(move.sx, move.sy, move.dx, move.dy);

    // [승급 로직 수정]
    if (p.type == PieceType::Pawn && (move.dy == 0 || move.dy == 7))
    {
        Piece promo = board.GetPiece(move.dx, move.dy);
        // move.promotion에 값이 있으면 그걸로, 없으면 퀸(기본값)
        promo.type = (move.promotion != PieceType::None) ? move.promotion : PieceType::Queen;
        board.SetPiece(move.dx, move.dy, promo);
    }

    // 캐슬링 권한 상실
    if (p.type == PieceType::King) {
        if (p.color == PieceColor::White) { board.m_whiteCanCastleK = false; board.m_whiteCanCastleQ = false; }
        else { board.m_blackCanCastleK = false; board.m_blackCanCastleQ = false; }
    }
    if (p.type == PieceType::Rook) {
        if (move.sx == 0 && move.sy == 7) board.m_whiteCanCastleQ = false;
        if (move.sx == 7 && move.sy == 7) board.m_whiteCanCastleK = false;
        if (move.sx == 0 && move.sy == 0) board.m_blackCanCastleQ = false;
        if (move.sx == 7 && move.sy == 0) board.m_blackCanCastleK = false;
    }

    // [중요] 이동 후 내 킹이 체크 상태면 이동 취소 (불법수)
    if (IsKingInCheck(board, isWhiteTurn)) return false; // (Board는 값 복사로 넘어오므로 원본 영향 없음)

    return true;
}

void GameLogic::GeneratePseudoLegalMoves(const Board& board, int x, int y, bool isWhiteTurn, std::vector<Move>& outMoves)
{
    outMoves.clear();
    const Piece& p = board.GetPiece(x, y);
    if (p.type == PieceType::None) return;
    if (isWhiteTurn && p.color != PieceColor::White) return;
    if (!isWhiteTurn && p.color != PieceColor::Black) return;

    switch (p.type) {
    case PieceType::Pawn:   AddPawnMoves(board, x, y, isWhiteTurn, outMoves); break;
    case PieceType::Knight: AddKnightMoves(board, x, y, isWhiteTurn, outMoves); break;
    case PieceType::Bishop: { static const int d[4][2] = { {1,1},{1,-1},{-1,1},{-1,-1} }; AddSlidingMoves(board, x, y, d, 4, isWhiteTurn, outMoves); break; }
    case PieceType::Rook: { static const int d[4][2] = { {1,0},{-1,0},{0,1},{0,-1} }; AddSlidingMoves(board, x, y, d, 4, isWhiteTurn, outMoves); break; }
    case PieceType::Queen: { static const int d[8][2] = { {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1} }; AddSlidingMoves(board, x, y, d, 8, isWhiteTurn, outMoves); break; }
    case PieceType::King: {
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            Move mv{ x, y, x + dx, y + dy };
            if (IsMoveLegalBasic(board, mv, isWhiteTurn)) outMoves.push_back(mv);
        }
        Move ck{ x, y, x + 2, y }; Board t = board; if (ApplyMove(t, ck, isWhiteTurn)) outMoves.push_back(ck);
        Move cq{ x, y, x - 2, y }; t = board; if (ApplyMove(t, cq, isWhiteTurn)) outMoves.push_back(cq);
        break;
    }
    }
}

void GameLogic::AddPawnMoves(const Board& board, int x, int y, bool isWhiteTurn, std::vector<Move>& outMoves) {
    const Piece& p = board.GetPiece(x, y);
    int dir = (p.color == PieceColor::White) ? -1 : 1;
    int ny = y + dir;
    if (ny >= 0 && ny < 8 && board.GetPiece(x, ny).type == PieceType::None) {
        outMoves.push_back({ x, y, x, ny });
        if (!p.hasMoved && y + 2 * dir >= 0 && y + 2 * dir < 8 && board.GetPiece(x, y + 2 * dir).type == PieceType::None)
            outMoves.push_back({ x, y, x, y + 2 * dir });
    }
    for (int dx = -1; dx <= 1; dx += 2) {
        int nx = x + dx;
        if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
            const Piece& t = board.GetPiece(nx, ny);
            if (t.type != PieceType::None && t.color != p.color) outMoves.push_back({ x, y, nx, ny });
            else if (nx == board.m_enPassantX && ny == board.m_enPassantY) outMoves.push_back({ x, y, nx, ny });
        }
    }
}
void GameLogic::AddKnightMoves(const Board& board, int x, int y, bool isWhiteTurn, std::vector<Move>& outMoves) {
    static const int o[8][2] = { {1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2} };
    for (auto& k : o) { Move mv{ x, y, x + k[0], y + k[1] }; if (IsMoveLegalBasic(board, mv, isWhiteTurn)) outMoves.push_back(mv); }
}
void GameLogic::AddSlidingMoves(const Board& board, int x, int y, const int dirs[][2], int dirCount, bool isWhiteTurn, std::vector<Move>& outMoves) {
    for (int d = 0; d < dirCount; ++d) {
        int nx = x + dirs[d][0], ny = y + dirs[d][1];
        while (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
            Move mv{ x, y, nx, ny };
            const Piece& t = board.GetPiece(nx, ny);
            if (t.type != PieceType::None) {
                if (t.color != (isWhiteTurn ? PieceColor::White : PieceColor::Black)) outMoves.push_back(mv);
                break;
            }
            outMoves.push_back(mv);
            nx += dirs[d][0]; ny += dirs[d][1];
        }
    }
}