#ifndef MEMORYANALYZER_H
#define MEMORYANALYZER_H

#ifdef _DEBUG

#include "MemMngr.h"

#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW

#endif

#endif