#include "self_install.h"
#include "editor.h"
#include "settings.h"
#include "utils.h"

#include <windows.h>
#include <appmodel.h>
#include <shellapi.h>
#include <shlobj.h>
#include <tlhelp32.h>
#include <urlmon.h>

#include <fstream>
#include <iterator>
#include <thread>

namespace {

// Personal builds are released on the fork, merged with the owner's code
const wchar_t kLatestExeUrl[] =
    L"https://github.com/sebvannistel/tinta/releases/latest/download/tinta.exe";

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

// %TEMP%\tinta-update: where an update is downloaded, removed afterwards
std::wstring updateFolder() {
    wchar_t temp[MAX_PATH];
    DWORD length = GetTempPathW(MAX_PATH, temp);
    if (length == 0 || length >= MAX_PATH) return {};
    return std::wstring(temp) + L"tinta-update";
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

// A local fixed drive: the copy an install started from is removed only
// there, never from a USB stick or a network share
bool onLocalFixedDrive(const std::wstring& path) {
    if (path.size() < 3 || path[1] != L':') return false;
    std::wstring root = path.substr(0, 3);
    return GetDriveTypeW(root.c_str()) == DRIVE_FIXED;
}

enum class CopyResult { Done, Cancelled, Failed };

// Copies this exe into Program Files through an elevated run of itself
CopyResult elevatedCopy(const std::wstring& exe, const std::wstring& target) {
    SHELLEXECUTEINFOW info{sizeof(info)};
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"runas";
    info.lpFile = exe.c_str();
    info.lpParameters = L"--install-copy";
    info.nShow = SW_HIDE;
    if (!ShellExecuteExW(&info) || !info.hProcess) return CopyResult::Cancelled;
    WaitForSingleObject(info.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(info.hProcess, &exitCode);
    CloseHandle(info.hProcess);
    return exitCode == 0 && fileExists(target) ? CopyResult::Done : CopyResult::Failed;
}

// Waits until no process runs the installed exe, which locks it
void waitForInstalledToExit(const std::wstring& target) {
    DWORD self = GetCurrentProcessId();
    for (;;) {
        bool running = false;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W entry{sizeof(entry)};
            for (BOOL ok = Process32FirstW(snapshot, &entry); ok && !running;
                 ok = Process32NextW(snapshot, &entry)) {
                if (entry.th32ProcessID == self || _wcsicmp(entry.szExeFile, L"tinta.exe") != 0) continue;
                HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
                if (!process) continue;
                wchar_t image[MAX_PATH * 2];
                DWORD size = (DWORD)std::size(image);
                if (QueryFullProcessImageNameW(process, 0, image, &size) &&
                    _wcsicmp(image, target.c_str()) == 0) {
                    running = true;
                }
                CloseHandle(process);
            }
            CloseHandle(snapshot);
        }
        if (!running) return;
        Sleep(500);
    }
}

void launch(const std::wstring& exe, const std::wstring& args) {
    ShellExecuteW(nullptr, L"open", exe.c_str(), args.empty() ? nullptr : args.c_str(),
                  nullptr, SW_SHOWNORMAL);
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

    bool removeAfter = onLocalFixedDrive(exe);
    std::wstring folder = target.substr(0, target.find_last_of(L'\\'));
    std::wstring message = installed
        ? L"Another version of Tinta is installed in " + folder +
              L".\n\nReplace it with this one? Windows will ask for permission."
        : L"Install Tinta to " + folder +
              L"?\n\nTinta will be copied there and .md files will open with it. "
              L"Windows will ask for permission.";
    if (removeAfter) message += L"\n\nThis copy is deleted afterwards, so no extra tinta.exe is left behind.";
    message += L"\n\nChoose No to keep running Tinta from where it is; "
               L"this copy will not ask again.";
    int choice = MessageBoxW(nullptr, message.c_str(), L"Install Tinta",
                             MB_YESNO | MB_ICONQUESTION);
    if (choice != IDYES) {
        settings.installDeclinedPath = toUtf8(exe);
        saveSettings(settings);
        return false;
    }

    CopyResult copied = elevatedCopy(exe, target);
    // Cancelled at the Windows prompt: keep running, ask again next time
    if (copied == CopyResult::Cancelled) return false;
    if (copied == CopyResult::Failed) {
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
    if (removeAfter) args += (args.empty() ? L"" : L" ") + std::wstring(L"--cleanup \"") + exe + L"\"";
    launch(target, args);
    return true;
}

bool startSelfUpdate() {
    std::wstring folder = updateFolder();
    if (folder.empty()) return false;
    CreateDirectoryW(folder.c_str(), nullptr);
    std::wstring download = folder + L"\\tinta.exe";
    DeleteFileW(download.c_str());
    // The query string keeps a cached older download from being reused
    std::wstring url = std::wstring(kLatestExeUrl) + L"?t=" + std::to_wstring(GetTickCount64());
    HCURSOR cursor = SetCursor(LoadCursor(nullptr, IDC_WAIT));
    HRESULT hr = URLDownloadToFileW(nullptr, url.c_str(), download.c_str(), 0, nullptr);
    SetCursor(cursor);
    std::string bytes = SUCCEEDED(hr) ? fileBytes(download) : std::string();
    if (bytes.size() < 1024 * 1024 || bytes.compare(0, 2, "MZ") != 0) {
        DeleteFileW(download.c_str());
        RemoveDirectoryW(folder.c_str());
        return false;
    }
    launch(download, L"--update-install");
    return true;
}

int runUpdateInstall() {
    std::wstring exe = currentExePath();
    std::wstring target = installTarget();
    if (exe.empty() || target.empty()) return 2;
    waitForInstalledToExit(target);
    CopyResult copied = elevatedCopy(exe, target);
    if (copied == CopyResult::Done) {
        registerFileAssociation(target.c_str());
    } else if (copied == CopyResult::Failed) {
        MessageBoxW(nullptr, L"The Tinta update could not be installed.",
                    L"Update Tinta", MB_OK | MB_ICONWARNING);
    }
    // Back to Tinta either way; it removes this downloaded copy
    launch(target, L"--cleanup \"" + exe + L"\"");
    return 0;
}

void cleanupLeftovers(const std::wstring& requested) {
    std::wstring target = installTarget();
    std::wstring folder = updateFolder();
    std::thread([requested, target, folder] {
        // The copy that started us exits right after; give it time
        for (int attempt = 0; attempt < 60; ++attempt) {
            bool pending = false;
            // Only a file identical to the installed Tinta is removed
            if (!requested.empty() && fileExists(requested) && !target.empty() &&
                _wcsicmp(requested.c_str(), target.c_str()) != 0) {
                if (fileBytes(requested) == fileBytes(target)) {
                    if (!DeleteFileW(requested.c_str())) pending = true;
                }
            }
            if (!folder.empty() && GetFileAttributesW(folder.c_str()) != INVALID_FILE_ATTRIBUTES) {
                std::wstring download = folder + L"\\tinta.exe";
                if (fileExists(download) && !DeleteFileW(download.c_str())) pending = true;
                else if (!RemoveDirectoryW(folder.c_str())) pending = true;
            }
            if (!pending) return;
            Sleep(500);
        }
    }).detach();
}
