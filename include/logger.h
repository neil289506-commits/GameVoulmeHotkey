#pragma once
#include <QString>
#include <QFile>
#include <mutex>

class Logger
{
public:
    static Logger &get();

    void setPath(const QString &path);

    void debug  (const QString &msg);
    void info   (const QString &msg);
    void warning(const QString &msg);
    void error  (const QString &msg);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    void write(const char *level, const QString &msg);
    bool ensureOpen();

    QFile      m_file;
    std::mutex m_mutex;
};
