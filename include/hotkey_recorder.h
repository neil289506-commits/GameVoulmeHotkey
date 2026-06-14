#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <QObject>
#include <QString>

class HotKeyRecorder : public QObject
{
    Q_OBJECT

public:
    explicit HotKeyRecorder(QObject *parent = nullptr);
    ~HotKeyRecorder() override;

    void startRecording();
    void stopRecording();          // 手動取消
    bool isRecording() const { return m_recording; }

signals:
    void keyRecorded(const QString &keyString);
    void cancelled();

private:
    void internalStop(bool emitCancel);

    static LRESULT CALLBACK hookProc(int nCode, WPARAM wParam, LPARAM lParam);

    bool  m_recording = false;
    HHOOK m_hook      = nullptr;

    static HotKeyRecorder *s_current;
};
