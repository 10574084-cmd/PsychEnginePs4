#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <stdio.h>
#include "graphics.h"
#include "ps4png.h"

Ps4Image::Ps4Image(const char *path) {
    pixels_ = stbi_load(path, &width_, &height_, &sourceChannels_, STBI_rgb_alpha);
    if (pixels_ == nullptr) {
        printf("Failed to load menu image '%s': %s\n", path, stbi_failure_reason());
        width_ = 0;
        height_ = 0;
    } else {
        printf("Loaded menu image '%s' (%dx%d)\n", path, width_, height_);
    }
}

Ps4Image::~Ps4Image() {
    if (pixels_ != nullptr) {
        stbi_image_free(pixels_);
        pixels_ = nullptr;
    }
}

void Ps4Image::Draw(Scene2D *scene, int x, int y) const {
    if (scene == nullptr || pixels_ == nullptr || width_ <= 0 || height_ <= 0)
        return;
    scene->DrawRGBAImage(x, y, width_, height_, pixels_);
}
