#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <QString>

class SingleInstance
{
public:
    explicit SingleInstance(const QString &mutexName);
    ~SingleInstance();

    bool isFirst()          const { return m_first; }
    bool activateExisting() const;   // 喚醒第一個實例

private:
    HANDLE m_mutex = nullptr;
    bool   m_first = false;
};
