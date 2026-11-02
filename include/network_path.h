#ifndef TINTA_NETWORK_PATH_H
#define TINTA_NETWORK_PATH_H

#include <cwchar>
#include <cwctype>
#include <string>

namespace qmd {

// The computer a file path reaches, in lower case: empty for this one
// ("C:\notes\a.png", "\\?\C:\notes", a relative path), otherwise the
// server of a UNC path. Two leading separators of either kind open one
// ("\\nas\docs", "//nas/docs", "/\nas\docs", "\\?\UNC\nas\docs"). Other
// device paths (\\.\pipe\, \\?\GLOBALROOT\) answer "?", which names no
// server.
inline std::wstring pathHost(const std::wstring& path) {
    auto separator = [](wchar_t c) { return c == L'\\' || c == L'/'; };
    if (path.size() < 2 || !separator(path[0]) || !separator(path[1])) return {};
    size_t start = 2;
    if (path.size() >= 4 && (path[2] == L'?' || path[2] == L'.') && separator(path[3])) {
        const wchar_t drive = static_cast<wchar_t>(path.size() >= 6 ? path[4] | 0x20 : 0);
        if (drive >= L'a' && drive <= L'z' && path[5] == L':') return {};
        if (path.size() < 8 || _wcsnicmp(path.c_str() + 4, L"UNC", 3) != 0 || !separator(path[7]))
            return L"?";
        start = 8;
    }
    size_t end = start;
    while (end < path.size() && !separator(path[end])) end++;
    if (end == start) return L"?";
    std::wstring host = path.substr(start, end - start);
    for (auto& c : host) c = static_cast<wchar_t>(std::towlower(c));
    return host;
}

// Whether a document may have Tinta open a file it names without being
// asked, to draw an image or badge a link: anything on this computer, a
// network path only on the server the document's own folder is on.
// Opening a share signs in to its server with the user's Windows
// credentials, so a document must not get to pick the server.
inline bool mayOpenFromDocument(const std::wstring& path, const std::wstring& documentFolder) {
    const std::wstring host = pathHost(path);
    return host.empty() || (host != L"?" && host == pathHost(documentFolder));
}

}  // namespace qmd

#endif
