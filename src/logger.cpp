#include "logger.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>

Logger &Logger::get()
{
    static Logger inst;
    return inst;
}

Logger::~Logger()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_file.isOpen())
        m_file.close();
}

void Logger::setPath(const QString &path)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_file.isOpen())
        m_file.close();
    m_file.setFileName(path);
}

void Logger::debug  (const QString &msg) { write("DEBUG",   msg); }
void Logger::info   (const QString &msg) { write("INFO",    msg); }
void Logger::warning(const QString &msg) { write("WARNING", msg); }
void Logger::error  (const QString &msg) { write("ERROR",   msg); }

bool Logger::ensureOpen()
{
    if (m_file.isOpen())  return true;
    if (m_file.fileName().isEmpty()) return false;

    QFileInfo fi(m_file.fileName());
    QDir dir = fi.absoluteDir();
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));

    return m_file.open(QIODevice::Append | QIODevice::Text);
}

void Logger::write(const char *level, const QString &msg)
{
    QString line = QStringLiteral("[%1][%2] %3\n")
        .arg(QDateTime::currentDateTime()
             .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz")))
        .arg(QLatin1String(level), -7)
        .arg(msg);

    std::lock_guard<std::mutex> lk(m_mutex);
    if (ensureOpen())
    {
        m_file.write(line.toUtf8());
        m_file.flush();
    }
}
