#ifndef MEMMNGR_H
#define MEMMNGR_H


#include <assert.h>
#include <exception>
#include <iostream>

#ifdef _WIN32
#include <malloc.h>
#endif


enum AllocationType
{
	ALLOC_NEW,
	ALLOC_NEW_ARRAY
};

class MemoryManager
{
private:

	struct AllocationHeader
	{
		size_t rawSize;
		AllocationType type;
	};

	struct AddrListNode
	{
		void *address;
		const char *file;
		int line;
		AddrListNode *next;
	};

	struct MemInfoNode
	{
		size_t size;
		int numberOfAllocations;
		AddrListNode *addresses;
		MemInfoNode *next;
	};

	MemInfoNode *head_new;
	MemInfoNode *head_new_array;

	int currentMemory;
	int peakMemory;

	const char *filenameUnavail;

	MemoryManager();
	~MemoryManager();

	void AddAllocationToList(size_t size, AllocationType type, void *ptr, const char *file, int line);
	void RemoveAllocationFromList(void *ptr, AllocationType type);

	const char* GetAllocTypeAsString(AllocationType type);
	MemInfoNode* GetListHead(AllocationType type);

public:

	bool showAllAllocs;
	bool showAllDeallocs;
	
	static MemoryManager& Get();
	
	void* Allocate(size_t size, AllocationType type, const char *file, int line, bool throwEx = false);
	void Deallocate(void *ptr, AllocationType type, bool throwEx = false);

	void DisplayAllocations(bool sortBySize = true, bool displayList = false);
	int GetCurrentMemory();
	int GetPeakMemory();

#ifdef _WIN32
	void HeapCheck();
#endif
};


void* operator new(size_t size);
void* operator new(size_t size, const std::nothrow_t&);
void* operator new(size_t size, const char *file, int line);
void operator delete(void *ptr);
void operator delete(void *ptr, const std::nothrow_t&);

void* operator new[](size_t size);
void* operator new[](size_t size, const std::nothrow_t&);
void* operator new[](size_t size, const char *file, int line);
void operator delete[](void *ptr);
void operator delete[](void *ptr, const std::nothrow_t&);


#endif