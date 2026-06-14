#include "audio_manager.h"
#include "logger.h"
#include <objbase.h>
#include <algorithm>

AudioManager::AudioManager(QObject *parent) : QObject(parent) {}

AudioManager::~AudioManager()
{
    shutdown();
}

bool AudioManager::initialize()
{
    // Qt 主執行緒通常已初始化為 STA；CoInitializeEx 回傳 S_FALSE 代表「已初始化，可用」
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        Logger::get().error(
            QStringLiteral("CoInitializeEx failed: 0x%1").arg((quint32)hr, 8, 16));
        return false;
    }
    m_comInit = (hr == S_OK); // 只有我們自己初始化才需要 CoUninitialize

    // IMMDeviceEnumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          reinterpret_cast<void **>(&m_enumerator));
    if (FAILED(hr))
    {
        Logger::get().error(
            QStringLiteral("CoCreateInstance MMDeviceEnumerator failed: 0x%1").arg((quint32)hr, 8, 16));
        if (m_comInit) { CoUninitialize(); m_comInit = false; }
        return false;
    }

    // 預設播放端點
    hr = m_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device);
    if (FAILED(hr))
    {
        Logger::get().error(
            QStringLiteral("GetDefaultAudioEndpoint failed: 0x%1").arg((quint32)hr, 8, 16));
        m_enumerator->Release(); m_enumerator = nullptr;
        if (m_comInit) { CoUninitialize(); m_comInit = false; }
        return false;
    }

    // IAudioEndpointVolume
    hr = m_device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL,
                            nullptr,
                            reinterpret_cast<void **>(&m_endpointVolume));
    if (FAILED(hr))
    {
        Logger::get().error(
            QStringLiteral("Activate IAudioEndpointVolume failed: 0x%1").arg((quint32)hr, 8, 16));
        m_device->Release();     m_device = nullptr;
        m_enumerator->Release(); m_enumerator = nullptr;
        if (m_comInit) { CoUninitialize(); m_comInit = false; }
        return false;
    }

    Logger::get().info(QStringLiteral("AudioManager initialized."));
    return true;
}

void AudioManager::shutdown()
{
    if (m_endpointVolume) { m_endpointVolume->Release(); m_endpointVolume = nullptr; }
    if (m_device)         { m_device->Release();         m_device = nullptr; }
    if (m_enumerator)     { m_enumerator->Release();     m_enumerator = nullptr; }
    if (m_comInit)        { CoUninitialize();             m_comInit = false; }
    Logger::get().info(QStringLiteral("AudioManager shutdown."));
}

bool AudioManager::volumeUp(float step)
{
    return setVolume(currentVolume() + step);
}

bool AudioManager::volumeDown(float step)
{
    return setVolume(currentVolume() - step);
}

bool AudioManager::toggleMute()
{
    if (!m_endpointVolume) return false;
    BOOL muted = FALSE;
    m_endpointVolume->GetMute(&muted);
    HRESULT hr = m_endpointVolume->SetMute(!muted, nullptr);
    if (SUCCEEDED(hr))
    {
        bool nowMuted = !muted;
        Logger::get().info(
            QStringLiteral("Mute toggled -> %1").arg(nowMuted ? "muted" : "unmuted"));
        emit muteStateChanged(nowMuted);
        return true;
    }
    Logger::get().error(QStringLiteral("SetMute failed: 0x%1").arg((quint32)hr, 8, 16));
    return false;
}

float AudioManager::currentVolume() const
{
    if (!m_endpointVolume) return 0.f;
    float v = 0.f;
    m_endpointVolume->GetMasterVolumeLevelScalar(&v);
    return v;
}

bool AudioManager::isMuted() const
{
    if (!m_endpointVolume) return false;
    BOOL muted = FALSE;
    m_endpointVolume->GetMute(&muted);
    return muted == TRUE;
}

bool AudioManager::setVolume(float v)
{
    if (!m_endpointVolume) return false;
    v = std::clamp(v, 0.f, 1.f);
    HRESULT hr = m_endpointVolume->SetMasterVolumeLevelScalar(v, nullptr);
    if (SUCCEEDED(hr))
    {
        Logger::get().debug(
            QStringLiteral("Volume set to %1%").arg(qRound(v * 100)));
        emit volumeChanged(v);
        return true;
    }
    Logger::get().error(
        QStringLiteral("SetMasterVolumeLevelScalar failed: 0x%1").arg((quint32)hr, 8, 16));
    return false;
}
