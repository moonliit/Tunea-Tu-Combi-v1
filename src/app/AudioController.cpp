#include "app/AudioController.hpp"

#include <algorithm>

void AudioController::Initialize() {
    InitAudioDevice();
    m_music = {
        LoadMusicStream("assets/audio/menu.wav"),
        LoadMusicStream("assets/audio/level_1.wav"),
        LoadMusicStream("assets/audio/level_2.wav"),
        LoadMusicStream("assets/audio/level_3.wav")
    };
    SetMusicPitch(m_music[1], 1.08f);
    m_radioStatic = LoadSound("assets/audio/radio_static.wav");
    SetSoundVolume(m_radioStatic, 0.15f);
    m_uiClick = LoadSound("assets/audio/sfx/ui_click.wav");
    m_partSelect = LoadSound("assets/audio/sfx/part_select.wav");
    m_partPlace = LoadSound("assets/audio/sfx/part_place.wav");
    m_sailSound = LoadSound("assets/audio/sfx/sail.wav");
    m_horns = {
        LoadSound("assets/audio/sfx/combi_horn_1.wav"),
        LoadSound("assets/audio/sfx/combi_horn_2.wav"),
        LoadSound("assets/audio/sfx/combi_horn_3.wav")
    };
    m_fanSound = LoadMusicStream("assets/audio/sfx/fan.wav");
    m_wheelSound = LoadMusicStream("assets/audio/sfx/wheel.wav");
    m_fanSound.looping = true;
    m_wheelSound.looping = true;
    SetMusicVolume(m_fanSound, 0.8f);
    SetMusicVolume(m_wheelSound, 0.05f);
    SetSoundVolume(m_sailSound, 0.5f);
    SetSoundVolume(m_uiClick, 0.45f);
    SetSoundVolume(m_partSelect, 0.45f);
    SetSoundVolume(m_partPlace, 0.5f);
    for (Sound horn : m_horns) SetSoundVolume(horn, 0.75f);
    m_current = 0;
    m_volume = 1.0f;
    SetMusicVolume(m_music[m_current], m_volume);
    PlayMusicStream(m_music[m_current]);
    m_initialized = true;
}

void AudioController::Shutdown() {
    if (!m_initialized) return;
    for (Music music : m_music) UnloadMusicStream(music);
    UnloadMusicStream(m_fanSound);
    UnloadMusicStream(m_wheelSound);
    UnloadSound(m_radioStatic);
    UnloadSound(m_uiClick);
    UnloadSound(m_partSelect);
    UnloadSound(m_partPlace);
    UnloadSound(m_sailSound);
    for (Sound horn : m_horns) UnloadSound(horn);
    CloseAudioDevice();
    m_initialized = false;
}

void AudioController::BeginTransition(int target) {
    if (target == m_current && !m_transitioning) return;
    m_target = target;
    m_transitioning = true;
}

void AudioController::StartLevel(int levelNumber) {
    m_returningToMenu = false;
    BeginTransition(std::clamp(levelNumber, 1, 3));
}

void AudioController::ReturnToSelector() {
    m_returningToMenu = true;
}

void AudioController::ResetToMenu() {
    PlaySound(m_radioStatic);
    m_returningToMenu = false;
    BeginTransition(0);
}

void AudioController::PlayUiClick() { PlaySound(m_uiClick); }
void AudioController::PlayPartSelect() { PlaySound(m_partSelect); }
void AudioController::PlayPartPlace() { PlaySound(m_partPlace); }
void AudioController::PlaySail() { PlaySound(m_sailSound); }
void AudioController::PlayDriverHorn() {
    PlaySound(m_horns[m_nextHorn]);
    m_nextHorn = (m_nextHorn + 1) % static_cast<int>(m_horns.size());
}

void AudioController::SetFanActive(bool active) {
    if (active == m_fanActive) return;
    m_fanActive = active;
    if (active) {
        SeekMusicStream(m_fanSound, 0.0f);
        PlayMusicStream(m_fanSound);
    }
    else StopMusicStream(m_fanSound);
}

void AudioController::SetWheelActive(bool active) {
    if (active == m_wheelActive) return;
    m_wheelActive = active;
    if (active) {
        SeekMusicStream(m_wheelSound, 0.0f);
        PlayMusicStream(m_wheelSound);
    }
    else StopMusicStream(m_wheelSound);
}

void AudioController::Update(float deltaTime) {
    if (!m_initialized) return;
    UpdateMusicStream(m_music[m_current]);
    if (m_fanActive) UpdateMusicStream(m_fanSound);
    if (m_wheelActive) UpdateMusicStream(m_wheelSound);

    if (m_transitioning) {
        m_volume = std::max(0.0f, m_volume - deltaTime / 1.25f);
        SetMusicVolume(m_music[m_current], m_volume);
        if (m_volume == 0.0f) {
            StopMusicStream(m_music[m_current]);
            m_current = m_target;
            SeekMusicStream(m_music[m_current], 0.0f);
            PlayMusicStream(m_music[m_current]);
            m_volume = 0.0f;
            SetMusicVolume(m_music[m_current], m_volume);
            m_transitioning = false;
        }
    } else if (!IsMusicStreamPlaying(m_music[m_current])) {
        if (m_current == 0 || !m_returningToMenu) {
            SeekMusicStream(m_music[m_current], 0.0f);
            PlayMusicStream(m_music[m_current]);
        } else {
            BeginTransition(0);
        }
    } else if (m_volume < 1.0f) {
        m_volume = std::min(1.0f, m_volume + deltaTime / 1.25f);
        SetMusicVolume(m_music[m_current], m_volume);
    }
}
