#include "Logger.h"
#include <windows.h>

void Log(const std::wstring& msg)
{
    std::wstring s = L"[ChessProject] " + msg + L"\n";
    OutputDebugStringW(s.c_str());
}

void LogA(const std::string& msg)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, nullptr, 0);
    std::wstring w;
    w.resize(len);
    MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, &w[0], len);
    Log(w);
}
