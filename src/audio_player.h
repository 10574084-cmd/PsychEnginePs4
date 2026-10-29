#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include <pthread.h>

class AudioPlayer {
public:
    AudioPlayer() = default;
    ~AudioPlayer();

    AudioPlayer(const AudioPlayer &) = delete;
    AudioPlayer &operator=(const AudioPlayer &) = delete;

    bool play(const std::string &oggPath);
    void stop();
    bool isPlaying() const { return active_.load(); }
    const char *lastError() const { return lastError_.c_str(); }

private:
    static void *threadEntry(void *context);
    void playbackLoop();

    pthread_t thread_{};
    bool threadStarted_ = false;
    bool audioInitialized_ = false;
    int audioHandle_ = -1;
    std::atomic<bool> stopRequested_{false};
    std::atomic<bool> active_{false};
    std::vector<int16_t> stereoPcm_;
    std::string lastError_;
};
