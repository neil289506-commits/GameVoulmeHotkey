#include "trayicon.h"
#include <QAction>

TrayIcon::TrayIcon(QObject *parent) : QSystemTrayIcon(parent)
{
    setIcon(QIcon(QStringLiteral(":/images/icon.ico")));
    setToolTip(QStringLiteral("Game Volume Hotkey"));

    buildMenu();

    connect(this, &QSystemTrayIcon::activated,
            this, &TrayIcon::onActivated);
}

TrayIcon::~TrayIcon()
{
    delete m_menu;
}

void TrayIcon::setStartupChecked(bool checked)
{
    if (m_startupAction)
        m_startupAction->setChecked(checked);
}

void TrayIcon::buildMenu()
{
    m_menu = new QMenu();

    auto *settingsAct = m_menu->addAction(QStringLiteral("開啟設定"));
    connect(settingsAct, &QAction::triggered, this, &TrayIcon::showSettingsRequested);

    auto *reloadAct = m_menu->addAction(QStringLiteral("重新載入設定"));
    connect(reloadAct, &QAction::triggered, this, &TrayIcon::reloadConfigRequested);

    m_menu->addSeparator();

    m_startupAction = m_menu->addAction(QStringLiteral("開機自啟動"));
    m_startupAction->setCheckable(true);
    connect(m_startupAction, &QAction::toggled, this, &TrayIcon::startupToggled);

    m_menu->addSeparator();

    auto *aboutAct = m_menu->addAction(QStringLiteral("關於"));
    connect(aboutAct, &QAction::triggered, this, &TrayIcon::aboutRequested);

    m_menu->addSeparator();

    auto *exitAct = m_menu->addAction(QStringLiteral("結束"));
    connect(exitAct, &QAction::triggered, this, &TrayIcon::exitRequested);

    setContextMenu(m_menu);
}

void TrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick)
        emit showSettingsRequested();
}
