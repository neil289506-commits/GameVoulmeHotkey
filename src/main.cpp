#include <QApplication>
#include "mainwindow.h"
#include "single_instance.h"
#include "config_manager.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("GameVolumeHotkey"));
    app.setOrganizationName(QStringLiteral("GameVolumeHotkey"));
    app.setQuitOnLastWindowClosed(false);   // 關閉設定視窗不退出

    // 設定 Logger 路徑
    Logger &log = Logger::get();
    log.setPath(ConfigManager::logsDirPath() + QStringLiteral("/app.log"));
    log.info(QStringLiteral("=== Application Start ==="));

    // 單實例防護
    SingleInstance guard(QStringLiteral("GameVolumeHotkey_MUTEX_{8F3A1C2D-4B5E-4F7A-9D8C-0E1F2A3B4C5D}"));
    if (!guard.isFirst())
    {
        log.info(QStringLiteral("Another instance detected, activating it."));
        guard.activateExisting();
        return 0;
    }

    MainWindow window;
    // MainWindow 建構時自動 hide()，直接常駐 System Tray

    log.info(QStringLiteral("Entering event loop."));
    int ret = app.exec();
    log.info(QStringLiteral("=== Application Exit (code=%1) ===").arg(ret));
    return ret;
}
