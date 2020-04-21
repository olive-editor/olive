/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "crashhandler.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <QtGlobal>

#if defined(Q_OS_WINDOWS)
#include <Windows.h>
#include <DbgHelp.h>
#include <stdio.h>
#include <stdlib.h>
#elif defined(Q_OS_MAC) || defined(Q_OS_LINUX)
#include <execinfo.h>
#endif

#include "common/filefunctions.h"

OLIVE_NAMESPACE_ENTER

void crash_handler(int sig) {
  QString log_path = QDir(FileFunctions::GetTempPath()).filePath(QStringLiteral("olive_crash"));
  QFile output(log_path);

  output.open(QFile::WriteOnly);
  QTextStream ostream(&output);

  ostream << "Signal: " << sig << "\n\n";

#if defined(Q_OS_WINDOWS)
  // Use Windows stackwalk API
  HANDLE process = GetCurrentProcess();
  HANDLE thread = GetCurrentThread();

  CONTEXT context;
  memset(&context, 0, sizeof(CONTEXT));
  context.ContextFlags = CONTEXT_FULL;
  RtlCaptureContext(&context);

  SymInitialize(process, NULL, TRUE);

  DWORD image;
  STACKFRAME64 stackframe;
  ZeroMemory(&stackframe, sizeof(STACKFRAME64));

#ifdef _M_IX86
  image = IMAGE_FILE_MACHINE_I386;
  stackframe.AddrPC.Offset = context.Eip;
  stackframe.AddrPC.Mode = AddrModeFlat;
  stackframe.AddrFrame.Offset = context.Ebp;
  stackframe.AddrFrame.Mode = AddrModeFlat;
  stackframe.AddrStack.Offset = context.Esp;
  stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
  image = IMAGE_FILE_MACHINE_AMD64;
  stackframe.AddrPC.Offset = context.Rip;
  stackframe.AddrPC.Mode = AddrModeFlat;
  stackframe.AddrFrame.Offset = context.Rsp;
  stackframe.AddrFrame.Mode = AddrModeFlat;
  stackframe.AddrStack.Offset = context.Rsp;
  stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
  image = IMAGE_FILE_MACHINE_IA64;
  stackframe.AddrPC.Offset = context.StIIP;
  stackframe.AddrPC.Mode = AddrModeFlat;
  stackframe.AddrFrame.Offset = context.IntSp;
  stackframe.AddrFrame.Mode = AddrModeFlat;
  stackframe.AddrBStore.Offset = context.RsBSP;
  stackframe.AddrBStore.Mode = AddrModeFlat;
  stackframe.AddrStack.Offset = context.IntSp;
  stackframe.AddrStack.Mode = AddrModeFlat;
#endif

  for (int i = 0; i < 50; i++) {

    BOOL result = StackWalk64(
          image, process, thread,
          &stackframe, &context, NULL,
          SymFunctionTableAccess64, SymGetModuleBase64, NULL);

    if (!result) { break; }

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;

    ostream << "[" << i << "] ";

    if (SymFromAddr(process, stackframe.AddrPC.Offset, &displacement, symbol)) {
      ostream << symbol->Name;
      //printf("[%i] %s\n", i, symbol->Name);
    } else {
      ostream << "???";
      //printf("[%i] ???\n", i);
    }

    ostream << "\n";
  }

  SymCleanup(process);
#elif defined(Q_OS_MAC) || defined(Q_OS_LINUX)
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  backtrace_symbols_fd(array, size, output.handle());
#endif

  output.close();

  QString crash_handler_exe = QDir(qApp->applicationDirPath()).filePath(QStringLiteral("olive-crashhandler"));
  QProcess::startDetached(crash_handler_exe, {log_path});

  exit(1);
}

OLIVE_NAMESPACE_EXIT
