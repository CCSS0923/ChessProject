#include "Stockfish.h"
#include "../Utils/Logger.h"

StockfishEngine::StockfishEngine() {}
StockfishEngine::~StockfishEngine()
{
    Shutdown();
}

bool StockfishEngine::Initialize(const std::wstring& enginePath)
{
    if (m_initialized)
        return true;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    if (!CreatePipe(&m_hChildStdoutRd, &m_hChildStdoutWr, &sa, 0))
        return false;
    if (!SetHandleInformation(m_hChildStdoutRd, HANDLE_FLAG_INHERIT, 0))
        return false;

    if (!CreatePipe(&m_hChildStdinRd, &m_hChildStdinWr, &sa, 0))
        return false;
    if (!SetHandleInformation(m_hChildStdinWr, HANDLE_FLAG_INHERIT, 0))
        return false;

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = m_hChildStdoutWr;
    si.hStdOutput = m_hChildStdoutWr;
    si.hStdInput = m_hChildStdinRd;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // [중요] 엔진 경로 처리 (따옴표로 감싸기)
    std::wstring cmd = L"\"" + enginePath + L"\"";
    wchar_t cmdLine[512]{};
    wcsncpy_s(cmdLine, cmd.c_str(), _TRUNCATE);

    ZeroMemory(&m_pi, sizeof(m_pi));

    // [핵심 수정] dwCreationFlags에 'CREATE_NO_WINDOW' 플래그 추가
    // 이 플래그가 있어야 자식 프로세스(Stockfish)의 검정색 콘솔창이 뜨지 않습니다.
    DWORD creationFlags = CREATE_NO_WINDOW;

    if (!CreateProcessW(
        nullptr,
        cmdLine,
        nullptr,
        nullptr,
        TRUE,
        creationFlags, // [변경] 0 -> CREATE_NO_WINDOW
        nullptr,
        nullptr,
        &si,
        &m_pi))
    {
        return false;
    }

    m_initialized = true;

    SendCommand("uci");
    SendCommand("isready");
    for (;;)
    {
        std::string line = ReadLine();
        if (line.find("readyok") != std::string::npos)
            break;
        if (line.empty())
            break;
    }
    return true;
}

void StockfishEngine::Shutdown()
{
    if (!m_initialized)
        return;

    SendCommand("quit");

    if (m_pi.hProcess)
    {
        WaitForSingleObject(m_pi.hProcess, 1000);
        CloseHandle(m_pi.hProcess);
    }
    if (m_pi.hThread)
    {
        CloseHandle(m_pi.hThread);
    }

    if (m_hChildStdinRd)  CloseHandle(m_hChildStdinRd);
    if (m_hChildStdinWr)  CloseHandle(m_hChildStdinWr);
    if (m_hChildStdoutRd) CloseHandle(m_hChildStdoutRd);
    if (m_hChildStdoutWr) CloseHandle(m_hChildStdoutWr);

    m_hChildStdinRd = m_hChildStdinWr = nullptr;
    m_hChildStdoutRd = m_hChildStdoutWr = nullptr;
    m_initialized = false;
}

void StockfishEngine::SendCommand(const std::string& cmd)
{
    if (!m_initialized)
        return;

    std::string data = cmd + "\n";
    DWORD written = 0;
    WriteFile(m_hChildStdinWr, data.c_str(), (DWORD)data.size(), &written, nullptr);
}

std::string StockfishEngine::ReadLine()
{
    if (!m_initialized)
        return {};

    std::string result;
    char ch;
    DWORD bytesRead = 0;

    while (true)
    {
        // 파이프에서 1바이트씩 읽음
        if (!ReadFile(m_hChildStdoutRd, &ch, 1, &bytesRead, nullptr) || bytesRead == 0)
            break;
        if (ch == '\n')
            break;
        if (ch != '\r')
            result.push_back(ch);
    }
    return result;
}

std::string StockfishEngine::GetBestMove(const std::string& fen)
{
    if (!m_initialized)
        return {};

    SendCommand("position fen " + fen);
    // [수정 전] 고정 깊이 10 (약 Expert 수준, 시간 가변적)
    // SendCommand("go depth 10"); 

    // [수정 후] 시간 제한 방식 (밀리초 단위, 1000 = 1초)
    // 예: 3초 동안 생각하고 두기
    // go movetime 3000 : Elo 3500~3700, 세계 챔피언(Magnus Carlsen)도 이기기 힘든 수준
    SendCommand("go movetime 3000");

    for (;;)
    {
        std::string line = ReadLine();

        // bestmove가 나오면 파싱해서 반환
        if (line.rfind("bestmove", 0) == 0)
        {
            size_t pos = line.find(' ');
            if (pos != std::string::npos)
            {
                size_t pos2 = line.find(' ', pos + 1);
                return line.substr(pos + 1, pos2 - (pos + 1));
            }
        }
        if (line.empty())
            break;
    }
    return {};
}