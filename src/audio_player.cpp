#include "audio_player.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <orbis/AudioOut.h>
#include <orbis/UserService.h>

namespace {
constexpr int kOutputRate = 48000;
constexpr int kOutputChannels = 2;
constexpr int kAudioBlockFrames = 512;
}

AudioPlayer::~AudioPlayer() {
    stop();
}

bool AudioPlayer::play(const std::string &oggPath) {
    stop();
    lastError_.clear();

    int sourceChannels = 0;
    int sourceRate = 0;
    short *decoded = nullptr;
    const int sourceFrames = stb_vorbis_decode_filename(
        oggPath.c_str(), &sourceChannels, &sourceRate, &decoded);

    if (sourceFrames <= 0 || decoded == nullptr) {
        if (decoded != nullptr) free(decoded);
        lastError_ = "Could not decode OGG instrumental.";
        return false;
    }
    if ((sourceChannels != 1 && sourceChannels != 2) || sourceRate < 8000 || sourceRate > 192000) {
        free(decoded);
        lastError_ = "Unsupported OGG channel layout or sample rate.";
        return false;
    }

    const size_t inputFrames = static_cast<size_t>(sourceFrames);
    const size_t outputFrames = static_cast<size_t>(
        (static_cast<uint64_t>(inputFrames) * kOutputRate + sourceRate - 1) / sourceRate);
    if (outputFrames == 0 || outputFrames > (static_cast<size_t>(kOutputRate) * 60 * 10)) {
        free(decoded);
        lastError_ = "OGG duration is invalid or exceeds the 10-minute preview limit.";
        return false;
    }

    stereoPcm_.resize(outputFrames * kOutputChannels);
    for (size_t i = 0; i < outputFrames; ++i) {
        const double sourcePosition = static_cast<double>(i) * sourceRate / kOutputRate;
        const size_t frameA = std::min(static_cast<size_t>(sourcePosition), inputFrames - 1);
        const size_t frameB = std::min(frameA + 1, inputFrames - 1);
        const double fraction = sourcePosition - static_cast<double>(frameA);
        for (int channel = 0; channel < kOutputChannels; ++channel) {
            const int sourceChannel = sourceChannels == 1 ? 0 : channel;
            const int sampleA = decoded[frameA * static_cast<size_t>(sourceChannels) + sourceChannel];
            const int sampleB = decoded[frameB * static_cast<size_t>(sourceChannels) + sourceChannel];
            const double mixed = sampleA + (sampleB - sampleA) * fraction;
            const double clamped = std::max(-32768.0, std::min(32767.0, mixed));
            stereoPcm_[i * 2 + static_cast<size_t>(channel)] = static_cast<int16_t>(std::lround(clamped));
        }
    }
    free(decoded);

    if (!audioInitialized_) {
        const int initResult = sceAudioOutInit();
        if (initResult != 0) {
            stereoPcm_.clear();
            lastError_ = "sceAudioOutInit failed.";
            return false;
        }
        audioInitialized_ = true;
    }

    audioHandle_ = sceAudioOutOpen(
        ORBIS_USER_SERVICE_USER_ID_SYSTEM,
        ORBIS_AUDIO_OUT_PORT_TYPE_MAIN, 0, kAudioBlockFrames,
        kOutputRate, ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);
    if (audioHandle_ <= 0) {
        audioHandle_ = -1;
        stereoPcm_.clear();
        lastError_ = "Could not open the PS4 main audio output.";
        return false;
    }

    stopRequested_.store(false);
    active_.store(true);
    const int threadResult = pthread_create(&thread_, nullptr, &AudioPlayer::threadEntry, this);
    if (threadResult != 0) {
        active_.store(false);
        sceAudioOutClose(audioHandle_);
        audioHandle_ = -1;
        stereoPcm_.clear();
        lastError_ = "Could not start the audio worker thread.";
        return false;
    }
    threadStarted_ = true;
    return true;
}

void AudioPlayer::stop() {
    stopRequested_.store(true);
    if (threadStarted_) {
        pthread_join(thread_, nullptr);
        threadStarted_ = false;
    }
    active_.store(false);
    if (audioHandle_ >= 0) {
        sceAudioOutClose(audioHandle_);
        audioHandle_ = -1;
    }
    stereoPcm_.clear();
}

void *AudioPlayer::threadEntry(void *context) {
    AudioPlayer *player = static_cast<AudioPlayer *>(context);
    if (player != nullptr) player->playbackLoop();
    return nullptr;
}

void AudioPlayer::playbackLoop() {
    std::vector<int16_t> block(static_cast<size_t>(kAudioBlockFrames) * 2, 0);
    const size_t totalFrames = stereoPcm_.size() / 2;
    size_t offsetFrames = 0;
    bool submitted = false;

    while (!stopRequested_.load() && offsetFrames < totalFrames) {
        if (submitted) {
            if (sceAudioOutOutput(audioHandle_, nullptr) < 0) break;
            if (stopRequested_.load()) break;
        }

        std::fill(block.begin(), block.end(), 0);
        const size_t available = totalFrames - offsetFrames;
        const size_t frames = std::min(available, static_cast<size_t>(kAudioBlockFrames));
        std::memcpy(block.data(), stereoPcm_.data() + offsetFrames * 2,
                    frames * 2 * sizeof(int16_t));
        if (sceAudioOutOutput(audioHandle_, block.data()) < 0) break;

        offsetFrames += frames;
        submitted = true;
    }

    // Wait for the final queued buffer before reporting playback as complete.
    if (submitted && !stopRequested_.load()) sceAudioOutOutput(audioHandle_, nullptr);
    active_.store(false);
}
