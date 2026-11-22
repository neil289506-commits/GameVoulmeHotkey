#include "hotkey_manager.h"
#include "logger.h"
#include <QCoreApplication>
#include <QStringList>

// ── 按鍵名稱對照表（Windows SDK 沒有 VK_A~VK_Z，字母直接用 ASCII） ──
namespace {

struct KeyEntry {
    const char *parseName;    // 大寫，用於解析
    const char *displayName;  // 顯示用
    UINT        vk;
};

constexpr KeyEntry kSpecialKeys[] = {
    {"DELETE",      "Delete",      VK_DELETE},
    {"UP",          "Up",          VK_UP},
    {"DOWN",        "Down",        VK_DOWN},
    {"LEFT",        "Left",        VK_LEFT},
    {"RIGHT",       "Right",       VK_RIGHT},
    {"SPACE",       "Space",       VK_SPACE},
    {"ENTER",       "Enter",       VK_RETURN},
    {"RETURN",      "Enter",       VK_RETURN},
    {"ESCAPE",      "Escape",      VK_ESCAPE},
    {"ESC",         "Escape",      VK_ESCAPE},
    {"TAB",         "Tab",         VK_TAB},
    {"BACKSPACE",   "Backspace",   VK_BACK},
    {"BACK",        "Backspace",   VK_BACK},
    {"INSERT",      "Insert",      VK_INSERT},
    {"HOME",        "Home",        VK_HOME},
    {"END",         "End",         VK_END},
    {"PAGEUP",      "PageUp",      VK_PRIOR},
    {"PRIOR",       "PageUp",      VK_PRIOR},
    {"PAGEDOWN",    "PageDown",    VK_NEXT},
    {"NEXT",        "PageDown",    VK_NEXT},
    {"F1",  "F1",  VK_F1},  {"F2",  "F2",  VK_F2},
    {"F3",  "F3",  VK_F3},  {"F4",  "F4",  VK_F4},
    {"F5",  "F5",  VK_F5},  {"F6",  "F6",  VK_F6},
    {"F7",  "F7",  VK_F7},  {"F8",  "F8",  VK_F8},
    {"F9",  "F9",  VK_F9},  {"F10", "F10", VK_F10},
    {"F11", "F11", VK_F11}, {"F12", "F12", VK_F12},
    {"NUMPAD0", "Num0", VK_NUMPAD0}, {"NUMPAD1", "Num1", VK_NUMPAD1},
    {"NUMPAD2", "Num2", VK_NUMPAD2}, {"NUMPAD3", "Num3", VK_NUMPAD3},
    {"NUMPAD4", "Num4", VK_NUMPAD4}, {"NUMPAD5", "Num5", VK_NUMPAD5},
    {"NUMPAD6", "Num6", VK_NUMPAD6}, {"NUMPAD7", "Num7", VK_NUMPAD7},
    {"NUMPAD8", "Num8", VK_NUMPAD8}, {"NUMPAD9", "Num9", VK_NUMPAD9},
    {"MULTIPLY","Num*", VK_MULTIPLY}, {"ADD",    "Num+", VK_ADD},
    {"SUBTRACT","Num-", VK_SUBTRACT}, {"DECIMAL","Num.", VK_DECIMAL},
    {"DIVIDE",  "Num/", VK_DIVIDE},
    {"CAPSLOCK","CapsLock", VK_CAPITAL},
    {"PAUSE",   "Pause",    VK_PAUSE},
    {"PRINTSCREEN","PrintScreen", VK_SNAPSHOT},
};

} // namespace

// ─────────────────────────────────────────────────────────────────
HotKeyManager::HotKeyManager(QObject *parent) : QObject(parent)
{
    // 把自己安裝為全域原生事件過濾器，攔截 WM_HOTKEY
    qApp->installNativeEventFilter(this);
    Logger::get().info(QStringLiteral("HotKeyManager created."));
}

HotKeyManager::~HotKeyManager()
{
    unregisterAll();
    qApp->removeNativeEventFilter(this);
}

// ─────────────────────────────────────────────────────────────────
bool HotKeyManager::registerHotKey(int id, const QString &keyString)
{
    UINT vk = 0, mods = 0;
    if (!parseKeyString(keyString, vk, mods))
    {
        Logger::get().error(
            QStringLiteral("Cannot parse key string: '%1'").arg(keyString));
        emit registrationFailed(id,
            QStringLiteral("無效的快捷鍵格式：%1").arg(keyString));
        return false;
    }

    // 若已註冊同一 id，先取消
    if (m_hotKeys.count(id))
        UnregisterHotKey(nullptr, id);

    // MOD_NOREPEAT 防止長按重複觸發
    if (!::RegisterHotKey(nullptr, id, mods | MOD_NOREPEAT, vk))
    {
        DWORD err = GetLastError();
        Logger::get().error(
            QStringLiteral("RegisterHotKey failed id=%1 key='%2' err=%3")
            .arg(id).arg(keyString).arg(err));
        emit registrationFailed(id,
            QStringLiteral("快捷鍵「%1」已被其他程式佔用").arg(keyString));
        return false;
    }

    m_hotKeys[id] = {vk, mods};
    Logger::get().info(
        QStringLiteral("Hotkey registered id=%1 key='%2'").arg(id).arg(keyString));
    return true;
}

bool HotKeyManager::unregisterHotKey(int id)
{
    auto it = m_hotKeys.find(id);
    if (it == m_hotKeys.end()) return false;
    UnregisterHotKey(nullptr, id);
    m_hotKeys.erase(it);
    Logger::get().info(QStringLiteral("Hotkey unregistered id=%1").arg(id));
    return true;
}

void HotKeyManager::unregisterAll()
{
    for (auto &[id, _] : m_hotKeys)
        UnregisterHotKey(nullptr, id);
    m_hotKeys.clear();
    Logger::get().info(QStringLiteral("All hotkeys unregistered."));
}

// ─────────────────────────────────────────────────────────────────
bool HotKeyManager::nativeEventFilter(const QByteArray &eventType,
                                      void *message,
                                      qintptr *result)
{
    Q_UNUSED(result)
    if (eventType != "windows_generic_MSG") return false;

    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_HOTKEY)
    {
        int id = static_cast<int>(msg->wParam);
        if (m_hotKeys.count(id))
        {
            emit hotKeyPressed(id);
            return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────
// static
bool HotKeyManager::parseKeyString(const QString &keyString, UINT &vk, UINT &mods)
{
    vk = 0; mods = 0;
    QString s = keyString.trimmed();
    if (s.isEmpty()) return false;

    QStringList parts = s.split('+', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return false;

    // 最後一個 token 是主鍵，其餘是修飾符
    QString keyName = parts.last().trimmed().toUpper();

    for (int i = 0; i < parts.size() - 1; ++i)
    {
        QString m = parts[i].trimmed().toUpper();
        if      (m == QLatin1String("CTRL")  || m == QLatin1String("CONTROL")) mods |= MOD_CONTROL;
        else if (m == QLatin1String("ALT"))                                     mods |= MOD_ALT;
        else if (m == QLatin1String("SHIFT"))                                   mods |= MOD_SHIFT;
        else if (m == QLatin1String("WIN")   || m == QLatin1String("WINDOWS")) mods |= MOD_WIN;
    }

    // 特殊鍵查表
    for (const auto &e : kSpecialKeys)
    {
        if (keyName == QLatin1String(e.parseName))
        {
            vk = e.vk;
            return true;
        }
    }

    // 單字母 A-Z（ASCII 0x41-0x5A = VK code）
    if (keyName.length() == 1)
    {
        ushort c = keyName.at(0).unicode();
        if (c >= 'A' && c <= 'Z') { vk = c; return true; }
        if (c >= '0' && c <= '9') { vk = c; return true; }
    }

    return false;
}

// static
QString HotKeyManager::formatKeyString(UINT vk, UINT mods)
{
    QString result;
    if (mods & MOD_CONTROL) result += QStringLiteral("Ctrl+");
    if (mods & MOD_ALT)     result += QStringLiteral("Alt+");
    if (mods & MOD_SHIFT)   result += QStringLiteral("Shift+");
    if (mods & MOD_WIN)     result += QStringLiteral("Win+");

    // 特殊鍵查表
    for (const auto &e : kSpecialKeys)
    {
        if (vk == e.vk)
        {
            result += QLatin1String(e.displayName);
            return result;
        }
    }

    // A-Z
    if (vk >= 0x41 && vk <= 0x5A) { result += QChar(static_cast<char>(vk)); return result; }
    // 0-9
    if (vk >= 0x30 && vk <= 0x39) { result += QChar(static_cast<char>(vk)); return result; }

    result += QStringLiteral("VK_%1").arg(vk, 2, 16, QChar('0')).toUpper();
    return result;
}
