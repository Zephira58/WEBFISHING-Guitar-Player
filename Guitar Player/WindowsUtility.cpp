#include "WindowsUtility.h"

HWND WindowsUtility::findWindowByProcessName(const std::string& processName) {
    HWND hwnd = NULL;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to create process snapshot: " + getLastErrorAsString());
    }

    try {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);

        if (Process32First(snapshot, &processEntry)) {
            do {
                std::string currentProcessName = processEntry.szExeFile;
                std::transform(currentProcessName.begin(), currentProcessName.end(),
                    currentProcessName.begin(), ::tolower);

                std::string searchName = processName;
                std::transform(searchName.begin(), searchName.end(),
                    searchName.begin(), ::tolower);

                if (currentProcessName.find(searchName) != std::string::npos) {
                    HWND window = findTargetWindow(processEntry.th32ProcessID);
                    if (window != NULL) {
                        hwnd = window;
                        break;
                    }
                }
            } while (Process32Next(snapshot, &processEntry));
        }
    }
    catch (...) {
        CloseHandle(snapshot);
        throw;
    }

    CloseHandle(snapshot);
    return hwnd;
}

bool WindowsUtility::isProcessElevated(HANDLE process) {
    BOOL isElevated = FALSE;
    HANDLE token = NULL;

    if (OpenProcessToken(process, TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);

        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isElevated = elevation.TokenIsElevated;
        }

        CloseHandle(token);
    }

    return isElevated;
}

bool WindowsUtility::needsElevation(HWND targetWindow) {
    DWORD targetProcessId;
    GetWindowThreadProcessId(targetWindow, &targetProcessId);

    HANDLE targetProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, targetProcessId);
    if (!targetProcess) {
        return false;
    }

    bool targetElevated = isProcessElevated(targetProcess);
    CloseHandle(targetProcess);

    HANDLE currentProcess = GetCurrentProcess();
    bool currentElevated = isProcessElevated(currentProcess);

    return targetElevated && !currentElevated;
}

bool WindowsUtility::restartWithElevation(const std::string& exePath) {
    SHELLEXECUTEINFOA shellExec = {};
    shellExec.cbSize = sizeof(SHELLEXECUTEINFOA);
    shellExec.lpVerb = "runas";
    shellExec.lpFile = exePath.c_str();
    shellExec.nShow = SW_NORMAL;

    return ShellExecuteExA(&shellExec);
}

bool WindowsUtility::canSendMessage(HWND targetWindow) {
    if (!isWindowValid(targetWindow)) {
        return false;
    }

    return !needsElevation(targetWindow);
}

bool WindowsUtility::sendWindowsMessage(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!canSendMessage(window)) {
        return false;
    }

    return PostMessage(window, msg, wParam, lParam) != 0;
}

std::string WindowsUtility::getLastErrorAsString() {
    DWORD error = GetLastError();
    if (error == 0) {
        return "";
    }

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        NULL
    );

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);

    return message;
}

HWND WindowsUtility::findTargetWindow(DWORD processId) {
    struct EnumData {
        DWORD processId;
        HWND window;
    };

    EnumData data = { processId, NULL };

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto data = reinterpret_cast<EnumData*>(lParam);
    DWORD windowProcessId;
    GetWindowThreadProcessId(hwnd, &windowProcessId);

    if (windowProcessId == data->processId) {
        data->window = hwnd;
        return FALSE;
    }
    return TRUE;
        }, reinterpret_cast<LPARAM>(&data));

    return data.window;
}

std::pair<int, int> WindowsUtility::getWindowDimensions(HWND window) {
    RECT rect;
    GetClientRect(window, &rect);
    return { rect.right - rect.left, rect.bottom - rect.top };
}

bool WindowsUtility::isWindowValid(HWND window) {
    return window != NULL && IsWindow(window);
}

void WindowsUtility::showMessageBox(const std::string& message, const std::string& title, UINT type) {
    MessageBoxA(NULL, message.c_str(), title.c_str(), type);
}

bool WindowsUtility::elevateWithPrompt(const std::string& reason) {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    int result = MessageBoxA(NULL,
        reason.c_str(),
        "Elevation Required",
        MB_ICONEXCLAMATION | MB_OKCANCEL);

    if (result == IDOK) {
        return restartWithElevation(exePath);
    }
    return false;
}