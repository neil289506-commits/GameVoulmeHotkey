#pragma once
#include <QString>
#include <QJsonObject>

class ConfigManager
{
public:
    ConfigManager();

    bool load();
    bool save() const;
    void resetDefaults();

    QString muteHotKey()       const;
    QString volumeUpHotKey()   const;
    QString volumeDownHotKey() const;
    bool    startupEnabled()   const;

    void setMuteHotKey      (const QString &v);
    void setVolumeUpHotKey  (const QString &v);
    void setVolumeDownHotKey(const QString &v);
    void setStartupEnabled  (bool v);

    static QString configFilePath();
    static QString logsDirPath();

    static constexpr const char *kDefaultMute    = "Alt+Delete";
    static constexpr const char *kDefaultVolUp   = "Alt+Up";
    static constexpr const char *kDefaultVolDown = "Alt+Down";

private:
    bool ensureDir() const;

    QJsonObject m_data;
};
