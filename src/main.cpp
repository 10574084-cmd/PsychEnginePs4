#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <cmath>
#include <dirent.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <chrono>
#include <sys/stat.h>

#include <orbis/Pad.h>
#include <orbis/UserService.h>

#include "graphics.h"
#include "ps4png.h"
#include "audio_player.h"

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
    FreeplayDetails,
    Gameplay,
    Results,
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
constexpr const char *kSongDirectory = "/data/PSYH00001/songs";
constexpr const char *kChartDirectory = "/data/PSYH00001/data";
constexpr const char *kBundledSongDirectory = "/app0/assets/songs";
constexpr const char *kBundledChartDirectory = "/app0/assets/preload/data";
constexpr int kFreeplayVisibleRows = 7;

struct ChartNote {
    double strumTimeMs = 0.0;
    int lane = 0;
    double sustainMs = 0.0;
    bool hit = false;
    bool missed = false;
};

struct InstalledSong {
    std::string folder;
    std::string displayName;
    std::string audioDirectory;
    std::string chartDirectory;
    std::string instrumentalPath;
    std::string voicesPath;
    bool hasChart = false;
    bool hasInstrumental = false;
    bool hasVoices = false;
    std::string chartPath;
    std::string defaultChartPath;
    std::string easyChartPath;
    std::string normalChartPath;
    std::string hardChartPath;
    double bpm = 0.0;
    int noteCount = 0;
    int difficultyCount = 0;
    int selectedDifficulty = 3; // 0 easy, 1 normal, 2 hard, 3 default chart
    double chartDurationMs = 0.0;
    std::vector<ChartNote> notes;
};
std::vector<InstalledSong> gInstalledSongs;
AudioPlayer gAudioPlayer;

struct GameplayState {
    std::chrono::steady_clock::time_point startedAt;
    int score = 0;
    int hits = 0;
    int misses = 0;
};
GameplayState gGameplay;

bool pathExists(const std::string &path) {
    FILE *file = fopen(path.c_str(), "rb");
    if (!file) return false;
    fclose(file);
    return true;
}

std::string readTextFile(const std::string &path) {
    FILE *file = fopen(path.c_str(), "rb");
    if (!file) return std::string();
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return std::string(); }
    const long length = ftell(file);
    if (length <= 0 || length > 8 * 1024 * 1024 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return std::string();
    }
    std::string result(static_cast<size_t>(length), char(0));
    const size_t read = fread(&result[0], 1, result.size(), file);
    fclose(file);
    result.resize(read);
    return result;
}

double readJsonNumberForKey(const std::string &json, const char *key) {
    const std::string token = std::string(1, char(34)) + key + std::string(1, char(34));
    const size_t keyAt = json.find(token);
    if (keyAt == std::string::npos) return 0.0;
    const size_t colon = json.find(':', keyAt + token.size());
    if (colon == std::string::npos) return 0.0;
    const char *begin = json.c_str() + colon + 1;
    while (std::isspace(static_cast<unsigned char>(*begin))) ++begin;
    char *end = nullptr;
    const double value = strtod(begin, &end);
    return end == begin || value < 0.0 ? 0.0 : value;
}

size_t findMatchingDelimiter(const std::string &json, size_t start, char open, char close) {
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (size_t i = start; i < json.size(); ++i) {
        const char ch = json[i];
        if (inString) {
            if (escaped) escaped = false;
            else if (static_cast<unsigned char>(ch) == 92) escaped = true;
            else if (ch == '"') inString = false;
            continue;
        }
        if (ch == '"') inString = true;
        else if (ch == open) ++depth;
        else if (ch == close) {
            --depth;
            if (depth == 0) return i;
        }
    }
    return std::string::npos;
}

bool jsonBooleanInObject(const std::string &json, size_t objectStart, size_t objectEnd, const char *key) {
    if (objectStart >= json.size() || objectEnd >= json.size() || objectEnd < objectStart) return false;
    const std::string object = json.substr(objectStart, objectEnd - objectStart + 1);
    const std::string token = std::string(1, char(34)) + key + std::string(1, char(34));
    const size_t keyAt = object.find(token);
    if (keyAt == std::string::npos) return false;
    const size_t colon = object.find(':', keyAt + token.size());
    if (colon == std::string::npos) return false;
    const size_t valueAt = object.find_first_not_of(" \\t\\r\n", colon + 1);
    return valueAt != std::string::npos && object.compare(valueAt, 4, "true") == 0;
}

int parseNumericRow(const std::string &row, double values[3]) {
    const char *cursor = row.c_str();
    int parsed = 0;
    while (*cursor != '\0' && parsed < 3) {
        while (std::isspace(static_cast<unsigned char>(*cursor)) || *cursor == ',') ++cursor;
        char *end = nullptr;
        const double value = strtod(cursor, &end);
        if (end == cursor) break;
        values[parsed++] = value;
        cursor = end;
        while (std::isspace(static_cast<unsigned char>(*cursor))) ++cursor;
        if (*cursor == ',') ++cursor;
    }
    return parsed;
}

void parsePlayerChartNotes(InstalledSong &song, const std::string &json) {
    song.notes.clear();
    song.noteCount = 0;
    song.chartDurationMs = 0.0;
    const std::string token = std::string(1, char(34)) + "sectionNotes" + std::string(1, char(34));
    size_t searchFrom = 0;

    while (searchFrom < json.size() && song.noteCount < 100000) {
        const size_t keyAt = json.find(token, searchFrom);
        if (keyAt == std::string::npos) break;
        const size_t colon = json.find(':', keyAt + token.size());
        if (colon == std::string::npos) break;
        const size_t arrayStart = json.find('[', colon + 1);
        if (arrayStart == std::string::npos) break;
        const size_t arrayEnd = findMatchingDelimiter(json, arrayStart, '[', ']');
        if (arrayEnd == std::string::npos) break;

        const size_t sectionStart = json.rfind('{', keyAt);
        const size_t sectionEnd = sectionStart == std::string::npos
            ? std::string::npos : findMatchingDelimiter(json, sectionStart, '{', '}');
        const bool mustHit = sectionEnd != std::string::npos && sectionEnd >= keyAt
            ? jsonBooleanInObject(json, sectionStart, sectionEnd, "mustHitSection") : false;

        size_t rowCursor = arrayStart + 1;
        while (rowCursor < arrayEnd && song.noteCount < 100000) {
            while (rowCursor < arrayEnd &&
                   (std::isspace(static_cast<unsigned char>(json[rowCursor])) || json[rowCursor] == ',')) ++rowCursor;
            if (rowCursor >= arrayEnd) break;
            if (json[rowCursor] != '[') { ++rowCursor; continue; }

            const size_t rowEnd = findMatchingDelimiter(json, rowCursor, '[', ']');
            if (rowEnd == std::string::npos || rowEnd > arrayEnd) break;
            const std::string row = json.substr(rowCursor + 1, rowEnd - rowCursor - 1);
            double values[3] = {0.0, 0.0, 0.0};
            const int valueCount = parseNumericRow(row, values);
            if (valueCount >= 2 && values[0] >= 0.0) {
                ++song.noteCount;
                song.chartDurationMs = std::max(song.chartDurationMs, values[0]);
                int lane = static_cast<int>(values[1]);
                bool playerNote = mustHit;
                if (lane >= 4) {
                    lane -= 4;
                    playerNote = !playerNote;
                }
                if (playerNote && lane >= 0 && lane < 4) {
                    ChartNote note;
                    note.strumTimeMs = values[0];
                    note.lane = lane;
                    note.sustainMs = valueCount >= 3 ? std::max(0.0, values[2]) : 0.0;
                    song.notes.push_back(note);
                }
            }
            rowCursor = rowEnd + 1;
        }
        searchFrom = arrayEnd + 1;
    }

    std::sort(song.notes.begin(), song.notes.end(),
        [](const ChartNote &a, const ChartNote &b) { return a.strumTimeMs < b.strumTimeMs; });
}
std::string firstExistingPath(const std::vector<std::string> &candidates) {
    for (const std::string &candidate : candidates) {
        if (pathExists(candidate)) return candidate;
    }
    return std::string();
}

void loadSelectedChartMetadata(InstalledSong &song) {
    song.chartPath.clear();
    if (song.selectedDifficulty == 0) song.chartPath = song.easyChartPath;
    else if (song.selectedDifficulty == 1) song.chartPath = song.normalChartPath;
    else if (song.selectedDifficulty == 2) song.chartPath = song.hardChartPath;
    else song.chartPath = song.defaultChartPath;

    if (song.chartPath.empty()) {
        if (!song.normalChartPath.empty()) {
            song.selectedDifficulty = 1;
            song.chartPath = song.normalChartPath;
        } else if (!song.defaultChartPath.empty()) {
            song.selectedDifficulty = 3;
            song.chartPath = song.defaultChartPath;
        } else if (!song.easyChartPath.empty()) {
            song.selectedDifficulty = 0;
            song.chartPath = song.easyChartPath;
        } else if (!song.hardChartPath.empty()) {
            song.selectedDifficulty = 2;
            song.chartPath = song.hardChartPath;
        }
    }

    song.hasChart = false;
    song.bpm = 0.0;
    song.noteCount = 0;
    if (song.chartPath.empty()) return;
    const std::string json = readTextFile(song.chartPath);
    if (json.empty()) return;
    song.hasChart = true;
    song.bpm = readJsonNumberForKey(json, "bpm");
    parsePlayerChartNotes(song, json);
}

void readSongChartMetadata(InstalledSong &song) {
    const std::string folderPath = song.audioDirectory;
    const std::string dataPath = song.chartDirectory;
    song.defaultChartPath = firstExistingPath({
        folderPath + "/" + song.folder + ".json",
        folderPath + "/chart.json",
        dataPath + "/" + song.folder + ".json",
        dataPath + "/chart.json"
    });
    song.easyChartPath = firstExistingPath({
        dataPath + "/" + song.folder + "-easy.json",
        folderPath + "/" + song.folder + "-easy.json"
    });
    song.normalChartPath = firstExistingPath({
        dataPath + "/" + song.folder + "-normal.json",
        folderPath + "/" + song.folder + "-normal.json"
    });
    song.hardChartPath = firstExistingPath({
        dataPath + "/" + song.folder + "-hard.json",
        folderPath + "/" + song.folder + "-hard.json"
    });

    if (song.normalChartPath.empty() && !song.defaultChartPath.empty() &&
        (!song.easyChartPath.empty() || !song.hardChartPath.empty())) {
        song.normalChartPath = song.defaultChartPath;
    }

    const bool hasSeparateDifficulties = !song.easyChartPath.empty() ||
        !song.normalChartPath.empty() || !song.hardChartPath.empty();
    song.difficultyCount = (!song.easyChartPath.empty() ? 1 : 0) +
        (!song.normalChartPath.empty() ? 1 : 0) + (!song.hardChartPath.empty() ? 1 : 0);
    if (!hasSeparateDifficulties && !song.defaultChartPath.empty()) {
        song.difficultyCount = 1;
        song.selectedDifficulty = 3;
    } else if (!song.normalChartPath.empty()) {
        song.selectedDifficulty = 1;
    } else if (!song.easyChartPath.empty()) {
        song.selectedDifficulty = 0;
    } else if (!song.hardChartPath.empty()) {
        song.selectedDifficulty = 2;
    } else {
        song.selectedDifficulty = 3;
    }
    loadSelectedChartMetadata(song);
}

void changeSongDifficulty(InstalledSong &song, int direction) {
    int choices[3];
    int count = 0;
    if (!song.easyChartPath.empty()) choices[count++] = 0;
    if (!song.normalChartPath.empty()) choices[count++] = 1;
    if (!song.hardChartPath.empty()) choices[count++] = 2;
    if (count < 2) return;

    int current = -1;
    for (int i = 0; i < count; ++i)
        if (choices[i] == song.selectedDifficulty) current = i;
    if (current < 0) current = direction >= 0 ? -1 : 0;
    current = (current + (direction >= 0 ? 1 : count - 1)) % count;
    song.selectedDifficulty = choices[current];
    loadSelectedChartMetadata(song);
}

const char *songDifficultyName(const InstalledSong &song) {
    if (song.selectedDifficulty == 0) return "DIFFICULTY EASY";
    if (song.selectedDifficulty == 1) return "DIFFICULTY NORMAL";
    if (song.selectedDifficulty == 2) return "DIFFICULTY HARD";
    return song.defaultChartPath.empty() ? "DIFFICULTY DEFAULT" : "DIFFICULTY NORMAL";
}

std::string readableSongName(const std::string &folder) {
    std::string result;
    result.reserve(folder.size());
    for (unsigned char ch : folder) {
        if (ch == '_' || ch == '-') result.push_back(' ');
        else if (std::isalnum(ch)) result.push_back(static_cast<char>(std::toupper(ch)));
        else if (ch == ' ') result.push_back(' ');
    }
    return result.empty() ? "UNTITLED SONG" : result;
}

void appendSongsFromRoot(std::vector<InstalledSong> &songs,
                         const std::string &songRoot,
                         const std::string &chartRoot) {
    DIR *directory = opendir(songRoot.c_str());
    if (!directory) {
        printf("Freeplay: song root not available: %s\n", songRoot.c_str());
        return;
    }

    struct dirent *entry = nullptr;
    while ((entry = readdir(directory)) != nullptr && songs.size() < 128) {
        const char *name = entry->d_name;
        if (!name || name[0] == '.' || name[0] == '\0') continue;
        const std::string folder(name);
        const auto existing = std::find_if(songs.begin(), songs.end(),
            [&folder](const InstalledSong &song) { return song.folder == folder; });
        if (existing != songs.end()) continue;

        const std::string base = songRoot + "/" + folder;
        struct stat info;
        if (stat(base.c_str(), &info) != 0 || !S_ISDIR(info.st_mode)) continue;

        InstalledSong song;
        song.folder = folder;
        song.displayName = readableSongName(folder);
        song.audioDirectory = base;
        song.chartDirectory = chartRoot + "/" + folder;
        readSongChartMetadata(song);
        song.instrumentalPath = firstExistingPath({base + "/Inst.ogg", base + "/inst.ogg"});
        song.voicesPath = firstExistingPath({base + "/Voices.ogg", base + "/voices.ogg"});
        song.hasInstrumental = !song.instrumentalPath.empty();
        song.hasVoices = !song.voicesPath.empty();
        if (!song.hasInstrumental && !song.hasChart) continue;
        songs.push_back(song);
    }
    closedir(directory);
}

std::vector<InstalledSong> scanInstalledSongs() {
    std::vector<InstalledSong> songs;
    mkdir(kSettingsDirectory, 0777);
    mkdir(kSongDirectory, 0777);

    // Custom songs are scanned first and override bundled songs with the same folder name.
    appendSongsFromRoot(songs, kSongDirectory, kChartDirectory);
    appendSongsFromRoot(songs, kBundledSongDirectory, kBundledChartDirectory);

    std::sort(songs.begin(), songs.end(), [](const InstalledSong &a, const InstalledSong &b) {
        return a.displayName < b.displayName;
    });
    printf("Freeplay: found %u song folders (bundled + custom).\n",
           static_cast<unsigned>(songs.size()));
    return songs;
}

void drawFreeplay(Scene2D *scene, Ps4Image &background, int selectedIndex) {
    if (gMenuSettings.showMenuBackground) background.Draw(scene, 0, 0);
    else scene->DrawRectangle(0, 0, kScreenWidth, kScreenHeight, {13, 16, 31});
    const Color panel = {22, 20, 32}, accent = {239, 45, 71};
    const Color white = {255, 255, 255}, muted = {175, 171, 190};
    const Color rowFill = {32, 29, 43}, rowEdge = {70, 65, 82};
    drawPanel(scene, 220, 85, 1480, 910, panel, accent, 5);
    drawCenteredText(scene, "FREEPLAY", 125, 9, white);
    drawCenteredText(scene, "FNF STOCK SONGS + CUSTOM SONGS", 205, 4, muted);
    if (gInstalledSongs.empty()) {
        drawCenteredText(scene, "NO SONGS FOUND", 395, 6, white);
        drawCenteredText(scene, "BUILT-IN FNF SONGS SHOULD SHIP WITH THE PKG", 500, 3, muted);
        drawCenteredText(scene, "CUSTOM SONGS: DATA PSYH00001 SONGS", 555, 3, muted);
        drawCenteredText(scene, "SONGS NEED AN OGG INSTRUMENTAL AND CHART", 610, 3, muted);
        drawCenteredText(scene, "PRESS CIRCLE TO RETURN", 865, 4, white);
        return;
    }
    const int maxIndex = static_cast<int>(gInstalledSongs.size()) - 1;
    selectedIndex = std::max(0, std::min(selectedIndex, maxIndex));
    const int first = selectedIndex >= kFreeplayVisibleRows
        ? selectedIndex - kFreeplayVisibleRows + 1 : 0;
    const int last = std::min(static_cast<int>(gInstalledSongs.size()), first + kFreeplayVisibleRows);
    for (int i = first; i < last; ++i) {
        const InstalledSong &song = gInstalledSongs[static_cast<size_t>(i)];
        const int rowY = 300 + (i - first) * 82;
        const bool selected = i == selectedIndex;
        drawPanel(scene, 340, rowY, 1240, 64, rowFill, selected ? accent : rowEdge, selected ? 4 : 2);
        const std::string label = song.displayName.substr(0, 25);
        drawText(scene, label.c_str(), 375, rowY + 22, 4, selected ? white : muted);
        drawText(scene, song.hasChart ? "CHART OK" : "NO CHART", 1110, rowY + 10, 3, song.hasChart ? white : muted);
        drawText(scene, song.hasInstrumental ? "INST OK" : "NO INST", 1110, rowY + 37, 3, song.hasInstrumental ? white : muted);
    }
    char countLine[48];
    snprintf(countLine, sizeof(countLine), "SONGS FOUND: %u", static_cast<unsigned>(gInstalledSongs.size()));
    drawText(scene, countLine, 345, 910, 3, muted);
    drawCenteredText(scene, "UP DOWN SELECT  X DETAILS  O BACK", 950, 3, white);
}

void drawFreeplayDetails(Scene2D *scene, Ps4Image &background, const InstalledSong &song) {
    if (gMenuSettings.showMenuBackground) background.Draw(scene, 0, 0);
    else scene->DrawRectangle(0, 0, kScreenWidth, kScreenHeight, {13, 16, 31});
    const Color panel = {22, 20, 32}, accent = {239, 45, 71};
    const Color white = {255, 255, 255}, muted = {190, 186, 202};
    drawPanel(scene, 280, 155, 1360, 770, panel, accent, 5);
    const std::string title = song.displayName.substr(0, 25);
    drawCenteredText(scene, title.c_str(), 220, 8, white);
    drawCenteredText(scene, song.hasChart ? "CHART FILE DETECTED" : "CHART FILE MISSING", 345, 5, song.hasChart ? white : muted);
    drawCenteredText(scene, song.hasInstrumental ? "INSTRUMENTAL FILE DETECTED" : "INST OGG MISSING", 415, 4, song.hasInstrumental ? white : muted);
    drawCenteredText(scene, song.hasVoices ? "VOCAL TRACK DETECTED" : "NO VOCAL TRACK", 475, 4, muted);
    char bpmLine[48];
    char noteLine[48];
    char difficultyLine[48];
    if (song.bpm > 0.0) snprintf(bpmLine, sizeof(bpmLine), "BPM %.0f", song.bpm);
    else snprintf(bpmLine, sizeof(bpmLine), "BPM UNKNOWN");
    snprintf(noteLine, sizeof(noteLine), "CHART NOTES %d", song.noteCount);
    snprintf(difficultyLine, sizeof(difficultyLine), "CHART DIFFICULTIES %d", song.difficultyCount);
    drawCenteredText(scene, songDifficultyName(song), 475, 4, white);
    drawCenteredText(scene, bpmLine, 525, 4, white);
    drawCenteredText(scene, noteLine, 575, 4, white);
    drawCenteredText(scene, difficultyLine, 625, 4, white);
    drawCenteredText(scene, song.hasChart && !song.notes.empty() ? "CROSS STARTS CHART PLAYTEST" :
        (song.hasInstrumental ? "TRIANGLE PREVIEWS FULL SONG" : "NO CHART OR AUDIO FOUND"),
        680, 3, gAudioPlayer.isPlaying() ? white : muted);
    drawCenteredText(scene, "LEFT RIGHT CHANGE DIFFICULTY", 735, 3, white);
    drawCenteredText(scene, "SQUARE STOPS AUDIO PREVIEW", 770, 3, white);
    drawCenteredText(scene, "CIRCLE BACK TO SONG LIST", 805, 4, white);
}


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
        drawText(scene, "PSYCH ENGINE PORT WIP", 18, kScreenHeight - 94, footerScale, white);
        drawText(scene, "SOURCE: HAXE 0.6.3", 18, kScreenHeight - 66, footerScale, white);
        drawText(scene, "NATIVE PREVIEW ONLY", 18, kScreenHeight - 38, footerScale, white);
    }

    if (gMenuSettings.showControlHints) {
        drawText(scene, "UP DOWN MOVE", 1640, kScreenHeight - 66, footerScale, white);
        drawText(scene, "X SELECT  O BACK", 1592, kScreenHeight - 38, footerScale, white);
    }
}

double gameplayElapsedMs() {
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - gGameplay.startedAt).count();
}

void startGameplay(InstalledSong &song) {
    gAudioPlayer.stop();
    for (ChartNote &note : song.notes) {
        note.hit = false;
        note.missed = false;
    }
    gGameplay.score = 0;
    gGameplay.hits = 0;
    gGameplay.misses = 0;
    if (song.hasInstrumental && !gAudioPlayer.play(song.instrumentalPath, song.voicesPath))
        printf("Gameplay audio unavailable: %s\n", gAudioPlayer.lastError());
    gGameplay.startedAt = std::chrono::steady_clock::now();
}

void hitGameplayLane(InstalledSong &song, int lane, double elapsed) {
    int bestIndex = -1;
    double bestDelta = 151.0;
    for (size_t i = 0; i < song.notes.size(); ++i) {
        ChartNote &note = song.notes[i];
        if (note.hit || note.missed || note.lane != lane) continue;
        const double delta = std::fabs(note.strumTimeMs - elapsed);
        if (delta < bestDelta) { bestDelta = delta; bestIndex = static_cast<int>(i); }
    }
    if (bestIndex < 0) return;
    ChartNote &note = song.notes[static_cast<size_t>(bestIndex)];
    note.hit = true;
    ++gGameplay.hits;
    if (bestDelta <= 45.0) gGameplay.score += 350;
    else if (bestDelta <= 90.0) gGameplay.score += 200;
    else gGameplay.score += 100;
}

void updateGameplayMisses(InstalledSong &song, double elapsed) {
    for (ChartNote &note : song.notes) {
        if (!note.hit && !note.missed && elapsed > note.strumTimeMs + 180.0) {
            note.missed = true;
            ++gGameplay.misses;
        }
    }
}

void drawGameplay(Scene2D *scene, Ps4Image &background, InstalledSong &song, double elapsed) {
    if (gMenuSettings.showMenuBackground) background.Draw(scene, 0, 0);
    else scene->DrawRectangle(0, 0, kScreenWidth, kScreenHeight, {13, 16, 31});

    const Color panel = {18, 17, 28}, accent = {239, 45, 71};
    const Color white = {255, 255, 255}, muted = {170, 168, 190};
    const Color lanes[4] = {{72, 184, 245}, {74, 220, 135}, {246, 202, 71}, {236, 85, 116}};
    drawPanel(scene, 390, 70, 1140, 910, panel, accent, 5);
    drawCenteredText(scene, "CUSTOM CHART PLAYTEST", 105, 6, white);

    char scoreLine[48], hitLine[32], missLine[32];
    snprintf(scoreLine, sizeof(scoreLine), "SCORE %d", gGameplay.score);
    snprintf(hitLine, sizeof(hitLine), "HITS %d", gGameplay.hits);
    snprintf(missLine, sizeof(missLine), "MISSES %d", gGameplay.misses);
    drawText(scene, scoreLine, 430, 160, 3, white);
    drawText(scene, hitLine, 430, 195, 3, white);
    drawText(scene, missLine, 430, 230, 3, white);

    const int laneX = 600, laneStep = 180, receptorY = 790;
    const double pixelsPerMs = 0.42;
    for (int lane = 0; lane < 4; ++lane) {
        const int x = laneX + lane * laneStep;
        scene->DrawRectangle(x, 285, 90, 590, {32, 30, 43});
        scene->DrawRectangle(x + 9, receptorY, 72, 18, lanes[lane]);
    }
    for (const ChartNote &note : song.notes) {
        if (note.hit || note.missed) continue;
        const double difference = note.strumTimeMs - elapsed;
        if (difference > 1650.0 || difference < -200.0) continue;
        const int y = static_cast<int>(std::lround(receptorY - difference * pixelsPerMs));
        if (y < 270 || y > 875) continue;
        const int x = laneX + note.lane * laneStep + 9;
        scene->DrawRectangle(x, y, 72, 24, lanes[note.lane]);
        if (note.sustainMs > 0.0) {
            const int height = static_cast<int>(std::min(220.0, note.sustainMs * pixelsPerMs));
            scene->DrawRectangle(x + 28, y - height, 16, height, lanes[note.lane]);
        }
    }
    drawCenteredText(scene, "DPAD LEFT DOWN UP RIGHT TO HIT", 910, 3, white);
    drawCenteredText(scene, "CIRCLE ABORTS SONG", 945, 3, muted);
}

void drawGameplayResults(Scene2D *scene, Ps4Image &background) {
    if (gMenuSettings.showMenuBackground) background.Draw(scene, 0, 0);
    else scene->DrawRectangle(0, 0, kScreenWidth, kScreenHeight, {13, 16, 31});
    const Color panel = {22, 20, 32}, accent = {239, 45, 71};
    const Color white = {255, 255, 255}, muted = {190, 186, 202};
    drawPanel(scene, 360, 200, 1200, 650, panel, accent, 5);
    drawCenteredText(scene, "PLAYTEST RESULTS", 265, 8, white);
    char score[48], hits[48], misses[48];
    snprintf(score, sizeof(score), "SCORE %d", gGameplay.score);
    snprintf(hits, sizeof(hits), "HITS %d", gGameplay.hits);
    snprintf(misses, sizeof(misses), "MISSES %d", gGameplay.misses);
    drawCenteredText(scene, score, 420, 6, white);
    drawCenteredText(scene, hits, 505, 5, muted);
    drawCenteredText(scene, misses, 565, 5, muted);
    drawCenteredText(scene, "CROSS PLAY AGAIN", 700, 4, white);
    drawCenteredText(scene, "CIRCLE BACK TO FREEPLAY", 755, 4, muted);
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
    int freeplaySelectedIndex = 0;
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
                    case 1:
                        gInstalledSongs = scanInstalledSongs();
                        freeplaySelectedIndex = 0;
                        page = SubPage::Freeplay;
                        break;
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
        } else if (page == SubPage::Freeplay) {
            if (!gInstalledSongs.empty()) {
                const int count = static_cast<int>(gInstalledSongs.size());
                if (pressed & ORBIS_PAD_BUTTON_UP) freeplaySelectedIndex = (freeplaySelectedIndex + count - 1) % count;
                if (pressed & ORBIS_PAD_BUTTON_DOWN) freeplaySelectedIndex = (freeplaySelectedIndex + 1) % count;
                if (pressed & ORBIS_PAD_BUTTON_CROSS) page = SubPage::FreeplayDetails;
            }
            if (pressed & ORBIS_PAD_BUTTON_TRIANGLE) {
                gInstalledSongs = scanInstalledSongs();
                freeplaySelectedIndex = 0;
            }
            if (pressed & ORBIS_PAD_BUTTON_CIRCLE) page = SubPage::None;
        } else if (page == SubPage::FreeplayDetails) {
            if (!gInstalledSongs.empty()) {
                const int detailIndex = std::max(0, std::min(freeplaySelectedIndex,
                    static_cast<int>(gInstalledSongs.size()) - 1));
                InstalledSong &song = gInstalledSongs[static_cast<size_t>(detailIndex)];
                if (pressed & ORBIS_PAD_BUTTON_LEFT) changeSongDifficulty(song, -1);
                if (pressed & ORBIS_PAD_BUTTON_RIGHT) changeSongDifficulty(song, 1);
                if (pressed & ORBIS_PAD_BUTTON_TRIANGLE) {
                    if (song.hasInstrumental) {
                        if (!gAudioPlayer.play(song.instrumentalPath, song.voicesPath))
                            printf("Audio preview failed: %s\n", gAudioPlayer.lastError());
                    } else {
                        gInstalledSongs = scanInstalledSongs();
                        freeplaySelectedIndex = std::min(freeplaySelectedIndex,
                            std::max(0, static_cast<int>(gInstalledSongs.size()) - 1));
                    }
                }
                if (pressed & ORBIS_PAD_BUTTON_CROSS) {
                    if (song.hasChart && !song.notes.empty()) {
                        startGameplay(song);
                        page = SubPage::Gameplay;
                    } else if (song.hasInstrumental &&
                               !gAudioPlayer.play(song.instrumentalPath, song.voicesPath)) {
                        printf("Audio preview failed: %s\n", gAudioPlayer.lastError());
                    }
                }
            }
            if (pressed & ORBIS_PAD_BUTTON_SQUARE) gAudioPlayer.stop();
            if (pressed & ORBIS_PAD_BUTTON_CIRCLE) {
                gAudioPlayer.stop();
                page = SubPage::Freeplay;
            }
        } else if (page == SubPage::Gameplay) {
            if (!gInstalledSongs.empty()) {
                const int songIndex = std::max(0, std::min(freeplaySelectedIndex,
                    static_cast<int>(gInstalledSongs.size()) - 1));
                InstalledSong &song = gInstalledSongs[static_cast<size_t>(songIndex)];
                const double elapsed = gameplayElapsedMs();
                updateGameplayMisses(song, elapsed);
                if (pressed & ORBIS_PAD_BUTTON_LEFT) hitGameplayLane(song, 0, elapsed);
                if (pressed & ORBIS_PAD_BUTTON_DOWN) hitGameplayLane(song, 1, elapsed);
                if (pressed & ORBIS_PAD_BUTTON_UP) hitGameplayLane(song, 2, elapsed);
                if (pressed & ORBIS_PAD_BUTTON_RIGHT) hitGameplayLane(song, 3, elapsed);
                if (pressed & ORBIS_PAD_BUTTON_CIRCLE) {
                    gAudioPlayer.stop();
                    page = SubPage::Freeplay;
                } else if (elapsed >= song.chartDurationMs + 2200.0) {
                    gAudioPlayer.stop();
                    page = SubPage::Results;
                }
            } else {
                gAudioPlayer.stop();
                page = SubPage::Freeplay;
            }
        } else if (page == SubPage::Results) {
            if (pressed & ORBIS_PAD_BUTTON_CIRCLE) page = SubPage::Freeplay;
            else if ((pressed & ORBIS_PAD_BUTTON_CROSS) && !gInstalledSongs.empty()) {
                const int songIndex = std::max(0, std::min(freeplaySelectedIndex,
                    static_cast<int>(gInstalledSongs.size()) - 1));
                startGameplay(gInstalledSongs[static_cast<size_t>(songIndex)]);
                page = SubPage::Gameplay;
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
        } else if (page == SubPage::Freeplay) {
            drawFreeplay(scene, background, freeplaySelectedIndex);
        } else if (page == SubPage::FreeplayDetails && !gInstalledSongs.empty()) {
            const int detailIndex = std::max(0, std::min(freeplaySelectedIndex, static_cast<int>(gInstalledSongs.size()) - 1));
            drawFreeplayDetails(scene, background, gInstalledSongs[static_cast<size_t>(detailIndex)]);
        } else if (page == SubPage::FreeplayDetails) {
            page = SubPage::Freeplay;
            drawFreeplay(scene, background, freeplaySelectedIndex);
        } else if (page == SubPage::Gameplay && !gInstalledSongs.empty()) {
            const int songIndex = std::max(0, std::min(freeplaySelectedIndex,
                static_cast<int>(gInstalledSongs.size()) - 1));
            drawGameplay(scene, background, gInstalledSongs[static_cast<size_t>(songIndex)], gameplayElapsedMs());
        } else if (page == SubPage::Results) {
            drawGameplayResults(scene, background);
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
