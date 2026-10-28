#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

#include <orbis/Pad.h>
#include <orbis/UserService.h>

#include "graphics.h"

std::stringstream debugLogStream;

namespace {
constexpr int kWidth = 1920;
constexpr int kHeight = 1080;

struct Glyph {
    char ch;
    uint8_t rows[7];
};

static const Glyph kFont[] = {
    {'A',{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'B',{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
    {'C',{0x0F,0x10,0x10,0x10,0x10,0x10,0x0F}},
    {'D',{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},
    {'E',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'F',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
    {'G',{0x0F,0x10,0x10,0x13,0x11,0x11,0x0F}},
    {'H',{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'I',{0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}},
    {'J',{0x07,0x02,0x02,0x02,0x12,0x12,0x0C}},
    {'K',{0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
    {'L',{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'M',{0x11,0x1B,0x15,0x15,0x11,0x11,0x11}},
    {'N',{0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
    {'O',{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'P',{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'Q',{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},
    {'R',{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S',{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T',{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'U',{0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'V',{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
    {'W',{0x11,0x11,0x11,0x15,0x15,0x15,0x0A}},
    {'X',{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
    {'Y',{0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
    {'Z',{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},
    {' ',{0,0,0,0,0,0,0}}
};

enum class Page {
    Main,
    Story,
    Freeplay,
    Options
};

const Glyph *findGlyph(char ch) {
    for (const auto &glyph : kFont) {
        if (glyph.ch == ch) return &glyph;
    }
    return &kFont[sizeof(kFont) / sizeof(kFont[0]) - 1];
}

void drawText(Scene2D *scene, const char *text, int x, int y, int scale, Color color) {
    for (const char *p = text; *p; ++p) {
        const Glyph *glyph = findGlyph(*p);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyph->rows[row] & (1u << (4 - col))) {
                    scene->DrawRectangle(x + col * scale, y + row * scale,
                                         scale, scale, color);
                }
            }
        }
        x += 6 * scale;
    }
}

void drawCenteredText(Scene2D *scene, const char *text, int y, int scale, Color color) {
    const int width = static_cast<int>(strlen(text)) * 6 * scale;
    drawText(scene, text, (kWidth - width) / 2, y, scale, color);
}

void drawPanel(Scene2D *scene, int x, int y, int w, int h,
               Color fill, Color border, int borderSize) {
    scene->DrawRectangle(x, y, w, h, fill);
    scene->DrawRectangle(x, y, w, borderSize, border);
    scene->DrawRectangle(x, y + h - borderSize, w, borderSize, border);
    scene->DrawRectangle(x, y, borderSize, h, border);
    scene->DrawRectangle(x + w - borderSize, y, borderSize, h, border);
}

void drawLogo(Scene2D *scene, Color accent, Color pale, Color background) {
    Color shadow = {34, 38, 57};
    scene->DrawRectangle(862, 76, 196, 196, accent);
    scene->DrawRectangle(880, 94, 160, 160, shadow);
    scene->DrawRectangle(898, 112, 124, 124, background);
    drawText(scene, "P", 918, 130, 18, pale);
    scene->DrawRectangle(910, 238, 100, 7, pale);
}

void drawBackButton(Scene2D *scene, Color accent, Color pale, Color panel) {
    drawPanel(scene, 725, 810, 470, 78, panel, accent, 3);
    drawCenteredText(scene, "O BACK TO MENU", 836, 4, pale);
}

void drawMainMenu(Scene2D *scene, int selected, bool controllerReady, bool blueTheme) {
    const Color background = {13, 16, 31};
    const Color pale = {244, 246, 250};
    const Color muted = {164, 176, 199};
    const Color panel = {25, 29, 46};
    const Color accent = blueTheme ? Color{68, 190, 255} : Color{239, 45, 71};

    scene->FrameBufferFill(background);
    scene->DrawRectangle(0, 0, kWidth, 14, accent);
    scene->DrawRectangle(0, kHeight - 14, kWidth, 14, accent);
    scene->DrawRectangle(72, 180, 8, 660, accent);
    scene->DrawRectangle(kWidth - 80, 180, 8, 660, accent);

    drawLogo(scene, accent, pale, background);
    drawCenteredText(scene, "PSYCH ENGINE", 304, 11, pale);
    drawCenteredText(scene, "NATIVE PS4 MENU PREVIEW", 393, 4, muted);

    const char *labels[] = {"STORY MODE", "FREEPLAY", "OPTIONS"};
    for (int i = 0; i < 3; ++i) {
        const int y = 475 + i * 116;
        const bool active = i == selected;
        const Color fill = active ? accent : panel;
        const Color edge = active ? pale : accent;
        const Color labelColor = active ? pale : muted;
        drawPanel(scene, 490, y, 940, 86, fill, edge, active ? 4 : 2);
        if (active) {
            scene->DrawRectangle(520, y + 18, 7, 50, pale);
            scene->DrawRectangle(1393, y + 18, 7, 50, pale);
        }
        drawCenteredText(scene, labels[i], y + 26, 6, labelColor);
    }

    drawCenteredText(scene, "UP DOWN MOVE   X SELECT", 884, 4, pale);
    drawCenteredText(scene, controllerReady ? "DUALSHOCK INPUT READY" : "CONTROLLER NOT DETECTED",
                     964, 3, controllerReady ? accent : muted);
}

void drawSubPage(Scene2D *scene, Page page, bool blueTheme) {
    const Color background = {13, 16, 31};
    const Color pale = {244, 246, 250};
    const Color muted = {164, 176, 199};
    const Color panel = {25, 29, 46};
    const Color accent = blueTheme ? Color{68, 190, 255} : Color{239, 45, 71};

    scene->FrameBufferFill(background);
    scene->DrawRectangle(0, 0, kWidth, 14, accent);
    scene->DrawRectangle(0, kHeight - 14, kWidth, 14, accent);
    scene->DrawRectangle(72, 110, 8, 820, accent);
    scene->DrawRectangle(kWidth - 80, 110, 8, 820, accent);

    const char *title = page == Page::Story ? "STORY MODE" :
                        page == Page::Freeplay ? "FREEPLAY" : "OPTIONS";
    drawCenteredText(scene, title, 260, 11, pale);
    scene->DrawRectangle(620, 365, 680, 4, accent);

    if (page == Page::Story) {
        drawCenteredText(scene, "NO SONGS INSTALLED", 485, 6, pale);
        drawCenteredText(scene, "GAMEPLAY RUNTIME NOT PORTED", 584, 4, muted);
        drawCenteredText(scene, "STORY MODE DATA IS NOT READY", 650, 4, muted);
    } else if (page == Page::Freeplay) {
        drawCenteredText(scene, "NO SONGS FOUND", 485, 6, pale);
        drawCenteredText(scene, "AUDIO AND CHART LOADING NOT READY", 584, 4, muted);
        drawCenteredText(scene, "THIS IS A NATIVE MENU PREVIEW", 650, 4, muted);
    } else {
        drawCenteredText(scene, "ACCENT COLOR", 465, 6, muted);
        drawCenteredText(scene, blueTheme ? "BLUE" : "RED", 565, 9, accent);
        drawCenteredText(scene, "LEFT RIGHT CHANGE COLOR", 690, 4, pale);
    }

    drawBackButton(scene, accent, pale, panel);
}

bool initController(int &padHandle) {
    if (scePadInit() != 0) {
        printf("scePadInit failed; menu will run without input.\n");
        return false;
    }

    OrbisUserServiceInitializeParams params = {};
    params.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    sceUserServiceInitialize(&params);

    int32_t userId = -1;
    if (sceUserServiceGetInitialUser(&userId) != 0 || userId < 0) {
        printf("No logged-in user; menu will run without input.\n");
        return false;
    }

    padHandle = scePadOpen(userId, ORBIS_PAD_PORT_TYPE_STANDARD, 0, nullptr);
    if (padHandle < 0) {
        printf("scePadOpen failed; menu will run without input.\n");
        return false;
    }
    return true;
}
} // namespace

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    printf("PsychEngine PS4 native menu starting.\n");

    Scene2D *scene = new Scene2D(kWidth, kHeight, 4);
    if (!scene->Init(0x0C000000, 2)) {
        printf("Video output initialization failed.\n");
        for (;;) sceKernelUsleep(250000);
    }

    int padHandle = -1;
    const bool controllerReady = initController(padHandle);
    uint32_t previousButtons = 0;
    int selected = 0;
    bool blueTheme = false;
    Page page = Page::Main;
    int frameId = 0;

    for (;;) {
        uint32_t pressed = 0;
        if (controllerReady && padHandle >= 0) {
            OrbisPadData padData = {};
            if (scePadReadState(padHandle, &padData) == 0) {
                const uint32_t currentButtons = padData.buttons;
                pressed = currentButtons & ~previousButtons;
                previousButtons = currentButtons;
            } else {
                previousButtons = 0;
            }
        }

        if (page == Page::Main) {
            if (pressed & ORBIS_PAD_BUTTON_UP) selected = (selected + 2) % 3;
            if (pressed & ORBIS_PAD_BUTTON_DOWN) selected = (selected + 1) % 3;
            if (pressed & ORBIS_PAD_BUTTON_CROSS) {
                if (selected == 0) page = Page::Story;
                else if (selected == 1) page = Page::Freeplay;
                else page = Page::Options;
            }
        } else {
            if (page == Page::Options &&
                (pressed & (ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_RIGHT))) {
                blueTheme = !blueTheme;
            }
            if (pressed & ORBIS_PAD_BUTTON_CIRCLE) page = Page::Main;
        }

        if (page == Page::Main) {
            drawMainMenu(scene, selected, controllerReady, blueTheme);
        } else {
            drawSubPage(scene, page, blueTheme);
        }

        scene->SubmitFlip(frameId);
        scene->FrameWait(frameId);
        scene->FrameBufferSwap();
        ++frameId;
    }

    return 0;
}
