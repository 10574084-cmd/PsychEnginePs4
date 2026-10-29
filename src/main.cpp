#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <cmath>
#include <sys/stat.h>

#include <orbis/Pad.h>
#include <orbis/UserService.h>

#include "graphics.h"
#include "ps4png.h"

std::stringstream debugLogStream;

namespace {
constexpr int kScreenWidth = 1920;
constexpr int kScreenHeight = 1080;
constexpr float kGameToScreen = 1.5f;

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
    {'0',{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
    {'1',{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'2',{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
    {'3',{0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}},
    {'4',{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},
    {'5',{0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}},
    {'6',{0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}},
    {'7',{0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},
    {'8',{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
    {'9',{0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}},
    {'.',{0,0,0,0,0,0x0C,0x0C}},
    {'-',{0,0,0,0x1F,0,0,0}},
    {'\'',{0x0C,0x0C,0x08,0,0,0,0}},
    {' ',{0,0,0,0,0,0,0}}
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
                    scene->DrawRectangle(x + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }
        x += 6 * scale;
    }
}

void drawCenteredText(Scene2D *scene, const char *text, int y, int scale, Color color) {
    const int textWidth = static_cast<int>(strlen(text)) * 6 * scale;
    drawText(scene, text, (kScreenWidth - textWidth) / 2, y, scale, color);
}

void drawPanel(Scene2D *scene, int x, int y, int w, int h, Color fill, Color edge, int border) {
    scene->DrawRectangle(x, y, w, h, fill);
    scene->DrawRectangle(x, y, w, border, edge);
    scene->DrawRectangle(x, y + h - border, w, border, edge);
    scene->DrawRectangle(x, y, border, h, edge);
    scene->DrawRectangle(x + w - border, y, border, h, edge);
}

bool initController(int &handle) {
    if (scePadInit() != 0) {
        printf("scePadInit failed; the menu will still render without controller navigation.\n");
        return false;
    }

    OrbisUserServiceInitializeParams params = {};
    params.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    sceUserServiceInitialize(&params);

    int32_t userId = -1;
    if (sceUserServiceGetInitialUser(&userId) != 0 || userId < 0) {
        printf("No logged-in user; the menu will render without controller navigation.\n");
        return false;
    }

    handle = scePadOpen(userId, ORBIS_PAD_PORT_TYPE_STANDARD, 0, nullptr);
    if (handle < 0) {
        printf("scePadOpen failed; the menu will render without controller navigation.\n");
        return false;
    }
    printf("DualShock controller input initialized.\n");
    return true;
}

struct MenuOption {
    const char *name;
    Ps4Image *basic;
    Ps4Image *selected;
};

enum class SubPage {
    None,
    Story,
    Freeplay,
    Mods,
    Awards,
    Credits,
    Donate,
    Options
};

struct MenuSettings {
    bool showVersionInfo = true;
    bool showControlHints = true;
    bool showMenuBackground = true;
};

MenuSettings gMenuSettings;
constexpr const char *kSettingsDirectory = "/data/PSYH00001";
constexpr const char *kSettingsPath = "/data/PSYH00001/menu_settings.cfg";
constexpr int kOptionsCount = 5;

void loadMenuSettings() {
    FILE *file = fopen(kSettingsPath, "r");
    if (file == nullptr) {
        printf("Menu settings: using defaults (no saved settings found).\n");
        return;
    }

    int showVersionInfo = 1;
    int showControlHints = 1;
    int showMenuBackground = 1;
    const int parsed = fscanf(file, "%d %d %d",
                              &showVersionInfo, &showControlHints, &showMenuBackground);
    fclose(file);

    if (parsed == 3) {
        gMenuSettings.showVersionInfo = showVersionInfo != 0;
        gMenuSettings.showControlHints = showControlHints != 0;
        gMenuSettings.showMenuBackground = showMenuBackground != 0;
        printf("Menu settings loaded.\n");
    } else {
        printf("Menu settings file is invalid; using defaults.\n");
    }
}

void saveMenuSettings() {
    mkdir(kSettingsDirectory, 0777);
    FILE *file = fopen(kSettingsPath, "w");
    if (file == nullptr) {
        printf("Could not save menu settings to %s.\n", kSettingsPath);
        return;
    }

    fprintf(file, "%d %d %d\n",
            gMenuSettings.showVersionInfo ? 1 : 0,
            gMenuSettings.showControlHints ? 1 : 0,
            gMenuSettings.showMenuBackground ? 1 : 0);
    fclose(file);
    printf("Menu settings saved.\n");
}

void drawMainMenu(Scene2D *scene, Ps4Image &background, MenuOption *items,
                  int itemCount, int selectedIndex) {
    if (gMenuSettings.showMenuBackground) {
        background.Draw(scene, 0, 0);
    } else {
        scene->DrawRectangle(0, 0, kScreenWidth, kScreenHeight, {13, 16, 31});
    }

    // Psych Engine 0.6.3's menu uses 140px spacing, a -132px base offset
    // for seven entries, and tracks the selected sprite with a small vertical bias.
    const int originalOffset = 108 - (itemCount > 4 ? (itemCount - 4) * 80 : 0);
    const int selectedBaseY = static_cast<int>(std::lround(
        (selectedIndex * 140 + originalOffset) * kGameToScreen));
    Ps4Image *selectedImage = items[selectedIndex].selected;
    const float targetCenterY = kScreenHeight * 0.5f +
        (itemCount > 4 ? itemCount * 8 * kGameToScreen : 0.0f);
    const float verticalShift = targetCenterY -
        (selectedBaseY + selectedImage->height() * 0.5f);

    for (int i = 0; i < itemCount; ++i) {
        Ps4Image *image = (i == selectedIndex) ? items[i].selected : items[i].basic;
        const int baseY = static_cast<int>(std::lround(
            (i * 140 + originalOffset) * kGameToScreen));
        const int x = (kScreenWidth - image->width()) / 2;
        const int y = static_cast<int>(std::lround(baseY + verticalShift));
        image->Draw(scene, x, y);
    }

    const Color white = {255, 255, 255};
    const int footerScale = 2;

    if (gMenuSettings.showVersionInfo) {
        drawText(scene, "PSYCH ENGINE PS4 BETA", 18, kScreenHeight - 94, footerScale, white);
        drawText(scene, "PSYCH ENGINE V0.6.3", 18, kScreenHeight - 66, footerScale, white);
        drawText(scene, "FRIDAY NIGHT FUNKIN V0.2", 18, kScreenHeight - 38, footerScale, white);
    }

    if (gMenuSettings.showControlHints) {
        drawText(scene, "UP DOWN MOVE", 1640, kScreenHeight - 66, footerScale, white);
        drawText(scene, "X SELECT  O BACK", 1592, kScreenHeight - 38, footerScale, white);
    }
}

void drawPlaceholder(Scene2D *scene, Ps4Image &background, SubPage page) {
    if (gMenuSettings.showMenuBackground) {
        background.Draw(scene, 0, 0);
    } else {
        scene->DrawRectangle(0, 0, kScreenWidth, kScreenHeight, {13, 16, 31});
    }
    const Color panel = {22, 20, 32};
    const Color accent = {239, 45, 71};
    const Color white = {255, 255, 255};
    const Color muted = {225, 225, 225};
    const int boxX = 240;
    const int boxY = 340;
    const int boxW = 1440;
    const int boxH = 360;
    drawPanel(scene, boxX, boxY, boxW, boxH, panel, accent, 5);

    const char *title = "NOT PORTED YET";
    if (page == SubPage::Story) title = "STORY MODE";
    else if (page == SubPage::Freeplay) title = "FREEPLAY";
    else if (page == SubPage::Mods) title = "MODS";
    else if (page == SubPage::Awards) title = "AWARDS";
    else if (page == SubPage::Credits) title = "CREDITS";
    else if (page == SubPage::Donate) title = "DONATE";
    else if (page == SubPage::Options) title = "OPTIONS";

    drawCenteredText(scene, title, boxY + 52, 9, white);
    drawCenteredText(scene, "THE ORIGINAL MENU ASSETS ARE LOADED", boxY + 175, 4, muted);
    drawCenteredText(scene, "GAMEPLAY AND THIS SUBMENU ARE NOT PORTED", boxY + 225, 4, muted);
    drawCenteredText(scene, "PRESS O TO RETURN", boxY + 286, 4, white);
}
void drawOptions(Scene2D *scene, Ps4Image &background, int selectedIndex) {
    if (gMenuSettings.showMenuBackground) {
        background.Draw(scene, 0, 0);
    } else {
        scene->DrawRectangle(0, 0, kScreenWidth, kScreenHeight, {13, 16, 31});
    }

    const Color panel = {22, 20, 32};
    const Color accent = {239, 45, 71};
    const Color white = {255, 255, 255};
    const Color muted = {175, 171, 190};
    const Color rowFill = {32, 29, 43};
    const Color rowEdge = {70, 65, 82};

    drawPanel(scene, 300, 145, 1320, 790, panel, accent, 5);
    drawCenteredText(scene, "PSYCH ENGINE OPTIONS", 195, 8, white);
    drawCenteredText(scene, "NATIVE PS4 MENU SETTINGS", 265, 4, muted);

    char versionLine[64];
    char hintsLine[64];
    char backgroundLine[64];
    snprintf(versionLine, sizeof(versionLine), "SHOW VERSION INFO - %s",
             gMenuSettings.showVersionInfo ? "ON" : "OFF");
    snprintf(hintsLine, sizeof(hintsLine), "SHOW CONTROL HINTS - %s",
             gMenuSettings.showControlHints ? "ON" : "OFF");
    snprintf(backgroundLine, sizeof(backgroundLine), "MENU BACKGROUND - %s",
             gMenuSettings.showMenuBackground ? "ON" : "OFF");

    const char *options[kOptionsCount] = {
        versionLine,
        hintsLine,
        backgroundLine,
        "RESTORE DEFAULTS",
        "BACK TO MAIN MENU"
    };

    for (int i = 0; i < kOptionsCount; ++i) {
        const int rowX = 430;
        const int rowY = 325 + i * 104;
        const int rowW = 1060;
        const int rowH = 78;
        const bool selected = i == selectedIndex;
        drawPanel(scene, rowX, rowY, rowW, rowH, rowFill,
                  selected ? accent : rowEdge, selected ? 4 : 2);
        drawText(scene, options[i], rowX + 30, rowY + 31, 4,
                 selected ? white : muted);
    }

    drawCenteredText(scene, "CROSS OR LEFT RIGHT TO CHANGE", 865, 3, white);
    drawCenteredText(scene, "CIRCLE TO RETURN", 900, 3, muted);
}

} // namespace

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    printf("PsychEngine PS4 v0.2 - original Psych Engine menu assets.\n");

    Scene2D *scene = new Scene2D(kScreenWidth, kScreenHeight, 4);
    if (!scene->Init(0x0C000000, 2)) {
        printf("Video output initialization failed.\n");
        for (;;) sceKernelUsleep(250000);
    }

    Ps4Image background("/app0/assets/psychmenu/menu_background.png");
    Ps4Image storyBasic("/app0/assets/psychmenu/story_mode_basic.png");
    Ps4Image storySelected("/app0/assets/psychmenu/story_mode_selected.png");
    Ps4Image freeplayBasic("/app0/assets/psychmenu/freeplay_basic.png");
    Ps4Image freeplaySelected("/app0/assets/psychmenu/freeplay_selected.png");
    Ps4Image modsBasic("/app0/assets/psychmenu/mods_basic.png");
    Ps4Image modsSelected("/app0/assets/psychmenu/mods_selected.png");
    Ps4Image awardsBasic("/app0/assets/psychmenu/awards_basic.png");
    Ps4Image awardsSelected("/app0/assets/psychmenu/awards_selected.png");
    Ps4Image creditsBasic("/app0/assets/psychmenu/credits_basic.png");
    Ps4Image creditsSelected("/app0/assets/psychmenu/credits_selected.png");
    Ps4Image donateBasic("/app0/assets/psychmenu/donate_basic.png");
    Ps4Image donateSelected("/app0/assets/psychmenu/donate_selected.png");
    Ps4Image optionsBasic("/app0/assets/psychmenu/options_basic.png");
    Ps4Image optionsSelected("/app0/assets/psychmenu/options_selected.png");

    MenuOption items[] = {
        {"story_mode", &storyBasic, &storySelected},
        {"freeplay", &freeplayBasic, &freeplaySelected},
        {"mods", &modsBasic, &modsSelected},
        {"awards", &awardsBasic, &awardsSelected},
        {"credits", &creditsBasic, &creditsSelected},
        {"donate", &donateBasic, &donateSelected},
        {"options", &optionsBasic, &optionsSelected}
    };
    constexpr int itemCount = sizeof(items) / sizeof(items[0]);

    if (!background.valid()) {
        printf("Original menu background missing from the package.\n");
        for (;;) sceKernelUsleep(250000);
    }
    for (int i = 0; i < itemCount; ++i) {
        if (!items[i].basic->valid() || !items[i].selected->valid()) {
            printf("Original menu sprite missing: %s\n", items[i].name);
            for (;;) sceKernelUsleep(250000);
        }
    }

    loadMenuSettings();

    int padHandle = -1;
    const bool controllerReady = initController(padHandle);
    if (!controllerReady) {
        printf("Warning: controller is not ready; the original menu stays visible but cannot change selection.\n");
    }

    uint32_t previousButtons = 0;
    int selectedIndex = 0;
    int optionsSelectedIndex = 0;
    SubPage page = SubPage::None;
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

        if (page == SubPage::None) {
            if (pressed & ORBIS_PAD_BUTTON_UP) {
                selectedIndex = (selectedIndex + itemCount - 1) % itemCount;
            }
            if (pressed & ORBIS_PAD_BUTTON_DOWN) {
                selectedIndex = (selectedIndex + 1) % itemCount;
            }
            if (pressed & ORBIS_PAD_BUTTON_CROSS) {
                switch (selectedIndex) {
                    case 0: page = SubPage::Story; break;
                    case 1: page = SubPage::Freeplay; break;
                    case 2: page = SubPage::Mods; break;
                    case 3: page = SubPage::Awards; break;
                    case 4: page = SubPage::Credits; break;
                    case 5: page = SubPage::Donate; break;
                    default:
                        page = SubPage::Options;
                        optionsSelectedIndex = 0;
                        break;
                }
            }
        } else if (page == SubPage::Options) {
            if (pressed & ORBIS_PAD_BUTTON_UP) {
                optionsSelectedIndex = (optionsSelectedIndex + kOptionsCount - 1) % kOptionsCount;
            }
            if (pressed & ORBIS_PAD_BUTTON_DOWN) {
                optionsSelectedIndex = (optionsSelectedIndex + 1) % kOptionsCount;
            }

            const bool activate = (pressed & ORBIS_PAD_BUTTON_CROSS) != 0;
            const bool adjustLeft = (pressed & ORBIS_PAD_BUTTON_LEFT) != 0;
            const bool adjustRight = (pressed & ORBIS_PAD_BUTTON_RIGHT) != 0;
            if (activate || adjustLeft || adjustRight) {
                switch (optionsSelectedIndex) {
                    case 0:
                        gMenuSettings.showVersionInfo = !gMenuSettings.showVersionInfo;
                        saveMenuSettings();
                        break;
                    case 1:
                        gMenuSettings.showControlHints = !gMenuSettings.showControlHints;
                        saveMenuSettings();
                        break;
                    case 2:
                        gMenuSettings.showMenuBackground = !gMenuSettings.showMenuBackground;
                        saveMenuSettings();
                        break;
                    case 3:
                        gMenuSettings = MenuSettings{};
                        saveMenuSettings();
                        break;
                    case 4:
                        if (activate || (pressed & ORBIS_PAD_BUTTON_CIRCLE)) {
                            page = SubPage::None;
                        }
                        break;
                }
            }

            if (pressed & ORBIS_PAD_BUTTON_CIRCLE) {
                page = SubPage::None;
            }
        } else if (pressed & ORBIS_PAD_BUTTON_CIRCLE) {
            page = SubPage::None;
        }

        if (page == SubPage::None) {
            drawMainMenu(scene, background, items, itemCount, selectedIndex);
        } else if (page == SubPage::Options) {
            drawOptions(scene, background, optionsSelectedIndex);
        } else {
            drawPlaceholder(scene, background, page);
        }

        scene->SubmitFlip(frameId);
        scene->FrameWait(frameId);
        scene->FrameBufferSwap();
        ++frameId;
    }
}
