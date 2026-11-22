#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "mainwindow.h"
#include "trayicon.h"
#include "settings_dialog.h"
#include "hotkey_manager.h"
#include "audio_manager.h"
#include "config_manager.h"
#include "logger.h"

#include <QApplication>
#include <QCloseEvent>
#include <QMessageBox>

MainWindow *MainWindow::s_instance = nullptr;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_trayIcon      (std::make_unique<TrayIcon>(this))
    , m_settingsDialog(std::make_unique<SettingsDialog>())
    , m_hotKeyManager (std::make_unique<HotKeyManager>(this))
    , m_audioManager  (std::make_unique<AudioManager>(this))
    , m_configManager (std::make_unique<ConfigManager>())
{
    s_instance = this;
    setWindowTitle(QStringLiteral("Game Volume Hotkey"));
    setWindowIcon(QIcon(QStringLiteral(":/images/icon.ico")));

    m_configManager->load();

    initAudio();
    initHotKeys();

    m_trayIcon->setStartupChecked(m_configManager->startupEnabled());
    m_trayIcon->show();

    // TrayIcon signals
    connect(m_trayIcon.get(), &TrayIcon::showSettingsRequested,
            this, &MainWindow::onShowSettings);
    connect(m_trayIcon.get(), &TrayIcon::reloadConfigRequested,
            this, &MainWindow::onReloadConfig);
    connect(m_trayIcon.get(), &TrayIcon::startupToggled,
            this, &MainWindow::onSetStartup);
    connect(m_trayIcon.get(), &TrayIcon::aboutRequested,
            this, &MainWindow::onAbout);
    connect(m_trayIcon.get(), &TrayIcon::exitRequested,
            this, &MainWindow::onExit);

    // HotKey signals
    connect(m_hotKeyManager.get(), &HotKeyManager::hotKeyPressed,
            this, &MainWindow::onHotKeyPressed);
    connect(m_hotKeyManager.get(), &HotKeyManager::registrationFailed,
            this, &MainWindow::onRegistrationFailed);

    // Settings signals
    connect(m_settingsDialog.get(), &SettingsDialog::settingsChanged,
            this, &MainWindow::onSettingsChanged);

    hide();   // 啟動後直接常駐托盤，不顯示視窗
    Logger::get().info(QStringLiteral("MainWindow ready."));
}

MainWindow::~MainWindow()
{
    s_instance = nullptr;
}

MainWindow *MainWindow::instance() { return s_instance; }

// ── Audio ────────────────────────────────────────────────────────
void MainWindow::initAudio()
{
    if (!m_audioManager->initialize())
    {
        Logger::get().error(QStringLiteral("AudioManager initialization failed."));
        m_trayIcon->showMessage(
            QStringLiteral("Game Volume Hotkey"),
            QStringLiteral("音頻系統初始化失敗，音量控制將無法使用。"),
            QSystemTrayIcon::Warning, 5000);
    }
}

// ── HotKeys ──────────────────────────────────────────────────────
void MainWindow::initHotKeys()
{
    m_hotKeyManager->unregisterAll();

    auto tryReg = [this](int id, const QString &key) {
        if (!m_hotKeyManager->registerHotKey(id, key))
        {
            Logger::get().warning(
                QStringLiteral("Failed to register id=%1 key='%2'").arg(id).arg(key));
        }
    };

    tryReg(HotKeyManager::ID_MUTE,        m_configManager->muteHotKey());
    tryReg(HotKeyManager::ID_VOLUME_UP,   m_configManager->volumeUpHotKey());
    tryReg(HotKeyManager::ID_VOLUME_DOWN, m_configManager->volumeDownHotKey());
}

// ── TrayIcon slots ───────────────────────────────────────────────
void MainWindow::onShowSettings()
{
    m_settingsDialog->refresh();
    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}

void MainWindow::onReloadConfig()
{
    m_configManager->load();
    initHotKeys();
    m_trayIcon->setStartupChecked(m_configManager->startupEnabled());
    m_trayIcon->showMessage(
        QStringLiteral("Game Volume Hotkey"),
        QStringLiteral("設定已重新載入"),
        QSystemTrayIcon::Information, 2000);
    Logger::get().info(QStringLiteral("Config reloaded."));
}

void MainWindow::onSetStartup(bool enabled)
{
    m_configManager->setStartupEnabled(enabled);
    m_configManager->save();
    writeStartupRegistry(enabled);
}

void MainWindow::writeStartupRegistry(bool enabled)
{
    const wchar_t *runKey =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, runKey, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
    {
        Logger::get().error(QStringLiteral("RegOpenKeyEx Run failed."));
        return;
    }
    if (enabled)
    {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        RegSetValueExW(hKey, L"GameVolumeHotkey", 0, REG_SZ,
            reinterpret_cast<const BYTE *>(exePath),
            static_cast<DWORD>((wcslen(exePath) + 1) * sizeof(wchar_t)));
        Logger::get().info(QStringLiteral("Startup registry key set."));
    }
    else
    {
        RegDeleteValueW(hKey, L"GameVolumeHotkey");
        Logger::get().info(QStringLiteral("Startup registry key removed."));
    }
    RegCloseKey(hKey);
}

void MainWindow::onAbout()
{
    QMessageBox::about(nullptr,
        QStringLiteral("關於 Game Volume Hotkey"),
        QStringLiteral(
            "<b>Game Volume Hotkey v1.0</b><br><br>"
            "在全螢幕遊戲（DirectX / OpenGL / Vulkan）中"
            "以全域快捷鍵控制 Windows 音量。<br><br>"
            "預設快捷鍵：<br>"
            "&nbsp;&nbsp;靜音：<b>Alt + Delete</b><br>"
            "&nbsp;&nbsp;音量增加：<b>Alt + ↑</b><br>"
            "&nbsp;&nbsp;音量減少：<b>Alt + ↓</b>"
        )
    );
}

void MainWindow::onExit()
{
    Logger::get().info(QStringLiteral("Exiting application."));
    QApplication::quit();
}

// ── HotKey 觸發 ───────────────────────────────────────────────────
void MainWindow::onHotKeyPressed(int id)
{
    Logger::get().debug(QStringLiteral("HotKey pressed id=%1").arg(id));
    switch (id)
    {
    case HotKeyManager::ID_MUTE:        m_audioManager->toggleMute();   break;
    case HotKeyManager::ID_VOLUME_UP:   m_audioManager->volumeUp();     break;
    case HotKeyManager::ID_VOLUME_DOWN: m_audioManager->volumeDown();   break;
    }
}

void MainWindow::onRegistrationFailed(int id, const QString &reason)
{
    Q_UNUSED(id)
    m_trayIcon->showMessage(
        QStringLiteral("Game Volume Hotkey — 快捷鍵衝突"),
        reason,
        QSystemTrayIcon::Warning, 5000);
}

// ── Settings changed ─────────────────────────────────────────────
void MainWindow::onSettingsChanged()
{
    m_configManager->load();
    initHotKeys();
    m_trayIcon->setStartupChecked(m_configManager->startupEnabled());
    writeStartupRegistry(m_configManager->startupEnabled());
}

// ── Window events ─────────────────────────────────────────────────
void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
        if (windowState() & Qt::WindowMinimized)
            hide();
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon && m_trayIcon->isVisible())
    {
        hide();
        event->ignore();
    }
    else
    {
        event->accept();
    }
}
