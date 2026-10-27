#ifndef TINTA_SELF_INSTALL_H
#define TINTA_SELF_INSTALL_H

#include "app.h"

#include <string>

// Personal: a tinta.exe started from anywhere but Program Files offers to
// install itself to C:\Program Files\Tinta, register .md files there and
// reopen from there. Declining is remembered for that copy's path.

// The elevated half: "tinta.exe --install-copy" copies the running exe to
// the fixed Program Files target. Returns the process exit code.
int runInstallCopy();

// Startup check. True when Tinta was installed and relaunched from Program
// Files, so this process should exit.
bool offerProgramFilesInstall(Settings& settings, const std::string& inputFile);

// Update chip: downloads the fork's latest tinta.exe to %TEMP%\tinta-update
// and starts it to install itself once this Tinta closes. False when the
// download fails.
bool startSelfUpdate();

// "tinta.exe --update-install": waits for the installed Tinta to close,
// copies itself over it (Windows asks for permission) and reopens it.
int runUpdateInstall();

// Removes what an install or update left behind: the downloaded update and
// the copy an install started from (only if identical to the installed
// exe). Retries in the background while those processes exit.
void cleanupLeftovers(const std::wstring& requested);

#endif // TINTA_SELF_INSTALL_H
