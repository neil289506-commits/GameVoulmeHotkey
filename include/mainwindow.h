#pragma once
#include <QMainWindow>
#include <memory>

class TrayIcon;
class SettingsDialog;
class HotKeyManager;
class AudioManager;
class ConfigManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    static MainWindow *instance();

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent (QCloseEvent *event) override;

private slots:
    void onShowSettings();
    void onReloadConfig();
    void onSetStartup(bool enabled);
    void onAbout();
    void onExit();
    void onHotKeyPressed  (int id);
    void onSettingsChanged();
    void onRegistrationFailed(int id, const QString &reason);

private:
    void initAudio();
    void initHotKeys();
    void writeStartupRegistry(bool enabled);

    std::unique_ptr<TrayIcon>       m_trayIcon;
    std::unique_ptr<SettingsDialog> m_settingsDialog;
    std::unique_ptr<HotKeyManager>  m_hotKeyManager;
    std::unique_ptr<AudioManager>   m_audioManager;
    std::unique_ptr<ConfigManager>  m_configManager;

    static MainWindow *s_instance;
};
