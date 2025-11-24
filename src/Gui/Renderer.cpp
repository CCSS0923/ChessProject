#include "Renderer.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "../Utils/Logger.h"

// [중요] AlphaBlend 함수 사용을 위해 라이브러리 링크
#pragma comment(lib, "Msimg32.lib")

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::Initialize()
{
    LoadPieceImages();
}

void Renderer::LoadPieceImages()
{
    // [경로 설정] 상위 폴더의 assets를 참조
    struct PieceImageInfo {
        const wchar_t* key;
        const wchar_t* filename;
    } infos[] = {
        { L"wp", L"../assets/pieces/Pawn_white.png" },
        { L"wr", L"../assets/pieces/Rook_white.png" },
        { L"wn", L"../assets/pieces/Knight_white.png" },
        { L"wb", L"../assets/pieces/Bishop_white.png" },
        { L"wq", L"../assets/pieces/Queen_white.png" },
        { L"wk", L"../assets/pieces/King_white.png" },
        { L"bp", L"../assets/pieces/Pawn_black.png" },
        { L"br", L"../assets/pieces/Rook_black.png" },
        { L"bn", L"../assets/pieces/Knight_black.png" },
        { L"bb", L"../assets/pieces/Bishop_black.png" },
        { L"bq", L"../assets/pieces/Queen_black.png" },
        { L"bk", L"../assets/pieces/King_black.png" },
    };

    bool anyError = false;

    for (auto& info : infos)
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, info.filename, -1, nullptr, 0, nullptr, nullptr);
        std::string path8;
        path8.resize(len);
        WideCharToMultiByte(CP_UTF8, 0, info.filename, -1, &path8[0], len, nullptr, nullptr);

        // [중요] 알파 채널(투명도)을 포함하여 로드 (IMREAD_UNCHANGED)
        cv::Mat img = cv::imread(path8, cv::IMREAD_UNCHANGED);
        if (img.empty())
        {
            Log(L"이미지 로드 실패: " + std::wstring(info.filename));
            anyError = true;
        }
        else
        {
            // 이미지가 3채널(BGR)이라면 4채널(BGRA)로 변환하여 통일
            if (img.channels() == 3)
            {
                cv::cvtColor(img, img, cv::COLOR_BGR2BGRA);
            }
            m_pieceImages[info.key] = img;
        }
    }

    if (anyError)
    {
        Log(L"경로 확인 필요: ../assets 폴더가 존재하는지 확인하세요.");
    }
}

void Renderer::DrawBoard(HDC hdc,
    const Board& board,
    int tileSize,
    int selX, int selY, bool hasSelection,
    const std::vector<MoveHint>& hints,
    const MoveAnim& anim,
    bool dragging,
    int dragScreenX, int dragScreenY,
    const Piece& dragPiece)
{
    DrawWoodenTiles(hdc, tileSize);
    DrawPieces(hdc, board, tileSize, anim, dragging, dragScreenX, dragScreenY, dragPiece);
    DrawSelectionAndHints(hdc, tileSize, selX, selY, hasSelection, hints);
}

void Renderer::DrawWoodenTiles(HDC hdc, int tileSize)
{
    COLORREF light = RGB(240, 220, 180);
    COLORREF dark = RGB(180, 120, 70);

    int boardSize = tileSize * 8;
    int frameThickness = tileSize / 3;
    COLORREF frameColor = RGB(140, 80, 40);

    HBRUSH frameBrush = CreateSolidBrush(frameColor);
    RECT outer{ 0, 0, boardSize + frameThickness * 2, boardSize + frameThickness * 2 };
    FillRect(hdc, &outer, frameBrush);
    DeleteObject(frameBrush);

    HBRUSH bgBrush = CreateSolidBrush(RGB(40, 30, 20));
    RECT inner{ frameThickness, frameThickness, frameThickness + boardSize, frameThickness + boardSize };
    FillRect(hdc, &inner, bgBrush);
    DeleteObject(bgBrush);

    POINT oldOrg;
    OffsetViewportOrgEx(hdc, frameThickness, frameThickness, &oldOrg);

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            bool isDark = ((x + y) % 2) != 0;
            COLORREF base = isDark ? dark : light;

            HBRUSH brush = CreateSolidBrush(base);
            RECT rc{ x * tileSize, y * tileSize, (x + 1) * tileSize, (y + 1) * tileSize };
            FillRect(hdc, &rc, brush);
            DeleteObject(brush);
        }
    }

    SetViewportOrgEx(hdc, oldOrg.x, oldOrg.y, nullptr);
}

void Renderer::DrawPieces(HDC hdc,
    const Board& board,
    int tileSize,
    const MoveAnim& anim,
    bool dragging,
    int dragScreenX, int dragScreenY,
    const Piece& dragPiece)
{
    int offset = tileSize / 3;
    POINT oldOrg;
    OffsetViewportOrgEx(hdc, offset, offset, &oldOrg);

    int animFromX = -1, animFromY = -1, animToX = -1, animToY = -1;
    if (anim.active)
    {
        animFromX = anim.fromX;
        animFromY = anim.fromY;
        animToX = anim.toX;
        animToY = anim.toY;
    }

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            const Piece& p = board.GetPiece(x, y);
            if (p.type == PieceType::None) continue;
            if (anim.active && x == animToX && y == animToY) continue;

            std::wstring key;
            key += (p.color == PieceColor::White) ? L'w' : L'b';
            switch (p.type)
            {
            case PieceType::Pawn:   key += L'p'; break;
            case PieceType::Rook:   key += L'r'; break;
            case PieceType::Knight: key += L'n'; break;
            case PieceType::Bishop: key += L'b'; break;
            case PieceType::Queen:  key += L'q'; break;
            case PieceType::King:   key += L'k'; break;
            default: break;
            }

            if (m_pieceImages.find(key) != m_pieceImages.end())
            {
                DrawMat(hdc, x * tileSize, y * tileSize, m_pieceImages[key], tileSize, tileSize);
            }
        }
    }

    if (anim.active)
    {
        std::wstring key;
        key += (anim.movingPiece.color == PieceColor::White) ? L'w' : L'b';
        switch (anim.movingPiece.type)
        {
        case PieceType::Pawn:   key += L'p'; break;
        case PieceType::Rook:   key += L'r'; break;
        case PieceType::Knight: key += L'n'; break;
        case PieceType::Bishop: key += L'b'; break;
        case PieceType::Queen:  key += L'q'; break;
        case PieceType::King:   key += L'k'; break;
        default: break;
        }
        if (m_pieceImages.find(key) != m_pieceImages.end())
        {
            double t = anim.progress;
            int px = (int)((anim.fromX + (anim.toX - anim.fromX) * t) * tileSize);
            int py = (int)((anim.fromY + (anim.toY - anim.fromY) * t) * tileSize);
            DrawMat(hdc, px, py, m_pieceImages[key], tileSize, tileSize);
        }
    }

    if (dragging && dragPiece.type != PieceType::None)
    {
        std::wstring key;
        key += (dragPiece.color == PieceColor::White) ? L'w' : L'b';
        switch (dragPiece.type)
        {
        case PieceType::Pawn:   key += L'p'; break;
        case PieceType::Rook:   key += L'r'; break;
        case PieceType::Knight: key += L'n'; break;
        case PieceType::Bishop: key += L'b'; break;
        case PieceType::Queen:  key += L'q'; break;
        case PieceType::King:   key += L'k'; break;
        default: break;
        }
        if (m_pieceImages.find(key) != m_pieceImages.end())
        {
            int px = dragScreenX - tileSize / 2;
            int py = dragScreenY - tileSize / 2;
            DrawMat(hdc, px, py, m_pieceImages[key], tileSize, tileSize);
        }
    }

    SetViewportOrgEx(hdc, oldOrg.x, oldOrg.y, nullptr);
}

void Renderer::DrawSelectionAndHints(HDC hdc,
    int tileSize,
    int selX, int selY, bool hasSelection,
    const std::vector<MoveHint>& hints)
{
    int offset = tileSize / 3;
    POINT oldOrg;
    OffsetViewportOrgEx(hdc, offset, offset, &oldOrg);

    if (hasSelection && selX >= 0 && selY >= 0)
    {
        HBRUSH nullBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        HPEN pen = CreatePen(PS_SOLID, 3, RGB(0, 200, 0));
        HGDIOBJ oldBrush = SelectObject(hdc, nullBrush);
        HGDIOBJ oldPen = SelectObject(hdc, pen);

        RECT rc{ selX * tileSize, selY * tileSize, (selX + 1) * tileSize, (selY + 1) * tileSize };
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }

    for (const auto& h : hints)
    {
        int cx = h.x * tileSize + tileSize / 2;
        int cy = h.y * tileSize + tileSize / 2;
        int r = tileSize / 6;

        HBRUSH brush = CreateSolidBrush(RGB(0, 200, 0));
        HPEN nullPen = (HPEN)GetStockObject(NULL_PEN);
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        HGDIOBJ oldPen = SelectObject(hdc, nullPen);

        Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
    }

    SetViewportOrgEx(hdc, oldOrg.x, oldOrg.y, nullptr);
}

HBITMAP Renderer::MatToHBITMAP(const cv::Mat& src)
{
    // [중요] 투명도 처리를 위해 무조건 4채널(32비트) 비트맵 생성
    cv::Mat argb;
    if (src.channels() == 4)
    {
        argb = src.clone();
        // Win32 AlphaBlend는 Premultiplied Alpha를 요구합니다.
        // (R,G,B) = (R*A/255, G*A/255, B*A/255)
        int total = argb.rows * argb.cols;
        for (int i = 0; i < total; ++i)
        {
            unsigned char* pixel = argb.data + i * 4;
            unsigned char a = pixel[3];
            pixel[0] = (unsigned char)((int)pixel[0] * a / 255); // B
            pixel[1] = (unsigned char)((int)pixel[1] * a / 255); // G
            pixel[2] = (unsigned char)((int)pixel[2] * a / 255); // R
        }
    }
    else
    {
        cv::cvtColor(src, argb, cv::COLOR_BGR2BGRA);
        // 알파 채널이 없으면 투명도 255(불투명)로 유지
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = argb.cols;
    bmi.bmiHeader.biHeight = -argb.rows; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32; // 32비트 (알파 포함)
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC hdc = GetDC(nullptr);
    HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, hdc);

    if (hBitmap && bits)
    {
        // [중요] 행 단위 복사 (Padding 문제 해결)
        // OpenCV Mat은 연속적일 수도 있고 아닐 수도 있지만, DIBSection은 4바이트 정렬을 요구합니다.
        // 32비트 이미지는 항상 4바이트 정렬이므로 memcpy로 복사해도 안전합니다.
        // 하지만 stride(step) 차이가 날 수 있으므로 행 단위 복사가 가장 안전합니다.

        int widthBytes = argb.cols * 4;
        for (int y = 0; y < argb.rows; ++y)
        {
            unsigned char* dst = (unsigned char*)bits + y * widthBytes;
            unsigned char* srcPtr = argb.ptr(y);
            memcpy(dst, srcPtr, widthBytes);
        }
    }
    return hBitmap;
}

void Renderer::DrawMat(HDC hdc, int x, int y, const cv::Mat& mat, int width, int height)
{
    if (mat.empty()) return;

    // AlphaBlend를 위해 HBITMAP 생성
    HBITMAP hBmp = MatToHBITMAP(mat);
    if (!hBmp) return;

    HDC memDC = CreateCompatibleDC(hdc);
    HGDIOBJ old = SelectObject(memDC, hBmp);
    BITMAP bm{};
    GetObject(hBmp, sizeof(bm), &bm);

    // [변경] StretchBlt -> AlphaBlend
    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA; // 알파 채널 사용

    AlphaBlend(hdc, x, y, width, height,
        memDC, 0, 0, bm.bmWidth, bm.bmHeight, bf);

    SelectObject(memDC, old);
    DeleteObject(hBmp);
    DeleteDC(memDC);
}