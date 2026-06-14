#include "single_instance.h"
#include "logger.h"

SingleInstance::SingleInstance(const QString &mutexName)
{
    std::wstring wname = mutexName.toStdWString();
    m_mutex = CreateMutexW(nullptr, FALSE, wname.c_str());
    if (m_mutex)
        m_first = (GetLastError() != ERROR_ALREADY_EXISTS);
    else
        Logger::get().error(QStringLiteral("CreateMutex failed: %1").arg(GetLastError()));
}

SingleInstance::~SingleInstance()
{
    if (m_mutex)
    {
        CloseHandle(m_mutex);
        m_mutex = nullptr;
    }
}

bool SingleInstance::activateExisting() const
{
    // 透過 WM_USER 訊息喚醒已存在的設定視窗
    // 先嘗試以標題找視窗（設定視窗會設好 WindowTitle）
    HWND hw = FindWindowW(nullptr, L"Game Volume Hotkey \u8a2d\u5b9a");
    if (!hw)
        hw = FindWindowW(nullptr, L"Game Volume Hotkey");
    if (hw)
    {
        ShowWindow(hw, SW_RESTORE);
        SetForegroundWindow(hw);
        return true;
    }
    return false;
}
