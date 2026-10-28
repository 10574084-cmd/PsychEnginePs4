#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace psychengine::ps4 {

// This interface is deliberately independent of OpenOrbis headers so the
// engine-side adapter can be developed and host-tested before wiring in PS4 APIs.

struct PadState {
    float left_x = 0.0f;
    float left_y = 0.0f;
    float right_x = 0.0f;
    float right_y = 0.0f;
    std::uint32_t buttons = 0;
    bool connected = false;
};

struct AudioSpec {
    int sample_rate = 48000;
    int channels = 2;
    int buffer_frames = 512;
};

class InputBackend {
public:
    virtual ~InputBackend() = default;
    virtual bool initialize() = 0;
    virtual PadState poll() = 0;
    virtual void shutdown() = 0;
};

class AudioBackend {
public:
    virtual ~AudioBackend() = default;
    virtual bool initialize(const AudioSpec& spec) = 0;
    virtual bool submit_pcm(const float* interleaved_samples,
                            std::uint32_t frame_count) = 0;
    virtual void shutdown() = 0;
};

class RendererBackend {
public:
    virtual ~RendererBackend() = default;
    virtual bool initialize(int width, int height) = 0;
    virtual void begin_frame() = 0;
    virtual void present() = 0;
    virtual void shutdown() = 0;
};

class FileBackend {
public:
    virtual ~FileBackend() = default;
    virtual bool read_all(const std::string& path,
                          std::vector<std::uint8_t>& output) = 0;
    virtual bool write_all(const std::string& path,
                           const std::uint8_t* data,
                           std::uint32_t size) = 0;
};

// TODO: implement these interfaces against OpenOrbis / PS4 system APIs.
// TODO: connect the real Psych Engine game loop and asset pipeline.
// TODO: add controller mapping, audio format conversion, GPU rendering,
//       save-data paths, and suspend/resume handling.

} // namespace psychengine::ps4
