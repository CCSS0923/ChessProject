#pragma once
#include <windows.h>
#include <string>

class StockfishEngine
{
public:
    StockfishEngine();
    ~StockfishEngine();

    bool Initialize(const std::wstring& enginePath);
    void Shutdown();

    void SendCommand(const std::string& cmd);
    std::string GetBestMove(const std::string& fen);

private:
    PROCESS_INFORMATION m_pi{};
    HANDLE m_hChildStdinRd = nullptr;
    HANDLE m_hChildStdinWr = nullptr;
    HANDLE m_hChildStdoutRd = nullptr;
    HANDLE m_hChildStdoutWr = nullptr;
    bool m_initialized = false;

    std::string ReadLine();
};
