#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <QObject>
#include <QAbstractNativeEventFilter>
#include <unordered_map>

class HotKeyManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    static constexpr int ID_MUTE        = 1;
    static constexpr int ID_VOLUME_UP   = 2;
    static constexpr int ID_VOLUME_DOWN = 3;

    explicit HotKeyManager(QObject *parent = nullptr);
    ~HotKeyManager() override;

    bool registerHotKey  (int id, const QString &keyString);
    bool unregisterHotKey(int id);
    void unregisterAll   ();

    // 工具：字串 <-> (vk, mods) 互轉
    static bool    parseKeyString (const QString &keyString, UINT &vk, UINT &mods);
    static QString formatKeyString(UINT vk, UINT mods);

    // QAbstractNativeEventFilter override
    bool nativeEventFilter(const QByteArray &eventType,
                           void *message,
                           qintptr *result) override;

signals:
    void hotKeyPressed      (int id);
    void registrationFailed (int id, const QString &reason);

private:
    struct Entry { UINT vk; UINT mods; };
    std::unordered_map<int, Entry> m_hotKeys;
};
