#ifndef __K_PRINTF__
#define __K_PRINTF__

#include <stdarg.h>
#include "debug.h"
#define putchar DebugPutch

int printf(const char *format, ...);

#define UartPrintf  printf
#endif

