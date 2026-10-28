#ifndef TINTA_FILE_ICON_H
#define TINTA_FILE_ICON_H

#include <string>
#include <vector>

// Personal: Markdown files can show another installed Markdown app's file
// icon instead of Tinta's. The icon is copied into Tinta's config folder,
// so nothing of the other app ships in this repo, and it keeps working
// after that app is uninstalled.

struct BorrowableFileIcon {
    std::wstring appName;     // e.g. "Typedown"
    std::wstring iconSource;  // the app's DefaultIcon value
};

// Other apps registered for .md that name a file icon
std::vector<BorrowableFileIcon> findBorrowableFileIcons();

// Where the copied icon lives: <config dir>\file-icon.ico (empty if no dir)
std::wstring borrowedFileIconPath();
bool hasBorrowedFileIcon();

// Splits a DefaultIcon value into a file and icon index, resolving Store
// apps' "@{package?ms-resource://...}" form to the asset file.
bool resolveIconSource(const std::wstring& iconSource, std::wstring& path, int& index);

// An .ico holding the PNG images (Vista+ reads PNG entries). Empty if any
// is not a PNG of at most 256px.
std::string encodeIco(const std::vector<std::string>& pngs);
std::string wrapPngAsIco(const std::string& png);

// Writes the icon named by iconSource (.png, .ico or an exe/dll resource)
// to destIco as an .ico file with every Explorer size, each resized from
// the largest image.
bool copyFileIcon(const std::wstring& iconSource, const std::wstring& destIco);

#endif
