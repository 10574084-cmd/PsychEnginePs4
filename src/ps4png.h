#pragma once

#include <stdint.h>

class Scene2D;

class Ps4Image {
public:
    explicit Ps4Image(const char *path);
    ~Ps4Image();

    Ps4Image(const Ps4Image &) = delete;
    Ps4Image &operator=(const Ps4Image &) = delete;

    void Draw(Scene2D *scene, int x, int y) const;
    int width() const { return width_; }
    int height() const { return height_; }
    bool valid() const { return pixels_ != nullptr; }

private:
    int width_ = 0;
    int height_ = 0;
    int sourceChannels_ = 0;
    uint8_t *pixels_ = nullptr;
};
