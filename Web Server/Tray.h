#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <functional>

struct tray_menu
{
    std::string text            { "" };
    int disabled                { 0 };
    int checked                 { 0 };
    void* context               { nullptr };
    tray_menu* submenu          { nullptr };
    std::function<void()> cb;

    tray_menu() = default;

    tray_menu(const std::string& _text, int _disabled, int _checked, std::function<void()> _cb, void* _context)
    {
        text = _text;
        disabled = _disabled;
        checked = _checked;
        cb = _cb;
        context = _context;
    }
};

struct tray
{
    std::string icon{ "" };
    std::vector<tray_menu> items;

    tray() = default;
    tray(const std::string& icon_name) : icon(icon_name) {}

    void addItem(tray_menu item)
    {
        items.push_back(item);
    }

    void addItem(const std::string& _text, int _disabled, int _checked, std::function<void()> _cb , void* _context)
    {
        items.push_back(tray_menu(_text, _disabled, _checked, _cb, _context));
    }
};

static void tray_update(struct tray* tray);

#define WM_TRAY_CALLBACK_MESSAGE (WM_USER + 1)
#define WC_TRAY_CLASS_NAME "TRAY"
#define ID_TRAY_FIRST 1000

static WNDCLASSEX wc;
static NOTIFYICONDATA nid;
static HWND hwnd;
static HMENU hmenu = NULL;

inline LRESULT CALLBACK _tray_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CLOSE:
    {
        DestroyWindow(hwnd);
        return 0;
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
        break;
    }
    case WM_TRAY_CALLBACK_MESSAGE:
    {
        if (lparam == WM_LBUTTONUP || lparam == WM_RBUTTONUP) {
            POINT p;
            GetCursorPos(&p);
            SetForegroundWindow(hwnd);
            WORD cmd = TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, p.x, p.y, 0, hwnd, NULL);
            SendMessage(hwnd, WM_COMMAND, cmd, 0);
            return 0;
        }
        break;
    }
    case WM_COMMAND:
    {
        if (wparam >= ID_TRAY_FIRST) {
            MENUITEMINFO item;
            item.cbSize = sizeof(MENUITEMINFO);
            item.fMask = MIIM_ID | MIIM_DATA;

            if (GetMenuItemInfo(hmenu, wparam, FALSE, &item)) {
                tray_menu* menu = (tray_menu*)item.dwItemData;
                if (menu != nullptr && menu->cb != nullptr) {
                    menu->cb();
                }
            }
            return 0;
        }
        break;
    }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

inline HMENU _tray_menu(const std::vector<tray_menu>& items, unsigned int* id)
{
    HMENU hmenu = CreatePopupMenu();
    for (const auto& i : items)
    {
        if (i.text == "-") {
            InsertMenu(hmenu, *id, MF_SEPARATOR, TRUE, (LPCWSTR)"");
        } else {
            MENUITEMINFO item;
            memset(&item, 0, sizeof(item));
            item.cbSize = sizeof(MENUITEMINFO);
            item.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
            item.fType = 0;
            item.fState = 0;
            if (i.submenu != NULL) {
                item.fMask = item.fMask | MIIM_SUBMENU;
                item.hSubMenu = _tray_menu({ *i.submenu }, id);
            }

            if (i.disabled) {
                item.fState |= MFS_DISABLED;
            }

            if (i.checked) {
                item.fState |= MFS_CHECKED;
            }

            item.wID = *id;
            wchar_t title[32];
            size_t str_size = i.text.size();
            mbstowcs_s(&str_size, title, i.text.c_str(), i.text.length());
            item.dwTypeData = title;
            item.dwItemData = (ULONG_PTR)&i;
            
            InsertMenuItem(hmenu, *id, TRUE, &item);
        }
        (*id)++;
    }
    return hmenu;
}

inline int tray_init(tray* tray)
{
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = _tray_wnd_proc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = (LPCWSTR)WC_TRAY_CLASS_NAME;

    if (!RegisterClassEx(&wc)) { return -1; }

    hwnd = CreateWindowEx(0, (LPCWSTR)WC_TRAY_CLASS_NAME, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (hwnd == NULL) { return -1; }

    UpdateWindow(hwnd);

    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE;
    nid.uCallbackMessage = WM_TRAY_CALLBACK_MESSAGE;
    Shell_NotifyIcon(NIM_ADD, &nid);

    tray_update(tray);
    return 0;
}

inline int tray_loop(bool blocking)
{
    MSG msg;
    
    if (blocking) {
        GetMessage(&msg, NULL, 0, 0);
    } else {
        PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
    }

    if (msg.message == WM_QUIT) {
        return -1;
    }

    TranslateMessage(&msg);
    DispatchMessage(&msg);
    return 0;
}

inline void tray_update(tray* tray)
{
    HMENU prevmenu = hmenu;
    UINT id = ID_TRAY_FIRST;
    hmenu = _tray_menu(tray->items, &id);
    SendMessage(hwnd, WM_INITMENUPOPUP, (WPARAM)hmenu, 0);
    HICON icon;
    ExtractIconExA(tray->icon.c_str(), 0, NULL, &icon, 1);

    if (nid.hIcon) {
        DestroyIcon(nid.hIcon);
    }

    nid.hIcon = icon;
    Shell_NotifyIcon(NIM_MODIFY, &nid);

    if (prevmenu != NULL) {
        DestroyMenu(prevmenu);
    }
}

inline void tray_exit()
{
    Shell_NotifyIcon(NIM_DELETE, &nid);

    if (nid.hIcon != 0) {
        DestroyIcon(nid.hIcon);
    }

    if (hmenu != 0) {
        DestroyMenu(hmenu);
    }

    PostQuitMessage(0);
    UnregisterClass((LPCWSTR)WC_TRAY_CLASS_NAME, GetModuleHandle(NULL));
}