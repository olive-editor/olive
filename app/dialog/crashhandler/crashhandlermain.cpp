#include "crashhandler.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  if (argc < 2) {
    return 1;
  }

  QApplication a(argc, argv);

  CrashHandlerDialog chd(argv[1]);
  chd.open();

  return a.exec();
}
