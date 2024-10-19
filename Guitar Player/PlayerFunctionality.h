#pragma once
// I'm too lazy to make a proper class.
#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <Windows.h>
#include <Psapi.h>
#include <tlhelp32.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

const std::array<int, 16> low_e = { 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55 };
const std::array<int, 16> a = { 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60 };
const std::array<int, 16> d = { 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65 };
const std::array<int, 16> g = { 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70 };
const std::array<int, 16> b = { 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74 };
const std::array<int, 16> high_e = { 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79 };
HWND targetWindow = NULL;



struct SoundEvent {
    int timestampMS;
    std::vector<float> notes;
    SoundEvent(int time, std::vector<float> noteList) : timestampMS(time), notes(noteList) {}
};


struct MouseClick {
    int x;
    int y;
    bool isLeft;
};

std::pair<int, int> getClickPosition(int windowWidth, int windowHeight, int x, int y) {
    const double xMinRatio = 0.17740;
    const double xMaxRatio = 0.263975;
    const double yMinRatio = 0.070;
    const double yMaxRatio = 0.916014;

    int gridWidth = static_cast<int>((xMaxRatio - xMinRatio) * windowWidth);
    int gridHeight = static_cast<int>(((yMaxRatio - yMinRatio) * windowHeight));

    double cellWidth = static_cast<double>(gridWidth) / 5;
    double cellHeight = static_cast<double>(gridHeight) / 15;

    int offsetX = static_cast<int>(xMinRatio * windowWidth);
    int offsetY = static_cast<int>(yMinRatio * windowHeight);

    int clickX = offsetX + static_cast<int>((x * cellWidth) + (cellWidth / 2));
    int clickY = offsetY + static_cast<int>((y * cellHeight) + (cellHeight / 2));

    return { clickX, clickY };
}


void FindTargetWindow(DWORD processId) {
    targetWindow = NULL;
    EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL {
        DWORD windowProcessId;
    GetWindowThreadProcessId(hWnd, &windowProcessId);
    if (windowProcessId == (DWORD)lParam) {
        targetWindow = hWnd;
        return FALSE;
    }
    return TRUE;
        }, (LPARAM)processId);
}

HWND FindWindowByProcessName(const std::string& processName) {
    HWND hwnd = NULL;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);
        if (Process32First(snapshot, &processEntry)) {
            do {
                std::string currentProcessName = processEntry.szExeFile;

                // Convert both strings to lowercase for case-insensitive comparison
                std::transform(currentProcessName.begin(), currentProcessName.end(), currentProcessName.begin(),
                    [](unsigned char c) { return std::tolower(c); });
                std::string lowerProcessName = processName;
                std::transform(lowerProcessName.begin(), lowerProcessName.end(), lowerProcessName.begin(),
                    [](unsigned char c) { return std::tolower(c); });

                if (currentProcessName.find(lowerProcessName) != std::string::npos) {
                    DWORD processId = processEntry.th32ProcessID;
                    FindTargetWindow(processId);

                    if (targetWindow != NULL) {
                        break;
                    }
                }
            } while (Process32Next(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    return targetWindow;
}


void sendMultipleKeys(const std::vector<char>& keys, int sleepTime, bool down = false) {
    if (sleepTime != 0 || down == true)
    {
        for (const auto& keyPress : keys) {
            WORD vkCode = VkKeyScanA(keyPress);
            WORD scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);

            PostMessage(targetWindow, WM_KEYDOWN, vkCode, (scanCode << 16) | 1);
        }
    }

    if (sleepTime != 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));

    if (down == false) {
        for (const auto& keyPress : keys) {
            WORD vkCode = VkKeyScanA(keyPress);
            WORD scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);

            PostMessage(targetWindow, WM_KEYUP, vkCode, (scanCode << 16) | 0xC0000001);
        }
    }
}

void sendMultipleClicks(const std::vector<MouseClick>& clicks, int sleepTime, bool down = false) {
    for (const auto& click : clicks) {
        LPARAM lParam = MAKELPARAM(click.x, click.y);
        WPARAM wParam = click.isLeft ? MK_LBUTTON : MK_RBUTTON;
        UINT downMsg = click.isLeft ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
        UINT upMsg = click.isLeft ? WM_LBUTTONUP : WM_RBUTTONUP;

        PostMessage(targetWindow, downMsg, wParam, lParam);


        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        lParam = MAKELPARAM(click.x, click.y);
        wParam = click.isLeft ? MK_LBUTTON : MK_RBUTTON;
        upMsg = click.isLeft ? WM_LBUTTONUP : WM_RBUTTONUP;

        PostMessage(targetWindow, upMsg, wParam, lParam);
    }
}

std::vector<std::string> explode_midi(std::string const& s, char delim)
{
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, delim); )
    {
        result.push_back(std::move(token));
    }

    return result;
}

void LoadSongFromFile(std::string filepath, std::vector<SoundEvent>& song) {
    song.clear();
    std::ifstream songFile(filepath);
    std::string tempLine;
    int outOfRangeNotes = 0;

    while (std::getline(songFile, tempLine)) {
        if (tempLine.size() < 3)
            continue;
        std::vector<std::string> split = explode_midi(tempLine, ' ');
        int time = std::stoi(split[0]);
        std::vector<float> tempNotes;
        for (unsigned i = 1; i < split.size(); i++) {
            float note = std::stof(split[i]);
            tempNotes.push_back(note);
            if (note < 40 || note > 79) {
                outOfRangeNotes++;
            }
        }
        song.push_back(SoundEvent(time, tempNotes));
    }

    std::cout << "Song loading complete." << std::endl;
    //std::cout << "Number of notes out of range (< 40 or > 80): " << outOfRangeNotes << std::endl;  // This now gets omitted by the midi converter.
}



void playNotes(std::vector<float> notes) {
    std::sort(notes.begin(), notes.end(), std::greater<float>());
    std::vector<MouseClick> clicksToSend;
    std::vector<char> keysToPress;
    std::vector<bool> stringUsed(6, false);  // Track which strings have been used
    static std::vector<int> lastClickedPositions(6, 0);  // Track last clicked position for each string

    RECT windowRect;
    GetClientRect(targetWindow, &windowRect);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    for (float note : notes) {
        int intNote = static_cast<int>(std::round(note));

        auto checkString = [&](const std::array<int, 16>& stringNotes, int stringIndex, char key) {
            auto it = std::find(stringNotes.begin(), stringNotes.end(), intNote);
            if (it != stringNotes.end() && !stringUsed[stringIndex]) {
                int index = static_cast<int>(std::distance(stringNotes.begin(), it));
                if (lastClickedPositions[stringIndex] != index) {
                    auto [clickX, clickY] = getClickPosition(windowWidth, windowHeight, stringIndex, index);
                    clicksToSend.push_back({ clickX, clickY, true });
                    lastClickedPositions[stringIndex] = index;
                }
                keysToPress.push_back(key);
                stringUsed[stringIndex] = true;
                return true;
            }
            return false;
        };

        bool noteMatched =
            checkString(high_e, 5, 'y') ||
            checkString(b, 4, 't') ||
            checkString(g, 3, 'r') ||
            checkString(d, 2, 'e') ||
            checkString(a, 1, 'w') ||
            checkString(low_e, 0, 'q');

        if (!noteMatched) {
            std::cout << "  No match found for note " << note << " (int: " << intNote << ")" << std::endl;
        }
    }

    if (!clicksToSend.empty()) {
        sendMultipleClicks(clicksToSend, 1, true);
    }
    int extraSleep = 6 - clicksToSend.size();
    if (extraSleep > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(extraSleep));

    if (!keysToPress.empty()) {
        sendMultipleKeys(keysToPress, 12);
    }
}

void clickThroughAllPositions(int windowWidth, int windowHeight) {
    for (int string = 0; string <= 5; ++string) {
        for (int fret = 0; fret <= 15; ++fret) {
            auto [clickX, clickY] = getClickPosition(windowWidth, windowHeight, string, fret);

            std::vector<MouseClick> clicksToSend;
            clicksToSend.push_back({ clickX, clickY, true });
            sendMultipleClicks(clicksToSend, 1, true);

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            std::cout << "Clicked position: string " << string << ", fret " << fret
                << " (X: " << clickX << ", Y: " << clickY << ")" << std::endl;
        }
    }
}

void PlaySong(std::string songFileName, bool& isPlaying) {
    std::vector<SoundEvent> songData;
    std::string songPath = "songs\\" + songFileName;
    LoadSongFromFile(songPath, songData);

    const std::string processName = "webfishing.exe"; // The name of the target executable
    targetWindow = FindWindowByProcessName(processName);
    if (targetWindow == NULL) {
        std::cout << "Failed to find target window. Exiting." << std::endl;
        return;
    }

    RECT windowRect;
    GetClientRect(targetWindow, &windowRect);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;




    //clickThroughAllPositions(windowWidth, windowHeight);

    Sleep(300); // Initial delay

    auto [clickX, clickY] = getClickPosition(windowWidth, windowHeight, 5, 0);
    auto [clickX2, clickY2] = getClickPosition(windowWidth, windowHeight, 4, 0);
    clickX += (clickX - clickX2);
    std::vector<MouseClick> clicksToSend;
    clicksToSend.push_back({ clickX, clickY, true });
    sendMultipleClicks(clicksToSend, 1, true);

    auto startTime = std::chrono::high_resolution_clock::now();

    for (const auto& event : songData) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();

        if (elapsedTime < event.timestampMS) {
            std::this_thread::sleep_for(std::chrono::milliseconds(event.timestampMS - elapsedTime));
        }
        if (!isPlaying)
            break;
        playNotes(event.notes);
    }

    return;

}
