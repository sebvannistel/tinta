#include "self_install.h"
#include "editor.h"
#include "settings.h"
#include "utils.h"

#include <windows.h>
#include <appmodel.h>
#include <shellapi.h>
#include <shlobj.h>

#include <fstream>
#include <iterator>

namespace {

std::wstring currentExePath() {
    std::wstring path(MAX_PATH, L'\0');
    for (;;) {
        DWORD length = GetModuleFileNameW(nullptr, path.data(), (DWORD)path.size());
        if (length == 0) return {};
        if (length < path.size()) { path.resize(length); return path; }
        path.resize(path.size() * 2);
    }
}

// C:\Program Files\Tinta\tinta.exe; the only place the elevated copy writes
std::wstring installTarget() {
    PWSTR folder = nullptr;
    std::wstring target;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, nullptr, &folder))) {
        target = std::wstring(folder) + L"\\Tinta\\tinta.exe";
    }
    CoTaskMemFree(folder);
    return target;
}

bool fileExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

std::string fileBytes(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

bool isPackaged() {
    UINT32 length = 0;
    return GetCurrentPackageFullName(&length, nullptr) != APPMODEL_ERROR_NO_PACKAGE;
}

}  // namespace

int runInstallCopy() {
    std::wstring source = currentExePath();
    std::wstring target = installTarget();
    if (source.empty() || target.empty()) return 2;
    std::wstring dir = target.substr(0, target.find_last_of(L'\\'));
    if (!CreateDirectoryW(dir.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return 3;
    return CopyFileW(source.c_str(), target.c_str(), FALSE) ? 0 : 4;
}

bool offerProgramFilesInstall(Settings& settings, const std::string& inputFile) {
    if (isPackaged()) return false;
    std::wstring exe = currentExePath();
    std::wstring target = installTarget();
    if (exe.empty() || target.empty()) return false;
    if (_wcsicmp(exe.c_str(), target.c_str()) == 0) return false;
    // Portable mode: a settings.ini beside the exe means "run from here"
    std::wstring exeDir = exe.substr(0, exe.find_last_of(L"\\/"));
    if (fileExists(exeDir + L"\\settings.ini")) return false;
    if (settings.installDeclinedPath == toUtf8(exe)) return false;
    // Already installed, byte for byte: nothing to offer
    bool installed = fileExists(target);
    if (installed && fileBytes(exe) == fileBytes(target)) return false;

    std::wstring folder = target.substr(0, target.find_last_of(L'\\'));
    std::wstring message = installed
        ? L"Another version of Tinta is installed in " + folder +
              L".\n\nReplace it with this one? Windows will ask for permission."
        : L"Install Tinta to " + folder +
              L"?\n\nTinta will be copied there and .md files will open with it. "
              L"Windows will ask for permission.";
    message += L"\n\nChoose No to keep running Tinta from where it is; "
               L"this copy will not ask again.";
    int choice = MessageBoxW(nullptr, message.c_str(), L"Install Tinta",
                             MB_YESNO | MB_ICONQUESTION);
    if (choice != IDYES) {
        settings.installDeclinedPath = toUtf8(exe);
        saveSettings(settings);
        return false;
    }

    SHELLEXECUTEINFOW info{sizeof(info)};
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"runas";
    info.lpFile = exe.c_str();
    info.lpParameters = L"--install-copy";
    info.nShow = SW_HIDE;
    if (!ShellExecuteExW(&info) || !info.hProcess) {
        // Cancelled at the Windows prompt: keep running, ask again next time
        return false;
    }
    WaitForSingleObject(info.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(info.hProcess, &exitCode);
    CloseHandle(info.hProcess);
    if (exitCode != 0 || !fileExists(target)) {
        MessageBoxW(nullptr,
                    L"Tinta could not be copied to Program Files. If the installed "
                    L"Tinta is open, close it and start this one again.",
                    L"Install Tinta", MB_OK | MB_ICONWARNING);
        return false;
    }

    registerFileAssociation(target.c_str());
    settings.hasAskedFileAssociation = true;
    saveSettings(settings);

    std::wstring args;
    if (!inputFile.empty()) args = L"\"" + toWide(inputFile) + L"\"";
    ShellExecuteW(nullptr, L"open", target.c_str(), args.empty() ? nullptr : args.c_str(),
                  nullptr, SW_SHOWNORMAL);
    return true;
}
