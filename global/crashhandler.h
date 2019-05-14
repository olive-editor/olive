#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

#ifdef __GNUC__
#include <signal.h>
void handler(int sig);
#endif

#endif // CRASHHANDLER_H
