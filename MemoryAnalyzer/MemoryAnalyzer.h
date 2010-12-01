/** @file MemoryAnalyzer.h
@brief Include this header file at the very beginning of your program to use MemoryAnalyzer.
*/

/** @mainpage
@section desc Description

MemoryAnalyzer is a very simple, portable memory information tool for C++ projects.  It was written for educational
purposes, for determining the correct memory scheme to use when developing video games (for example, to help
the user determine whether using a pool would be worthwhile), and for detecting leaks.  It was written with
single-threaded applications in	mind (although you are free to extend it to multi-threaded apps) and has very
few dependencies, all of which are part of the standard library.  MemoryAnalyzer was also designed to be simple
to understand and use (both installation and usage).

Note that since it was written with portability in mind, it does not have all the features of memory tools written
specifically for your platform.  It is also not intended to replace the more sophisticated tools out there (such as
Valgrind), but to serve as an easy-to-use, portable tool which you can use to check for leaks and get an overview
of your program's memory-related behavior.

@section inst Installation

Installation is very simple--just copy the header and source files to your project directory and include "MemoryAnalyzer.h" 
at the very beginning of your program (before any other includes).

@section use Usage

The name of the singleton object is memAnalyzer and is available to you in order to change certain aspects of its behavior.
Note that MemoryAnalyzer will only be in effect during debug builds.  Once you switch to your Release build, memory
allocation and deallocation will return to normal, with no effort on your part.  Also, note that, by default, exceptions
are not used (although exception versions of allocation/deallocation functions are present).

@subsection leaks Memory Leaks

If you just want to check for leaks, you don't have to do anything else--a leak report will show up in the console
when your program exits.  If you also want to dump leaks to a file when your program exits, set dumpLeaksToFile to true.

Example: memAnalyzer->dumpLeaksToFile = true;

@subsection allocInfo Alloc/Dealloc Information

Although it can create a (very) large amount of spam in the console if left on all the time, sometimes it may be useful
to see allocations and deallocations as they happen.  To turn on diagnostic information for allocations, set showAllAllocs
to true; likewise, to turn on diagnostic information for deallocations, set showAllDeallocs to true.

Example: memAnalyzer->showAllAllocs = true;

You may wish to get a summarized list of all the current allocations; for example, you may want to see if you
are making a lot of small allocations and relatively few large allocations, in order to determine if your memory scheme
could be improved.  To get a list of the current allocations, call DisplayAllocations().  Check the documentation for an
explanation of the parameters.

Example: memAnalyzer->DisplayAllocations();

If you're working within strict memory limits, keeping an eye on how much memory you are using is important.  To see how
much memory you currently have allocated, call GetCurrentMemory().  To see the peak amount of memory you had allocated
during your program's run, call GetPeakMemory().

Example: cout << memAnalyzer->GetCurrentMemory();

@subsection heap Heap Checking

Heap corruption is a very serious problem.  If you are on a Windows system, you can call HeapCheck() to determine the state
of the heap.  Although calling the function could shuffle things around in memory and cause the problem to move somewhere
else, it can often be helpful in narrowing down the problem region.

Example: memAnalyzer->HeapCheck();
*/

#ifndef MEMORYANALYZER_H
#define MEMORYANALYZER_H


#ifdef _DEBUG

#include "MemoryTracer.h"

#ifndef DISABLE_DEBUG_INFO_COLLECTION

/** @def DEBUG_NEW
Enables automatic inclusion of source filename, line number, and object type
*/
#define DEBUG_NEW SourcePacket(__FILE__, __LINE__) * new
#define new DEBUG_NEW

#endif

/**
Singleton global variable for memory analyzer
*/
extern MemoryTracer *memAnalyzer;

#endif

#endif