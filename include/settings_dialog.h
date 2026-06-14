#pragma once
#include <QDialog>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsDialog; }
QT_END_NAMESPACE

class ConfigManager;
class HotKeyRecorder;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

    void refresh();             // 重新從設定檔載入並更新 UI

signals:
    void settingsChanged();     // 儲存後通知 MainWindow 重新註冊快捷鍵

private slots:
    void onRecordMute();
    void onRecordVolumeUp();
    void onRecordVolumeDown();
    void onKeyRecorded  (const QString &keyStr);
    void onRecordCancelled();
    void onApply();
    void onReset();
    void onOk();
    void onCancel();

private:
    void connectSignals();
    void populateUI();
    bool applySettings();
    void resetRecordButtons();

    enum class Target { None, Mute, VolumeUp, VolumeDown };

    std::unique_ptr<Ui::SettingsDialog> ui;
    std::unique_ptr<ConfigManager>      m_config;
    std::unique_ptr<HotKeyRecorder>     m_recorder;
    Target                              m_target = Target::None;
};
