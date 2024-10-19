// The UI was 99% AI generated.

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <filesystem>
#include "PlayerFunctionality.h"


#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")

// Global variables
HDC hdcMem = NULL;
HBITMAP hbmMem = NULL;
HBITMAP hbmOld = NULL;
HWND hList, hPlayButton, hStopButton;
std::vector<std::string> files;
HFONT hFont;
HBRUSH hBackgroundBrush, hButtonBrush, hButtonHoverBrush, hListBorderBrush;

// Colors
#define COLOR_BACKGROUND RGB(30, 30, 30)
#define COLOR_TEXT RGB(200, 200, 200)
#define COLOR_BUTTON RGB(60, 60, 60)
#define COLOR_BUTTON_HOVER RGB(80, 80, 80)
#define COLOR_LIST_BORDER RGB(100, 100, 100)
#define COLOR_LIST_BACKGROUND RGB(40, 40, 40)

// Function prototypes
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void LoadSongs(const std::string& folder);
void ResizeControls(HWND hwnd);
void DrawButton(LPDRAWITEMSTRUCT lpDrawItem);
LRESULT CALLBACK ListBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Button IDs
#define ID_LISTBOX 1
#define ID_PLAY_BUTTON 2
#define ID_STOP_BUTTON 3

// Original ListBox procedure
WNDPROC OriginalListBoxProc;
bool isPlayingSong = false;
std::thread songPlayerThread;
std::string selectedSongFileName = "";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // Register the window class
    const char CLASS_NAME[] = "Sample Window Class";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&wc);

    // Create the window
    HWND hwnd = CreateWindowExA(
        0,                              // Optional window styles
        CLASS_NAME,                     // Window class
        "Guitar Player",                    // Window text
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 450,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    // Enable dark mode for the title bar
    BOOL value = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    // Pre-scroll the listbox

    ShowWindow(hwnd, nCmdShow);
    SendMessage(hList, WM_VSCROLL, SB_LINEDOWN, 0); // This is here to prevent some stupid flickering from non-styled text.
    SendMessage(hList, WM_VSCROLL, SB_LINEUP, 0);
    UpdateWindow(hwnd);

    // Run the message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Create brushes
        hBackgroundBrush = CreateSolidBrush(COLOR_BACKGROUND);
        hButtonBrush = CreateSolidBrush(COLOR_BUTTON);
        hButtonHoverBrush = CreateSolidBrush(COLOR_BUTTON_HOVER);
        hListBorderBrush = CreateSolidBrush(COLOR_LIST_BORDER);

        // Create a nicer font
        hFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        // Create listbox (removed WS_VSCROLL style)
        hList = CreateWindowExA(
            0,
            "LISTBOX",
            "",
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            10, 10, 260, 410,
            hwnd,
            (HMENU)ID_LISTBOX,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL);



        // Set colors for listbox
        SendMessage(hList, LB_SETCURSEL, (WPARAM)-1, 0);
        SendMessage(hList, LB_SETSEL, FALSE, -1);

        // Subclass the listbox to customize its appearance
        OriginalListBoxProc = (WNDPROC)SetWindowLongPtr(hList, GWLP_WNDPROC, (LONG_PTR)ListBoxProc);

        // Create Play button
        hPlayButton = CreateWindowExA(
            0,
            "BUTTON",
            "Play",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            10, 370, 100, 30,
            hwnd,
            (HMENU)ID_PLAY_BUTTON,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL);

        // Create Stop button
        hStopButton = CreateWindowExA(
            0,
            "BUTTON",
            "Stop",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            170, 370, 100, 30,
            hwnd,
            (HMENU)ID_STOP_BUTTON,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL);

        // Apply the font to all controls
        SendMessage(hList, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hPlayButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hStopButton, WM_SETFONT, (WPARAM)hFont, TRUE);

        LoadSongs("songs");

        for (const auto& file : files)
        {
            SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)file.c_str());
        }
    }
    return 0;

    case WM_SIZE:
        ResizeControls(hwnd);
        return 0;

    case WM_CTLCOLORLISTBOX:
    {
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_DRAWITEM:
        DrawButton((LPDRAWITEMSTRUCT)lParam);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_LISTBOX:
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                int selectedIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
                if (selectedIndex != LB_ERR)
                {
                    char selectedFile[MAX_PATH];
                    SendMessageA(hList, LB_GETTEXT, selectedIndex, (LPARAM)selectedFile);
                    UpdateWindow(hwnd);
                    selectedSongFileName = selectedFile;
                    //MessageBoxA(hwnd, selectedFile, "Selected Song", MB_OK);
                }
            }
            break;
        case ID_PLAY_BUTTON:
            if (selectedSongFileName != "" && isPlayingSong == false) {
                if (songPlayerThread.joinable()) {
                    songPlayerThread.join();
                }
                isPlayingSong = true;
                songPlayerThread = std::thread(PlaySong, selectedSongFileName, std::ref(isPlayingSong));

                //MessageBoxA(hwnd, "Play button clicked", selectedSongFileName.c_str(), MB_OK);
            }
            break;
        case ID_STOP_BUTTON:
            isPlayingSong = false;
            songPlayerThread.join();
            //MessageBoxA(hwnd, "Stop button clicked", "Action", MB_OK);
            break;
        }
        return 0;

    case WM_DESTROY:
        if (hdcMem)
        {
            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
        }
        DeleteObject(hFont);
        DeleteObject(hBackgroundBrush);
        DeleteObject(hButtonBrush);
        DeleteObject(hButtonHoverBrush);
        DeleteObject(hListBorderBrush);
        PostQuitMessage(0);
        return 0;

    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, hBackgroundBrush);
        return 1;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void LoadSongs(const std::string& folder)
{
    for (const auto& entry : std::filesystem::directory_iterator(folder))
    {
        if (entry.is_regular_file())
        {
            files.push_back(entry.path().filename().string());
        }
    }
}

void ResizeControls(HWND hwnd)
{
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);

    // Calculate new positions and sizes
    int listWidth = rcClient.right - 20;
    int listHeight = rcClient.bottom - 70; // Increased space between list and buttons
    int buttonY = rcClient.bottom - 45; // Moved buttons up slightly
    int buttonWidth = (listWidth - 10) / 2;

    // Resize and move controls
    SetWindowPos(hList, NULL, 10, 10, listWidth, listHeight, SWP_NOZORDER);
    SetWindowPos(hPlayButton, NULL, 10, buttonY, buttonWidth, 35, SWP_NOZORDER);
    SetWindowPos(hStopButton, NULL, 20 + buttonWidth, buttonY, buttonWidth, 35, SWP_NOZORDER);

    // Force redraw
    InvalidateRect(hwnd, NULL, TRUE);
}

void DrawButton(LPDRAWITEMSTRUCT lpDrawItem)
{
    HDC hDC = lpDrawItem->hDC;
    RECT rcItem = lpDrawItem->rcItem;
    UINT state = lpDrawItem->itemState;

    // Set the background color
    HBRUSH hBrush = (state & ODS_SELECTED) ? hButtonHoverBrush : hButtonBrush;
    FillRect(hDC, &rcItem, hBrush);

    // Draw the text
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, COLOR_TEXT);

    char buttonText[256];
    GetWindowTextA(lpDrawItem->hwndItem, buttonText, sizeof(buttonText));

    DrawTextA(hDC, buttonText, -1, &rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Draw the focus rectangle if the button has focus
    if (state & ODS_FOCUS)
    {
        DrawFocusRect(hDC, &rcItem);
    }
}
LRESULT CALLBACK ListBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_NCCALCSIZE:
        return 0;  // Prevent default non-client area painting
    case WM_ERASEBKGND:
        return 1; // Indicate that we'll handle background erasing in WM_PAINT

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Get the client rect
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        // Create a memory DC and bitmap for double buffering if not already created
        if (!hdcMem)
        {
            hdcMem = CreateCompatibleDC(hdc);
            hbmMem = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
            hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        }

        // Fill the background
        FillRect(hdcMem, &clientRect, CreateSolidBrush(COLOR_LIST_BACKGROUND));

        // Get the number of items and the top index
        int itemCount = SendMessage(hwnd, LB_GETCOUNT, 0, 0);
        int topIndex = SendMessage(hwnd, LB_GETTOPINDEX, 0, 0);

        // Get item height
        int itemHeight = SendMessage(hwnd, LB_GETITEMHEIGHT, 0, 0);

        // Calculate how many items can be fully displayed
        int visibleItems = (clientRect.bottom - clientRect.top) / itemHeight;

        // Select our custom font and set text color
        HFONT oldFont = (HFONT)SelectObject(hdcMem, hFont);
        SetTextColor(hdcMem, COLOR_TEXT);
        SetBkMode(hdcMem, TRANSPARENT);
        // Get the selected index
        int selectedIndex = SendMessage(hwnd, LB_GETCURSEL, 0, 0);

        // Draw each fully visible item
        for (int i = topIndex; i < min(itemCount, topIndex + visibleItems); i++)
        {
            RECT itemRect = clientRect;
            itemRect.top = (i - topIndex) * itemHeight;
            itemRect.bottom = itemRect.top + itemHeight;

            // Skip drawing if the item is not fully visible
            if (itemRect.bottom > clientRect.bottom)
                break;

            // Get item text
            char buffer[256];
            SendMessage(hwnd, LB_GETTEXT, i, (LPARAM)buffer);

            // Draw selection background if this item is selected
            if (i == selectedIndex)
            {
                FillRect(hdcMem, &itemRect, CreateSolidBrush(RGB(60, 60, 60))); // Darker background for selected item
            }

            itemRect.left += 5;
            itemRect.right -= 5;
            // Draw item text
            DrawTextA(hdcMem, buffer, -1, &itemRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
        }

        // Draw the border
        FrameRect(hdcMem, &clientRect, hListBorderBrush);

        // Copy the memory DC to the window DC
        BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

        // Restore the old font
        SelectObject(hdcMem, oldFont);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        // Handle mouse clicks to update selection
        InvalidateRect(hwnd, NULL, FALSE);
        break;



    case WM_MOUSEWHEEL:
    {
        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        int nScrollLines = 3; // You can adjust this value to change scroll speed
        if (zDelta > 0)
        {
            // Scroll up
            for (int i = 0; i < nScrollLines; i++)
                SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
        }
        else
        {
            // Scroll down
            for (int i = 0; i < nScrollLines; i++)
                SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_UP:
            SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
            return 0;
        case VK_DOWN:
            SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
            return 0;
        case VK_PRIOR:  // Page Up
            SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0);
            return 0;
        case VK_NEXT:   // Page Down
            SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0);
            return 0;
        }
        break;

    case WM_VSCROLL:
    {
        int topIndex = SendMessage(hwnd, LB_GETTOPINDEX, 0, 0);
        int itemCount = SendMessage(hwnd, LB_GETCOUNT, 0, 0);

        switch (LOWORD(wParam))
        {
        case SB_LINEUP:
            topIndex = max(topIndex - 1, 0);
            break;
        case SB_LINEDOWN:
            topIndex = min(topIndex + 1, itemCount - 1);
            break;
        case SB_PAGEUP:
        {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            int itemHeight = SendMessage(hwnd, LB_GETITEMHEIGHT, 0, 0);
            int visibleItems = (clientRect.bottom - clientRect.top) / itemHeight;
            topIndex = max(topIndex - visibleItems, 0);
        }
        break;
        case SB_PAGEDOWN:
        {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            int itemHeight = SendMessage(hwnd, LB_GETITEMHEIGHT, 0, 0);
            int visibleItems = (clientRect.bottom - clientRect.top) / itemHeight;
            topIndex = min(topIndex + visibleItems, itemCount - 1);
        }
        break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            topIndex = HIWORD(wParam);
            break;
        }

        SendMessage(hwnd, LB_SETTOPINDEX, topIndex, 0);
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);

        return 0;
    }
    }
    return CallWindowProc(OriginalListBoxProc, hwnd, uMsg, wParam, lParam);
}