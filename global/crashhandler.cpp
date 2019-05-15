#include "crashhandler.h"

#include <QStringList>
#include <iostream>

#include "dialogs/crashdialog.h"

#ifdef __GNUC__
#ifdef Q_OS_WIN
#include <windows.h>
#include <DbgHelp.h>
#elif defined(Q_OS_LINUX)
#include <execinfo.h>
#endif
#include <unistd.h>
#endif

#ifdef __GNUC__
void handler(int sig) {
  QStringList strings;

#ifdef Q_OS_WIN
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

  int counter = 0;
  QString line_template = "[%1] %2";
  while (StackWalk64(
           image, process, thread,
           &stackframe, &context, NULL,
           SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;


    QString sym_name;
    if (SymFromAddr(process, stackframe.AddrPC.Offset, &displacement, symbol)) {
      sym_name = symbol->Name;
    } else {
      sym_name = "???";
    }
    QString s = line_template.arg(QString::number(counter), sym_name);
    strings.append(s);
    std::cout << s.data() << std::endl << std::flush;

    counter++;
  }

  SymCleanup(process);
#elif defined(Q_OS_LINUX)
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Signal: %d\n\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);

  // try to show a GUI crash report
  char** bt_syms = backtrace_symbols(array, size);
  for (int i=0;i<size;i++) {
    strings.append(bt_syms[i]);
  }
  free(bt_syms);

#endif

  olive::crash_dialog->SetData(sig, strings);
  olive::crash_dialog->exec();

  abort();
}
#endif
