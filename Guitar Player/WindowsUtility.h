#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <Psapi.h>
#include <tlhelp32.h>


#include <algorithm>
class WindowsUtility {
public:
    WindowsUtility() = delete;

    static HWND findWindowByProcessName(const std::string& processName);
    static bool isWindowValid(HWND window);
    static std::pair<int, int> getWindowDimensions(HWND window);

    static bool isProcessElevated(HANDLE process);
    static bool needsElevation(HWND targetWindow);
    static bool restartWithElevation(const std::string& exePath);

    static bool canSendMessage(HWND targetWindow);
    static bool sendWindowsMessage(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

    static std::string getLastErrorAsString();
    static void showMessageBox(const std::string& message, const std::string& title = "Information",
        UINT type = MB_OK | MB_ICONINFORMATION);
    static bool elevateWithPrompt(const std::string& reason = "This operation requires elevated privileges.");


private:
    static HWND findTargetWindow(DWORD processId);
};