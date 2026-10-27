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

#endif // TINTA_SELF_INSTALL_H
