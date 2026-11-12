#include "app/AudioController.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace {

std::string CleanPathFor(const std::string& editedPath) {
    return (std::filesystem::path("assets/audio/clean") /
        std::filesystem::path(editedPath).filename()).string();
}

}

void AudioController::Initialize() {
    InitAudioDevice();
    std::ifstream levels("assets/audio/music_levels.tsv");
    std::string line;
    while (std::getline(levels, line)) {
        const std::size_t separator = line.find('\t');
        if (separator == std::string::npos) continue;
        m_musicGains.emplace(
            line.substr(0, separator),
            std::stof(line.substr(separator + 1))
        );
    }
    m_music[0] = LoadMusicStream("assets/audio/menu.wav");
    for (std::size_t index = 0; index < OfficialLevels.size(); ++index) {
        m_music[index + 1] = LoadMusicStream(OfficialLevels[index].musicPath);
    }
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
    SetMusicVolume(m_fanSound, 0.45f);
    SetMusicVolume(m_wheelSound, 0.05f);
    SetSoundVolume(m_sailSound, 0.5f);
    SetSoundVolume(m_uiClick, 0.45f);
    SetSoundVolume(m_partSelect, 0.45f);
    SetSoundVolume(m_partPlace, 0.5f);
    for (Sound horn : m_horns) SetSoundVolume(horn, 0.75f);
    m_current = 0;
    m_volume = 1.0f;
    ApplyMusicVolume();
    PlayMusicStream(m_music[m_current]);
    m_initialized = true;
}

void AudioController::Shutdown() {
    if (!m_initialized) return;
    if (m_customMusicLoaded) UnloadMusicStream(m_customMusic);
    if (m_cleanMusicLoaded) UnloadMusicStream(m_cleanMusic);
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

void AudioController::LoadCleanMusic(const std::string& editedPath) {
    if (m_cleanMusicLoaded) {
        UnloadMusicStream(m_cleanMusic);
        m_cleanMusicLoaded = false;
    }
    if (editedPath.empty()) return;
    const std::string cleanPath = CleanPathFor(editedPath);
    if (!FileExists(cleanPath.c_str())) return;
    m_cleanMusic = LoadMusicStream(cleanPath.c_str());
    m_cleanMusicLoaded = IsMusicValid(m_cleanMusic);
    if (m_cleanMusicLoaded) m_cleanMusic.looping = true;
}

Music& AudioController::SelectedMusic() {
    if (m_playMode || !m_cleanMusicLoaded) {
        return m_customMusicActive ? m_customMusic : m_music[m_current];
    }
    if (!m_customMusicActive && m_current == 0) return m_music[0];
    return m_cleanMusic;
}

float AudioController::SelectedMusicGain() const {
    std::string path;
    if (m_customMusicActive) {
        path = (!m_playMode && m_cleanMusicLoaded)
            ? CleanPathFor(m_customMusicPath)
            : m_customMusicPath;
    } else if (m_current == 0) {
        path = "assets/audio/menu.wav";
    } else {
        const std::string editedPath = OfficialLevels[m_current - 1].musicPath;
        path = (!m_playMode && m_cleanMusicLoaded)
            ? CleanPathFor(editedPath)
            : editedPath;
    }
    const auto gain = m_musicGains.find(path);
    return gain == m_musicGains.end() ? 1.0f : gain->second;
}

void AudioController::ApplyMusicVolume() {
    const bool cleanSelected = !m_playMode && m_cleanMusicLoaded &&
        (m_customMusicActive || m_current != 0);
    const float modeVolume = cleanSelected ? 0.85f : 1.0f;
    const float transitionVolume = m_customMusicActive ? 1.0f : m_volume;
    SetMusicVolume(
        SelectedMusic(),
        SelectedMusicGain() * modeVolume * transitionVolume * m_variantFade
    );
}

void AudioController::UpdateVariantFade(float deltaTime) {
    if (m_variantFade >= 1.0f) return;
    m_variantFade = std::min(1.0f, m_variantFade + deltaTime / 0.42f);
    ApplyMusicVolume();
}

void AudioController::StartLevel(int levelNumber) {
    SetPlayMode(false);
    if (m_customMusicActive) {
        StopMusicStream(SelectedMusic());
        m_customMusicActive = false;
        LoadCleanMusic("");
    }
    const int target = std::clamp(levelNumber, 1, OfficialLevelCount);
    m_returningToMenu = false;
    BeginTransition(target);
}

void AudioController::StartCustomMusic(const std::string& path) {
    if (!m_initialized || path.empty()) return;
    StopMusicStream(SelectedMusic());
    if (m_customMusicLoaded) {
        UnloadMusicStream(m_customMusic);
    }
    m_playMode = false;
    m_customMusicActive = false;
    m_customMusic = LoadMusicStream(path.c_str());
    m_customMusicLoaded = IsMusicValid(m_customMusic);
    if (!m_customMusicLoaded) return;
    m_customMusic.looping = true;
    m_customMusicPath = path;
    LoadCleanMusic(path);
    m_customMusicActive = true;
    m_variantFade = 1.0f;
    ApplyMusicVolume();
    PlayMusicStream(SelectedMusic());
    UpdateMusicStream(SelectedMusic());
    m_returningToMenu = false;
    m_transitioning = false;
}

void AudioController::SetPlayMode(bool playing) {
    if (m_playMode == playing) return;
    Music* previous = &SelectedMusic();
    const bool wasPlaying = IsMusicStreamPlaying(*previous);
    const float position = wasPlaying ? GetMusicTimePlayed(*previous) : 0.0f;
    m_playMode = playing;
    Music* next = &SelectedMusic();
    if (previous == next || !wasPlaying) {
        ApplyMusicVolume();
        return;
    }

    StopMusicStream(*previous);
    const float length = GetMusicTimeLength(*next);
    m_variantFade = 0.0f;
    SetMusicVolume(*next, 0.0f);
    PlayMusicStream(*next);
    SeekMusicStream(*next, length > 0.0f ? std::fmod(position, length) : 0.0f);
    UpdateMusicStream(*next);
    PlaySound(m_radioStatic);
}

void AudioController::ReturnToSelector() {
    SetPlayMode(true);
    if (m_customMusicActive) {
        StopMusicStream(SelectedMusic());
        m_customMusicActive = false;
        LoadCleanMusic("");
    }
    m_returningToMenu = true;
}

void AudioController::ResetToMenu() {
    SetPlayMode(true);
    if (m_customMusicActive) {
        StopMusicStream(SelectedMusic());
        m_customMusicActive = false;
        LoadCleanMusic("");
    }
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
    UpdateMusicStream(SelectedMusic());
    if (m_fanActive) UpdateMusicStream(m_fanSound);
    if (m_wheelActive) UpdateMusicStream(m_wheelSound);
    if (m_customMusicActive) {
        UpdateVariantFade(deltaTime);
        return;
    }

    if (m_transitioning) {
        m_volume = std::max(0.0f, m_volume - deltaTime / 1.25f);
        ApplyMusicVolume();
        if (m_volume == 0.0f) {
            StopMusicStream(SelectedMusic());
            m_current = m_target;
            if (m_current > 0) {
                LoadCleanMusic(OfficialLevels[m_current - 1].musicPath);
                if (m_current == 1 && m_cleanMusicLoaded) {
                    SetMusicPitch(m_cleanMusic, 1.08f);
                }
            } else {
                LoadCleanMusic("");
            }
            PlayMusicStream(SelectedMusic());
            SeekMusicStream(SelectedMusic(), 0.0f);
            UpdateMusicStream(SelectedMusic());
            m_volume = 0.0f;
            ApplyMusicVolume();
            m_transitioning = false;
        }
    } else if (!IsMusicStreamPlaying(SelectedMusic())) {
        if (m_current == 0 || !m_returningToMenu) {
            PlayMusicStream(SelectedMusic());
            SeekMusicStream(SelectedMusic(), 0.0f);
            UpdateMusicStream(SelectedMusic());
        } else {
            BeginTransition(0);
        }
    } else if (m_volume < 1.0f) {
        m_volume = std::min(1.0f, m_volume + deltaTime / 1.25f);
        ApplyMusicVolume();
    }
    UpdateVariantFade(deltaTime);
}
