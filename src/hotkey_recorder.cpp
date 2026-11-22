#include "hotkey_recorder.h"
#include "hotkey_manager.h"
#include "logger.h"

HotKeyRecorder *HotKeyRecorder::s_current = nullptr;

HotKeyRecorder::HotKeyRecorder(QObject *parent) : QObject(parent) {}

HotKeyRecorder::~HotKeyRecorder()
{
    internalStop(false);
}

void HotKeyRecorder::startRecording()
{
    if (m_recording) return;

    // 若有其他 recorder 正在錄製，先停止它
    if (s_current && s_current != this)
        s_current->internalStop(true);

    s_current   = this;
    m_recording = true;

    m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, hookProc, nullptr, 0);
    if (!m_hook)
    {
        Logger::get().error(
            QStringLiteral("SetWindowsHookEx WH_KEYBOARD_LL failed: %1").arg(GetLastError()));
        m_recording = false;
        s_current   = nullptr;
        emit cancelled();
    }
    else
    {
        Logger::get().info(QStringLiteral("HotKeyRecorder started."));
    }
}

void HotKeyRecorder::stopRecording()
{
    internalStop(true);
}

void HotKeyRecorder::internalStop(bool emitCancel)
{
    if (!m_recording) return;
    m_recording = false;

    if (m_hook)
    {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
    }
    if (s_current == this)
        s_current = nullptr;

    Logger::get().info(QStringLiteral("HotKeyRecorder stopped."));
    if (emitCancel)
        emit cancelled();
}

// static
LRESULT CALLBACK HotKeyRecorder::hookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && s_current && s_current->m_recording)
    {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            auto *kb = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
            UINT  vk = kb->vkCode;

            // 跳過單獨的修飾符按鍵
            if (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
                vk == VK_MENU    || vk == VK_LMENU    || vk == VK_RMENU    ||
                vk == VK_SHIFT   || vk == VK_LSHIFT   || vk == VK_RSHIFT   ||
                vk == VK_LWIN    || vk == VK_RWIN)
            {
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }

            // 從目前鍵狀態組合修飾符
            UINT mods = 0;
            if (GetKeyState(VK_CONTROL) & 0x8000) mods |= MOD_CONTROL;
            if (GetKeyState(VK_MENU)    & 0x8000) mods |= MOD_ALT;
            if (GetKeyState(VK_SHIFT)   & 0x8000) mods |= MOD_SHIFT;
            if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) mods |= MOD_WIN;

            QString keyStr = HotKeyManager::formatKeyString(vk, mods);
            Logger::get().info(
                QStringLiteral("Key recorded: '%1'").arg(keyStr));

            // 先停止（不 emit cancelled），再 emit keyRecorded
            HotKeyRecorder *rec = s_current;
            rec->internalStop(false);
            emit rec->keyRecorded(keyStr);

            return 1; // 吃掉此按鍵，不往下傳
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
