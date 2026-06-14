#include "settings_dialog.h"
#include "ui_settings_dialog.h"
#include "config_manager.h"
#include "hotkey_recorder.h"
#include "hotkey_manager.h"
#include "logger.h"
#include <QMessageBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(std::make_unique<Ui::SettingsDialog>())
    , m_config(std::make_unique<ConfigManager>())
    , m_recorder(std::make_unique<HotKeyRecorder>(this))
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("Game Volume Hotkey 設定"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(sizeHint());

    connectSignals();
    populateUI();
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::refresh()
{
    m_config->load();
    populateUI();
    resetRecordButtons();
}

void SettingsDialog::connectSignals()
{
    connect(ui->muteRecordBtn,  &QPushButton::clicked, this, &SettingsDialog::onRecordMute);
    connect(ui->upRecordBtn,    &QPushButton::clicked, this, &SettingsDialog::onRecordVolumeUp);
    connect(ui->downRecordBtn,  &QPushButton::clicked, this, &SettingsDialog::onRecordVolumeDown);

    connect(ui->applyBtn,   &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(ui->resetBtn,   &QPushButton::clicked, this, &SettingsDialog::onReset);
    connect(ui->okBtn,      &QPushButton::clicked, this, &SettingsDialog::onOk);
    connect(ui->cancelBtn,  &QPushButton::clicked, this, &SettingsDialog::onCancel);

    connect(m_recorder.get(), &HotKeyRecorder::keyRecorded,
            this, &SettingsDialog::onKeyRecorded);
    connect(m_recorder.get(), &HotKeyRecorder::cancelled,
            this, &SettingsDialog::onRecordCancelled);
}

void SettingsDialog::populateUI()
{
    ui->muteLineEdit->setText(m_config->muteHotKey());
    ui->upLineEdit  ->setText(m_config->volumeUpHotKey());
    ui->downLineEdit->setText(m_config->volumeDownHotKey());
    ui->startupCheck->setChecked(m_config->startupEnabled());
}

void SettingsDialog::resetRecordButtons()
{
    m_target = Target::None;
    ui->muteRecordBtn->setText(QStringLiteral("錄製"));
    ui->upRecordBtn  ->setText(QStringLiteral("錄製"));
    ui->downRecordBtn->setText(QStringLiteral("錄製"));
}

// ── 錄製按鈕 ───────────────────────────────────────────────────────
void SettingsDialog::onRecordMute()
{
    if (m_recorder->isRecording()) { m_recorder->stopRecording(); return; }
    resetRecordButtons();
    m_target = Target::Mute;
    ui->muteRecordBtn->setText(QStringLiteral("按下快捷鍵…"));
    m_recorder->startRecording();
}

void SettingsDialog::onRecordVolumeUp()
{
    if (m_recorder->isRecording()) { m_recorder->stopRecording(); return; }
    resetRecordButtons();
    m_target = Target::VolumeUp;
    ui->upRecordBtn->setText(QStringLiteral("按下快捷鍵…"));
    m_recorder->startRecording();
}

void SettingsDialog::onRecordVolumeDown()
{
    if (m_recorder->isRecording()) { m_recorder->stopRecording(); return; }
    resetRecordButtons();
    m_target = Target::VolumeDown;
    ui->downRecordBtn->setText(QStringLiteral("按下快捷鍵…"));
    m_recorder->startRecording();
}

void SettingsDialog::onKeyRecorded(const QString &keyStr)
{
    // 驗證能否解析（避免錄到無效鍵）
    UINT vk = 0, mods = 0;
    if (!HotKeyManager::parseKeyString(keyStr, vk, mods))
    {
        QMessageBox::warning(this, QStringLiteral("無效按鍵"),
            QStringLiteral("無法識別此按鍵組合：%1").arg(keyStr));
        resetRecordButtons();
        return;
    }

    switch (m_target)
    {
    case Target::Mute:      ui->muteLineEdit->setText(keyStr); break;
    case Target::VolumeUp:  ui->upLineEdit  ->setText(keyStr); break;
    case Target::VolumeDown:ui->downLineEdit ->setText(keyStr); break;
    default: break;
    }
    resetRecordButtons();
}

void SettingsDialog::onRecordCancelled()
{
    resetRecordButtons();
}

// ── 對話框按鈕 ─────────────────────────────────────────────────────
bool SettingsDialog::applySettings()
{
    QString muteKey = ui->muteLineEdit->text().trimmed();
    QString upKey   = ui->upLineEdit  ->text().trimmed();
    QString downKey = ui->downLineEdit ->text().trimmed();

    if (muteKey.isEmpty() || upKey.isEmpty() || downKey.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("錯誤"),
            QStringLiteral("快捷鍵不能為空。"));
        return false;
    }

    // 確認都能解析
    UINT vk = 0, mods = 0;
    if (!HotKeyManager::parseKeyString(muteKey, vk, mods) ||
        !HotKeyManager::parseKeyString(upKey,   vk, mods) ||
        !HotKeyManager::parseKeyString(downKey, vk, mods))
    {
        QMessageBox::warning(this, QStringLiteral("錯誤"),
            QStringLiteral("快捷鍵格式無效，請重新錄製。"));
        return false;
    }

    m_config->setMuteHotKey      (muteKey);
    m_config->setVolumeUpHotKey  (upKey);
    m_config->setVolumeDownHotKey(downKey);
    m_config->setStartupEnabled  (ui->startupCheck->isChecked());

    if (!m_config->save())
    {
        QMessageBox::critical(this, QStringLiteral("錯誤"),
            QStringLiteral("設定檔儲存失敗。"));
        return false;
    }

    Logger::get().info(QStringLiteral("Settings applied."));
    emit settingsChanged();
    return true;
}

void SettingsDialog::onApply()
{
    if (applySettings())
        QMessageBox::information(this, QStringLiteral("成功"),
            QStringLiteral("設定已套用並立即生效。"));
}

void SettingsDialog::onReset()
{
    if (QMessageBox::question(this, QStringLiteral("還原預設"),
            QStringLiteral("確定要還原為預設快捷鍵嗎？"),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        m_config->resetDefaults();
        m_config->save();
        populateUI();
        emit settingsChanged();
        Logger::get().info(QStringLiteral("Settings reset to defaults."));
    }
}

void SettingsDialog::onOk()
{
    if (applySettings())
        accept();
}

void SettingsDialog::onCancel()
{
    m_recorder->stopRecording();
    reject();
}
