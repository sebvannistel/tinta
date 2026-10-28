#include "file_icon.h"

#include <windows.h>

#include <fstream>
#include <iostream>
#include <iterator>

// Personal: the borrowed Markdown file icon (resolving DefaultIcon values,
// wrapping a PNG as .ico, copying an exe's icon)

static int failures = 0;

void check(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << "\n";
        failures++;
    }
}

static std::string pngHeader(uint32_t width, uint32_t height) {
    std::string png("\x89PNG\r\n\x1a\n\0\0\0\x0dIHDR", 16);
    for (uint32_t v : {width, height})
        for (int shift = 24; shift >= 0; shift -= 8) png += (char)((v >> shift) & 0xff);
    return png + std::string(13, '\0');
}

int main() {
    std::wstring path;
    int index = 7;
    check(resolveIconSource(L"C:\\Apps\\md.exe,-101", path, index) &&
          path == L"C:\\Apps\\md.exe" && index == -101, "resource id after the comma");
    check(resolveIconSource(L"\"C:\\Program Files\\md\\md.exe\",0", path, index) &&
          path == L"C:\\Program Files\\md\\md.exe" && index == 0, "quoted path with index");
    check(resolveIconSource(L"%SystemRoot%\\md.ico", path, index) && index == 0 &&
          path.find(L'%') == std::wstring::npos && path.size() > 7, "environment expanded");
    check(resolveIconSource(L"C:\\a,b\\md.ico", path, index) && path == L"C:\\a,b\\md.ico",
          "comma in a folder name is not an index");
    check(!resolveIconSource(L"@{Missing.Package_1.0.0.0_x64__abc?ms-resource://Missing/Files/a.png}",
                             path, index), "uninstalled Store package resolves to nothing");

    std::string png = pngHeader(256, 256);
    std::string ico = wrapPngAsIco(png);
    check(ico.size() == 22 + png.size(), "one entry header before the PNG");
    check(ico.compare(0, 6, std::string("\0\0\1\0\1\0", 6)) == 0, "ICONDIR for one icon");
    check(ico[6] == 0 && ico[7] == 0, "256px stored as 0");
    check((uint8_t)ico[18] == 22 && ico.compare(22, std::string::npos, png) == 0,
          "PNG at offset 22");
    std::string smaller = wrapPngAsIco(pngHeader(48, 32));
    check((uint8_t)smaller[6] == 48 && (uint8_t)smaller[7] == 32, "smaller sizes kept");
    check(wrapPngAsIco(pngHeader(512, 512)).empty(), "over 256px is not an icon entry");
    check(wrapPngAsIco("GIF89a not a png at all").empty(), "non-PNG rejected");

    // A real exe resource round-trips into an .ico Windows can load
    wchar_t temp[MAX_PATH];
    GetTempPathW(MAX_PATH, temp);
    std::wstring dest = std::wstring(temp) + L"tinta-file-icon-test.ico";
    DeleteFileW(dest.c_str());
    check(copyFileIcon(L"%SystemRoot%\\System32\\shell32.dll,0", dest), "copy shell32 icon");
    for (int size : {16, 32, 256}) {
        HICON loaded = (HICON)LoadImageW(nullptr, dest.c_str(), IMAGE_ICON, size, size,
                                         LR_LOADFROMFILE);
        check(loaded != nullptr, "copied icon loads at 16, 32 and 256px");
        if (loaded) DestroyIcon(loaded);
    }
    {
        // Explorer's small sizes are stored, not shrunk from 256px on the fly
        std::ifstream in(dest, std::ios::binary);
        std::string bytes((std::istreambuf_iterator<char>(in)), {});
        check(bytes.size() > 6 && (uint8_t)bytes[4] == 10, "ten sizes stored");
        check(bytes.size() > 7 && (uint8_t)bytes[6] == 16 && (uint8_t)bytes[7] == 16,
              "16px entry first");
    }

    std::string two = encodeIco({pngHeader(16, 16), pngHeader(32, 32)});
    check((uint8_t)two[4] == 2 && (uint8_t)two[18] == 38 &&
          (uint8_t)two[34] == 38 + pngHeader(16, 16).size(), "entries point past both headers");
    DeleteFileW(dest.c_str());
    check(!copyFileIcon(L"C:\\no\\such\\app.exe,0", dest), "missing source copies nothing");

    if (failures) return 1;
    std::cout << "file_icon tests passed\n";
    return 0;
}
