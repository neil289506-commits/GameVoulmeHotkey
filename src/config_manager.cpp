#include "config_manager.h"
#include "logger.h"
#include <QStandardPaths>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>

ConfigManager::ConfigManager()
{
    resetDefaults();
}

// static
QString ConfigManager::configFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/config.json");
}

// static
QString ConfigManager::logsDirPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/logs");
}

bool ConfigManager::ensureDir() const
{
    QFileInfo fi(configFilePath());
    QDir dir = fi.absoluteDir();
    if (dir.exists()) return true;
    return dir.mkpath(QStringLiteral("."));
}

bool ConfigManager::load()
{
    QString path = configFilePath();
    QFile f(path);
    if (!f.exists())
    {
        Logger::get().info(QStringLiteral("config.json not found, using defaults."));
        return true; // 已在建構子呼叫 resetDefaults()
    }
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        Logger::get().error(QStringLiteral("Cannot open config: ") + path);
        return false;
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (doc.isNull())
    {
        Logger::get().error(QStringLiteral("Config JSON parse error: ") + err.errorString());
        return false;
    }
    // 把讀到的值合併進來（保留 default 對應沒有的 key）
    QJsonObject loaded = doc.object();
    for (auto it = loaded.constBegin(); it != loaded.constEnd(); ++it)
        m_data[it.key()] = it.value();

    Logger::get().info(QStringLiteral("Config loaded."));
    return true;
}

bool ConfigManager::save() const
{
    if (!ensureDir()) return false;

    QFile f(configFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        Logger::get().error(QStringLiteral("Cannot write config: ") + configFilePath());
        return false;
    }
    f.write(QJsonDocument(m_data).toJson());
    f.close();
    Logger::get().info(QStringLiteral("Config saved."));
    return true;
}

void ConfigManager::resetDefaults()
{
    m_data = QJsonObject{
        {QStringLiteral("mute_hotkey"),        QLatin1String(kDefaultMute)},
        {QStringLiteral("volume_up_hotkey"),   QLatin1String(kDefaultVolUp)},
        {QStringLiteral("volume_down_hotkey"), QLatin1String(kDefaultVolDown)},
        {QStringLiteral("startup"),            false},
    };
}

QString ConfigManager::muteHotKey()       const { return m_data.value(QStringLiteral("mute_hotkey")).toString(QLatin1String(kDefaultMute)); }
QString ConfigManager::volumeUpHotKey()   const { return m_data.value(QStringLiteral("volume_up_hotkey")).toString(QLatin1String(kDefaultVolUp)); }
QString ConfigManager::volumeDownHotKey() const { return m_data.value(QStringLiteral("volume_down_hotkey")).toString(QLatin1String(kDefaultVolDown)); }
bool    ConfigManager::startupEnabled()   const { return m_data.value(QStringLiteral("startup")).toBool(false); }

void ConfigManager::setMuteHotKey      (const QString &v) { m_data[QStringLiteral("mute_hotkey")]        = v; }
void ConfigManager::setVolumeUpHotKey  (const QString &v) { m_data[QStringLiteral("volume_up_hotkey")]   = v; }
void ConfigManager::setVolumeDownHotKey(const QString &v) { m_data[QStringLiteral("volume_down_hotkey")] = v; }
void ConfigManager::setStartupEnabled  (bool v)           { m_data[QStringLiteral("startup")]             = v; }
