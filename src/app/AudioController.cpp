#include "app/AudioController.hpp"
#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <print>

std::optional<std::string> MusicWithPath::TryGetDualPath() const {
    std::filesystem::path currentPath(path);
    std::filesystem::path cleanDir("assets/audio/clean");
    std::filesystem::path customDir("assets/audio/custom");
    std::filesystem::path dualPath;

    // check if the current path is inside the clean directory
    if (currentPath.lexically_normal().string().find("assets/audio/clean") != std::string::npos) {
        dualPath = customDir / currentPath.filename();
    }
    // check if the current path is inside the custom directory
    else if (currentPath.lexically_normal().string().find("assets/audio/custom") != std::string::npos) {
        dualPath = cleanDir / currentPath.filename();
    }
    else {
        return std::nullopt;
    }
    if (std::filesystem::exists(dualPath)) {
        return dualPath.generic_string();
    }
    return std::nullopt;
}

std::string MusicWithPath::GetTrackName(const std::string& path) {
    std::filesystem::path p(path);
    return p.stem().string();
}

std::string MusicWithPath::GetTrackName() const {
    std::filesystem::path p(path);
    return p.stem().string();
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
    m_menuMusicClean = MusicWithPath::Load("assets/audio/menu-clean.ogg");
    m_menuMusicCustom = MusicWithPath::Load("assets/audio/menu-custom.wav");
    m_menuMusicClean.music.looping = true;
    m_menuMusicCustom.music.looping = true;
    for (std::size_t index = 0; index < OfficialLevels.size(); ++index) {
        m_customLevelMusic[index] = MusicWithPath::Load(OfficialLevels[index].customMusicPath);
        m_customLevelMusic[index].music.looping = true;
        m_cleanLevelMusic[index] = m_customLevelMusic[index].GetAndLoadDual();
        m_cleanLevelMusic[index].music.looping = true;
    }
    m_radioStatic = LoadSound("assets/audio/radio_static.wav");
    SetSoundVolume(m_radioStatic, 0.15f);
    m_uiClick = LoadSound("assets/audio/sfx/ui_click.wav");
    m_partSelect = LoadSound("assets/audio/sfx/part_select.wav");
    m_partPlace = LoadSound("assets/audio/sfx/part_place.wav");
    m_sailSound = LoadSound("assets/audio/sfx/sail.wav");
    m_damageSounds = {
        LoadSound("assets/audio/sfx/damage_01.wav"),
        LoadSound("assets/audio/sfx/damage_02.wav"),
        LoadSound("assets/audio/sfx/damage_03.wav"),
        LoadSound("assets/audio/sfx/damage_04.wav"),
        LoadSound("assets/audio/sfx/damage_05.wav"),
        LoadSound("assets/audio/sfx/damage_06.wav"),
        LoadSound("assets/audio/sfx/damage_07.wav"),
        LoadSound("assets/audio/sfx/damage_08.wav"),
        LoadSound("assets/audio/sfx/damage_09.wav"),
    };
    for (Sound damage : m_damageSounds) SetSoundVolume(damage, 0.65f);
    m_breakageSounds = {
        LoadSound("assets/audio/sfx/breakage_1.wav"),
        LoadSound("assets/audio/sfx/breakage_2.wav"),
        LoadSound("assets/audio/sfx/breakage_3.wav"),
        LoadSound("assets/audio/sfx/breakage_4.wav"),
    };
    for (Sound damage : m_breakageSounds) SetSoundVolume(damage, 1.75f);
    m_explosionSound = LoadSound("assets/audio/sfx/explosion.wav");
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
    m_current = {};
    m_volume = 1.0f;
    ApplyMusicVolume();
    PlayMusicStream(GetMusicFromScenario(m_current).music);
    m_initialized = true;
}

void AudioController::Shutdown() {
    if (!m_initialized) return;
    for (MusicWithPath& music : m_cleanLevelMusic) UnloadMusicStream(music.music);
    for (MusicWithPath& music : m_customLevelMusic) UnloadMusicStream(music.music);
    UnloadMusicStream(m_fanSound);
    UnloadMusicStream(m_wheelSound);
    UnloadSound(m_radioStatic);
    UnloadSound(m_uiClick);
    UnloadSound(m_partSelect);
    UnloadSound(m_partPlace);
    UnloadSound(m_sailSound);
    UnloadSound(m_explosionSound);
    for (Sound horn : m_horns) UnloadSound(horn);
    for (Sound damage : m_damageSounds) UnloadSound(damage);
    for (Sound breakage : m_breakageSounds) UnloadSound(breakage);
    CloseAudioDevice();
    m_initialized = false;
}

void AudioController::BeginTransition(Scenario target) {
    if (target == m_current && !m_transitioning) return;
    m_target = target;
    m_transitioning = true;
}

const MusicWithPath& AudioController::GetMusicFromScenario(Scenario scenario) const {
    if (scenario.type == Scenario::MainMenu) {
        return m_menuMusicClean;
    }
    if (scenario.type == Scenario::LevelSelect) {
        return m_menuMusicCustom;
    }
    if (scenario.type == Scenario::LevelEditorBuild) {
        return m_levelEditorMusicLoaded ? m_levelEditorMusicCustom : m_menuMusicCustom;
    }
    if (scenario.type == Scenario::LevelEditorMusicPreview) {
        return m_levelEditorMusicLoaded ? m_levelEditorMusicClean : m_menuMusicClean;
    }
    if (scenario.levelIndex > 0 && scenario.levelIndex <= OfficialLevelCount) {
        return scenario.type == Scenario::PlayLevel
            ? m_cleanLevelMusic[scenario.levelIndex - 1]
            : m_customLevelMusic[scenario.levelIndex - 1];
    }
    return scenario.type == Scenario::PlayLevel ? m_menuMusicClean : m_menuMusicCustom; // fallback
}

const MusicWithPath& AudioController::SelectedMusic() const {
    return GetMusicFromScenario(m_current);
}

float AudioController::SelectedMusicGain() const {
    const MusicWithPath& music = SelectedMusic();
    const auto gain = m_musicGains.find(music.path);
    return gain == m_musicGains.end() ? 1.0f : gain->second;
}

void AudioController::ApplyMusicVolume() {
    const bool cleanSelected =
        m_current.IsClean() && m_customMusicActive;
    const float modeVolume = cleanSelected ? 2.0f : 1.0f;
    const float transitionVolume = m_customMusicActive ? 1.0f : m_volume;
    SetMusicVolume(
        SelectedMusic().music,
        SelectedMusicGain() * modeVolume * transitionVolume * m_variantFade
    );
}

void AudioController::UpdateVariantFade(float deltaTime) {
    if (m_variantFade >= 1.0f) return;
    m_variantFade = std::min(1.0f, m_variantFade + deltaTime / 0.42f);
    ApplyMusicVolume();
}

void AudioController::StartLevel(int levelNumber) {
    Scenario scenario = {Scenario::VehicleBuild, levelNumber};
    SetCurrentScenario(scenario);
    if (m_customMusicActive) {
        StopMusicStream(SelectedMusic().music);
        m_customMusicActive = false;
    }
    const int target = std::clamp(levelNumber, 1, OfficialLevelCount);
    m_returningToMenu = false;
    BeginTransition(scenario);
}

void AudioController::StartLevelEditorMusic(const std::string& path, bool preview) {
    if (!m_initialized || path.empty()) return;
    if (!m_levelEditorMusicLoaded) {
        StopMusicStream(SelectedMusic().music);
    }

    if (m_levelEditorMusicLoaded &&
        m_levelEditorMusicClean.GetTrackName() != MusicWithPath::GetTrackName(path)) {
        UnloadMusicStream(m_levelEditorMusicClean.music);
    } else if (m_levelEditorMusicLoaded &&
        m_levelEditorMusicClean.GetTrackName() == MusicWithPath::GetTrackName(path)) {
        auto trackName = MusicWithPath::GetTrackName(path);
        if (m_customMusicActive) {
            StopMusicStream(SelectedMusic().music);
            m_customMusicActive = false;
        }
        if (preview) {
            SetCurrentScenario({Scenario::LevelEditorMusicPreview, 0, trackName});
        } else {
            SetCurrentScenario({Scenario::LevelEditorBuild, 0, trackName});
        }
        return;
    }
    m_levelEditorMusicCustom = MusicWithPath::Load(path);
    m_levelEditorMusicLoaded = IsMusicValid(m_levelEditorMusicCustom.music);
    if (!m_levelEditorMusicLoaded) return;
    std::string trackName = m_levelEditorMusicCustom.GetTrackName();
    m_levelEditorMusicCustom.music.looping = true;
    m_customMusicPath = path;
    m_levelEditorMusicClean = m_levelEditorMusicCustom.GetAndLoadDual();
    if (preview) {
        SetCurrentScenario({Scenario::LevelEditorMusicPreview, 0, trackName});
    } else {
        SetCurrentScenario({Scenario::LevelEditorBuild, 0, trackName});
    }
    m_customMusicActive = true;
    m_variantFade = 1.0f;
    ApplyMusicVolume();
    PlayMusicStream(SelectedMusic().music);
    UpdateMusicStream(SelectedMusic().music);
    m_returningToMenu = false;
    m_transitioning = false;
}

void AudioController::SetCurrentScenario(Scenario scenario) {
    if (m_current == scenario) return;
    bool sameScene = m_current.AreOnSameScene(scenario);
    const Music* previous = &SelectedMusic().music;
    const bool wasPlaying = IsMusicStreamPlaying(*previous);
    const float position = (sameScene && wasPlaying) ? GetMusicTimePlayed(*previous) : 0.0f;

    m_current = scenario;
    const Music* next = &SelectedMusic().music;
    if (!wasPlaying) {
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
Scenario AudioController::GetCurrentScenario() const { return m_current; }

void AudioController::ReturnToSelector() {
    SetCurrentScenario({Scenario::LevelSelect});
    if (m_customMusicActive) {
        StopMusicStream(SelectedMusic().music);
        m_customMusicActive = true;
    }
    m_returningToMenu = true;
}

void AudioController::ResetToMenu() {
    SetCurrentScenario({Scenario::MainMenu});
    if (m_customMusicActive) {
        StopMusicStream(SelectedMusic().music);
        m_customMusicActive = false;;
    }
    PlaySound(m_radioStatic);
    m_returningToMenu = false;
    BeginTransition({Scenario::MainMenu});
}

void AudioController::PlayUiClick() { PlaySound(m_uiClick); }
void AudioController::PlayPartSelect() { PlaySound(m_partSelect); }
void AudioController::PlayPartPlace() { PlaySound(m_partPlace); }
void AudioController::PlaySail() { PlaySound(m_sailSound); }
void AudioController::PlayDriverHorn() {
    PlaySound(m_horns[m_nextHorn]);
    m_nextHorn = (m_nextHorn + 1) % static_cast<int>(m_horns.size());
}
void AudioController::PlayDamageEffect() {
    if (m_damageSoundsTimer > 0.0f) return;
    int randomIndex = rand() % m_damageSounds.size();
    PlaySound(m_damageSounds[randomIndex]);
    m_damageSoundsTimer = m_damageSoundsTimeCooldown;
}
void AudioController::PlayBreakageEffect() {
    int randomIndex = rand() % m_breakageSounds.size();
    PlaySound(m_breakageSounds[randomIndex]);
}
void AudioController::PlayTntExplosion(int amount) {
    float volume = 1.0f + std::min(amount, 5) * 0.35f;
    SetSoundVolume(m_explosionSound, volume);
    PlaySound(m_explosionSound);
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
    UpdateMusicStream(SelectedMusic().music);
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
            StopMusicStream(SelectedMusic().music);
            m_current = m_target;
            PlayMusicStream(SelectedMusic().music);
            SeekMusicStream(SelectedMusic().music, 0.0f);
            UpdateMusicStream(SelectedMusic().music);
            m_volume = 0.0f;
            ApplyMusicVolume();
            m_transitioning = false;
        }
    } else if (!IsMusicStreamPlaying(SelectedMusic().music)) {
        if (m_current == Scenario{} || !m_returningToMenu) {
            PlayMusicStream(SelectedMusic().music);
            SeekMusicStream(SelectedMusic().music, 0.0f);
            UpdateMusicStream(SelectedMusic().music);
        } else {
            BeginTransition({Scenario::MainMenu});
        }
    } else if (m_volume < 1.0f) {
        m_volume = std::min(1.0f, m_volume + deltaTime / 1.25f);
        ApplyMusicVolume();
    }
    if (m_damageSoundsTimer > 0.0f) {
        m_damageSoundsTimer = std::max(0.0f, m_damageSoundsTimer - deltaTime);
    }
    UpdateVariantFade(deltaTime);
}
