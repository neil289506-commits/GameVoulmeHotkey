#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <QObject>

class AudioManager : public QObject
{
    Q_OBJECT

public:
    explicit AudioManager(QObject *parent = nullptr);
    ~AudioManager() override;

    bool initialize();
    void shutdown();

    bool  volumeUp  (float step = 0.05f);
    bool  volumeDown(float step = 0.05f);
    bool  toggleMute();

    float currentVolume() const;
    bool  isMuted()       const;

signals:
    void volumeChanged   (float volume);
    void muteStateChanged(bool muted);

private:
    bool setVolume(float v);

    IMMDeviceEnumerator  *m_enumerator     = nullptr;
    IMMDevice            *m_device         = nullptr;
    IAudioEndpointVolume *m_endpointVolume = nullptr;
    bool                  m_comInit        = false;
};
