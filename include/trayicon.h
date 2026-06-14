#pragma once
#include <QSystemTrayIcon>
#include <QMenu>

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    explicit TrayIcon(QObject *parent = nullptr);
    ~TrayIcon() override;

    void setStartupChecked(bool checked);

signals:
    void showSettingsRequested();
    void reloadConfigRequested();
    void startupToggled(bool enabled);
    void aboutRequested();
    void exitRequested();

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void buildMenu();

    QMenu   *m_menu          = nullptr;
    QAction *m_startupAction = nullptr;
};
