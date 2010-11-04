#include "MemMngr.h"


using namespace std;


MemoryManager::MemoryManager() 
	: head_new(nullptr), head_new_array(nullptr), showAllAllocs(false), showAllDeallocs(false),
	peakMemory(0), currentMemory(0), filenameUnavail("Filename unavailable"), dumpLeaksToFile(false)
{
}

MemoryManager::~MemoryManager()
{
	int totalLeaks = 0;
	auto cleanupLeakCheck = [&](MemInfoNode *head, AllocationType type)
	{
		ofstream dumpFile = ofstream("memleaks.log", ios::app);
		for(MemInfoNode *temp; head; head = temp)
		{
			if(head->numberOfAllocations != 0)
			{
				cout << head->numberOfAllocations << " memory leak(s) detected of size " << head->size 
					<< " and type " << GetAllocTypeAsString(type);
				if(dumpLeaksToFile)
				{
					dumpFile << head->numberOfAllocations << " memory leak(s) detected of size " << head->size 
						<< " and type " << GetAllocTypeAsString(type);
				}
				totalLeaks += head->numberOfAllocations;
				for(AddrListNode *addrNode = head->addresses, *temp; addrNode; )
				{
					cout << "\n\tAddress: 0x" << addrNode->address << " File: " << addrNode->file 
						<< " Line: " << addrNode->line;
					if(dumpLeaksToFile)
					{
						dumpFile << "\n\tAddress: 0x" << addrNode->address << " File: " << addrNode->file 
							<< " Line: " << addrNode->line;
					}
					temp = addrNode;
					addrNode = addrNode->next;
					free(temp);
				}
				cout << "\n\n";
				if(dumpLeaksToFile)
				{
					dumpFile << "\n\n";
				}
			}

			temp = head->next;
			free(head);
		}
	};
	remove( "memleaks.log" );
	cleanupLeakCheck(head_new, ALLOC_NEW);
	cleanupLeakCheck(head_new_array, ALLOC_NEW_ARRAY);
	cout << "Total number of leaks found: " << totalLeaks << "\nTotal memory leaked: " << currentMemory 
		<< " bytes (" << currentMemory / 1000. << " kilobytes / " << currentMemory / 1000000. << " megabytes)" 
		<< "\n\nPress any key to continue";
	cin.get();
}

void MemoryManager::AddAllocationToList(size_t size, AllocationType type, void *ptr, const char *file, int line)
{
	MemInfoNode *head = GetListHead(type);
	auto current = head;
	while(current && current->size != size)
	{
		current = current->next;
	}

	auto addAddress = [=](void *newAddr, MemInfoNode **curMemNode)
	{
		AddrListNode *newAddrNode = static_cast<AddrListNode*>(malloc(sizeof(AddrListNode)));
		newAddrNode->next = (*curMemNode)->addresses;
		newAddrNode->address = newAddr;
		if(strlen(file) == 0)
		{
			newAddrNode->file = filenameUnavail;
			newAddrNode->line = 0;
		}
		else
		{
			newAddrNode->file = file;
			newAddrNode->line = line;
		}
		(*curMemNode)->addresses = newAddrNode;
	};

	if(current)
	{
		current->numberOfAllocations++;
		addAddress(ptr, &current);
	}
	else
	{
		MemInfoNode *newMemNode = static_cast<MemInfoNode*>(malloc(sizeof(MemInfoNode)));
		newMemNode->size = size;
		newMemNode->numberOfAllocations = 1;
		newMemNode->next = GetListHead(type);
		newMemNode->addresses = nullptr;
		addAddress(ptr, &newMemNode);
		if(type == ALLOC_NEW)
		{
			head_new = newMemNode;
		}
		else
		{
			head_new_array = newMemNode;
		}
	}
}

void MemoryManager::RemoveAllocationFromList(void *ptr, AllocationType type)
{
	MemInfoNode *head = GetListHead(type);
	unsigned char *rawPtr = static_cast<unsigned char*>(ptr);
	AllocationHeader *header = reinterpret_cast<AllocationHeader*>(rawPtr - sizeof(AllocationHeader));
	size_t size = header->rawSize;
	auto current = head;
	while(current && current->size != size)
	{
		current = current->next;
	}
	assert(current);

	auto addressNode = current->addresses;
	AddrListNode *prevAddressNode = nullptr;
	assert(addressNode);
	while(addressNode && addressNode->address != ptr)
	{
		prevAddressNode = addressNode;
		addressNode = addressNode->next;
	}
	assert(addressNode);
	if(prevAddressNode)
	{
		prevAddressNode->next = addressNode->next;
	}
	else
	{
		current->addresses = addressNode->next;
	}
	if(showAllDeallocs)
	{
		cout << "\n\tFile: " << addressNode->file << "\n\tLine: " << addressNode->line << "\n\n";
	}
	free(addressNode);
	current->numberOfAllocations--;
}

const char* MemoryManager::GetAllocTypeAsString(AllocationType type)
{
	return type == ALLOC_NEW ? "non-array" : "array";
}

MemoryManager::MemInfoNode* MemoryManager::GetListHead(AllocationType type)
{
	return type == ALLOC_NEW ? head_new : head_new_array;
}

MemoryManager& MemoryManager::Get()
{
	static MemoryManager mmgr;
	return mmgr;
}

void* MemoryManager::Allocate(size_t size, AllocationType type, const char *file, int line, bool throwEx)
{
	unsigned char *ptr = static_cast<unsigned char*>(malloc(size + sizeof(AllocationHeader)));
	if(!ptr)
	{
		if(throwEx)
		{
			throw std::bad_alloc("Failed to acquire memory.\n");
		}
		else
		{
			return nullptr;
		}
	}
	AllocationHeader *header = reinterpret_cast<AllocationHeader*>(ptr);
	header->rawSize = size;
	header->type = type;
	AddAllocationToList(size, type, ptr + sizeof(AllocationHeader), file, line);
	currentMemory += size;
	if(currentMemory > peakMemory)
	{
		peakMemory = currentMemory;
	}
	if(showAllAllocs)
	{
		cout << "Allocation >\n\tSize: " <<  size << "\n\tType: " << GetAllocTypeAsString(type) 
			<< "\n\tFile: " << ((strlen(file) > 0) ? file : filenameUnavail) << "\n\tLine: " << line << "\n\n";
	}
	return ptr + sizeof(AllocationHeader);
}

void MemoryManager::Deallocate(void *ptr, AllocationType type, bool throwEx)
{
	if(ptr)
	{
		unsigned char *rawPtr = static_cast<unsigned char*>(ptr);
		AllocationHeader *header = reinterpret_cast<AllocationHeader*>(rawPtr - sizeof(AllocationHeader));
		if(showAllDeallocs)
		{
			cout << "Deallocation >\n\tSize: " <<  header->rawSize << "\n\tType: " 
				<< GetAllocTypeAsString(header->type);
		}
		RemoveAllocationFromList(ptr, type);
		currentMemory -= header->rawSize;
		free(header);
	}
}

void MemoryManager::DisplayAllocations(bool sortBySize, bool displayList)
{
	int totalAllocsNew = 0, totalAllocsNewArray = 0;
	auto DisplayAllocs = [=](MemInfoNode *head, AllocationType type, int &allocTotal)
	{
		for(MemInfoNode *temp = head; temp; temp = temp->next)
		{
			if(temp->numberOfAllocations != 0)
			{
				cout << "\t";
				if(sortBySize)
				{
					cout << temp->numberOfAllocations << "\tallocation(s) of size: " << temp->size;
				}
				else
				{
					cout << "Size: " << temp->size << "\t# of allocations: " << temp->numberOfAllocations;
				}
				allocTotal += temp->numberOfAllocations;
				if(displayList)
				{
					for(auto addrNode = temp->addresses; addrNode; addrNode = addrNode->next)
					{
						cout << "\n\tAddress: 0x" << addrNode->address << "  File: " << addrNode->file 
							<< "  Line: " << addrNode->line;
					}
				}
				cout << "\n";
			}
		}
	};

	cout << "<<Non-array allocations>>\n";
	DisplayAllocs(head_new, ALLOC_NEW, totalAllocsNew);
	cout << "<<Array allocations>>\n";
	DisplayAllocs(head_new_array, ALLOC_NEW_ARRAY, totalAllocsNewArray);
	cout << "Total allocations: " << totalAllocsNew + totalAllocsNewArray << " (" << totalAllocsNew 
		<< " non-array, " << totalAllocsNewArray << " array)\n\n";
}

size_t MemoryManager::GetCurrentMemory()
{
	return currentMemory;
}

size_t MemoryManager::GetPeakMemory()
{
	return peakMemory;
}

#ifdef _WIN32
void MemoryManager::HeapCheck()
{
	switch(_heapchk())
	{
	case _HEAPOK:
		cout << "OK - heap is fine.\n";
		break;
	case _HEAPEMPTY:
		cout << "OK - heap is empty.\n";
		break;
	case _HEAPBADBEGIN:
		cout << "ERROR - bad start of heap.\n";
		break;
	case _HEAPBADNODE:
		cout << "ERROR - bad node in heap.\n";
		break;
	}
}
#endif

void* operator new(size_t size) //throws
{
	return MemoryManager::Get().Allocate(size, ALLOC_NEW, "\0", 0, true);
}

void* operator new(size_t size, const std::nothrow_t&) // no throw
{
	return ::operator new(size, "\0", 0);
}

void* operator new(size_t size, const char *file, int line)
{
	return MemoryManager::Get().Allocate(size, ALLOC_NEW, file, line);
}

void operator delete(void *ptr)
{
	MemoryManager::Get().Deallocate(ptr, ALLOC_NEW);
}

void operator delete(void *ptr, const std::nothrow_t&) // no throw
{
	::operator delete(ptr);
}



void* operator new[](size_t size) //throws
{
	return MemoryManager::Get().Allocate(size, ALLOC_NEW_ARRAY, "\0", 0, true);
}

void* operator new[](size_t size, const std::nothrow_t&) //no throw
{
	return ::operator new[](size, "\0", 0);
}

void* operator new[](size_t size, const char *file, int line)
{
	return MemoryManager::Get().Allocate(size, ALLOC_NEW_ARRAY, file, line);
}

void operator delete[](void *ptr)
{
	MemoryManager::Get().Deallocate(ptr, ALLOC_NEW_ARRAY);
}

void operator delete[](void *ptr, const std::nothrow_t&) //no throw
{
	::operator delete[](ptr);
}