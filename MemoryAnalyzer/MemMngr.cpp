#include "MemMngr.h"

#include <assert.h>
#include <exception>
#include <iostream>

#ifdef _WIN32
#include <malloc.h>
#endif

using namespace std;


MemoryManager::MemoryManager() 
	: head_new(nullptr), head_new_array(nullptr), showAllAllocs(false), showAllDeallocs(false),
	peakMemory(0), currentMemory(0), unknown("Unknown"), dumpLeaksToFile(false), currentBlocks(0), 
	peakBlocks(0), head_types(nullptr)
{
}

MemoryManager::~MemoryManager()
{
	int totalLeaks = 0;

	auto cleanupLeakCheck = [&](MemInfoNode *head, AllocationType type)
	{
		// step through the list of nodes (which contain info on allocs of a single size) and check if
		// there are any allocations left
		for(MemInfoNode *temp; head; head = temp)
		{
			// if there are stored allocations, it means the user didn't free them, so list them
			// and, if the user wants, output them to the file
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

				// go through all the remaining addresses and display/store file & line #
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
	// this is here to basically clear the file contents
	remove( "memleaks.log" );
	if(dumpLeaksToFile)
	{
		dumpFile.open("memleaks.log", ios::app);
	}

	cleanupLeakCheck(head_new, ALLOC_NEW);
	cleanupLeakCheck(head_new_array, ALLOC_NEW_ARRAY);

	cout << "Total number of leaks found: " << totalLeaks << "\nTotal memory leaked: " << currentMemory 
		<< " bytes (" << currentMemory / 1000. << " kilobytes / " << currentMemory / 1000000. << " megabytes)" 
		<< "\n\nPress any key twice to continue";
	if(dumpLeaksToFile)
	{
		dumpFile << "Total number of leaks found: " << totalLeaks << "\nTotal memory leaked: " << currentMemory 
			<< " bytes (" << currentMemory / 1000. << " kilobytes / " << currentMemory / 1000000. << " megabytes)" 
			<< "\n\nPress any key twice to continue";
	}
	cin.get();
	cin.get();
}

void MemoryManager::AddAllocationToList(size_t size, AllocationType type, void *ptr)
{
	MemInfoNode *head = GetListHead(type);
	auto current = head;
	// move through the list until the correct node is found (based on alloc size)
	while(current && current->size != size)
	{
		current = current->next;
	}

	auto addAddress = [&](void *newAddr, MemInfoNode **curMemNode)
	{
		AddrListNode *newAddrNode = static_cast<AddrListNode*>(malloc(sizeof(AddrListNode)));
		// set the "next" pointer to the address the MemInfoNode is currently pointing to
		newAddrNode->next = (*curMemNode)->addresses;
		newAddrNode->address = newAddr;
		// object type, file, and line # start out as unknown or 0; the information, if available, will be added
		// through the use of the SourcePacket mechanism after the entire allocation is complete
		newAddrNode->type = unknown;
		newAddrNode->file = unknown;
		newAddrNode->line = 0;

		// push the new node in the front of the address list of the mem node (ie, push_front)
		(*curMemNode)->addresses = newAddrNode;
		mostRecentAllocAddrNode = newAddrNode;
	};

	// if we found a mem node with the correct size, just stick a new address node onto the list
	if(current)
	{
		current->numberOfAllocations++;
		addAddress(ptr, &current);
	}
	// otherwise, create a mem node for the new size and then add the address to the list
	else
	{
		MemInfoNode *newMemNode = static_cast<MemInfoNode*>(malloc(sizeof(MemInfoNode)));
		newMemNode->size = size;
		newMemNode->numberOfAllocations = 1;
		// set "next" to be the current head node
		newMemNode->next = GetListHead(type);
		newMemNode->addresses = nullptr;

		addAddress(ptr, &newMemNode);

		// stick the new MemInfoNode at the beginning of the list (ie, push_front)
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
	// get the header address by moving backwards by the size of the header (it's always contiguous)
	AllocationHeader *header = reinterpret_cast<AllocationHeader*>(rawPtr - sizeof(AllocationHeader));
	size_t size = header->rawSize;

	auto current = head;
	// find the mem node whose size matches the pointer size
	while(current && current->size != size)
	{
		current = current->next;
	}
	// make sure there was actually a node with that size
	assert(current);

	auto addressNode = current->addresses;
	AddrListNode *prevAddressNode = nullptr;
	// make sure there was at least one address listed under the mem info node
	assert(addressNode);
	// and then find the matching address node
	while(addressNode && addressNode->address != ptr)
	{
		prevAddressNode = addressNode;
		addressNode = addressNode->next;
	}
	// make sure the address attempting to be freed was actually created
	assert(addressNode);

	// if prevAddressNode is non-null, it means the discovered node wasn't the first one
	// in either case, update the links to remove the address node from the list
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
		cout << "\n\tObject Type: " << addressNode->type << "\n\tFile: " << addressNode->file 
			<< "\n\tLine: " << addressNode->line << "\n\n";
	}
	free(addressNode);
	current->numberOfAllocations--;
}

MemoryManager::AddrListNode* MemoryManager::RetrieveAddrNode(void *ptr, size_t objectSize)
{
	AddrListNode *temp_addr = nullptr;
	auto addressFind = [&](MemInfoNode *head) -> bool
	{
		MemInfoNode *temp_mem = head;
		// if we don't know the size, just iterate through the lists
		if(objectSize == -1)
		{
			while(temp_mem)
			{
				temp_addr = temp_mem->addresses;
				// move down the address list until it's null or the address matches
				while(temp_addr && temp_addr->address != ptr)
				{
					temp_addr = temp_addr->next;
				}
				// if it stopped on a non-null addr node and it matches the address, return it
				if(temp_addr && temp_addr->address == ptr)
				{
					return true;
				}
				// otherwise, move to the next size
				temp_mem = temp_mem->next;
			}
		}
		// but if we do know the size, skip through the memory nodes for the lists until we find the correct size
		else
		{
			for( ; temp_mem && temp_mem->size != objectSize; temp_mem = temp_mem->next);
			if(temp_mem)
			{
				temp_addr = temp_mem->addresses;
				while(temp_addr && temp_addr->address != ptr)
				{
					temp_addr = temp_addr->next;
				}
				if(temp_addr && temp_addr->address == ptr)
				{
					return true;
				}
			}
		}
		return false;
	};

	if(addressFind(head_new))
	{
		return temp_addr;
	}
	else if(addressFind(head_new_array))
	{
		return temp_addr;
	}
	return nullptr;
}

const char* MemoryManager::GetAllocTypeAsString(AllocationType type)
{
	return type == ALLOC_NEW ? "non-array" : "array";
}

MemoryManager::TypeNode* MemoryManager::GetTypeHead()
{
	return head_types;
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

void MemoryManager::AddAllocationDetails(void *ptr, const char *file, int line, const char *type, size_t objectSize)
{
	if(!ptr)
		return;

	if(mostRecentAllocAddrNode->address != ptr)
	{
		mostRecentAllocAddrNode = RetrieveAddrNode(ptr, objectSize);
		// if it returns null, it couldn't be found in the stored lists, possibly indicating some problem
		// during allocation/construction
		if(!mostRecentAllocAddrNode)
		{
			return;
		}
	}

	mostRecentAllocAddrNode->file = file;
	mostRecentAllocAddrNode->line = line;
	mostRecentAllocAddrNode->type = type;
}

void MemoryManager::AddTypeNode(TypeNode *newType)
{
	newType->next = head_types;
	head_types = newType;
}

void* MemoryManager::Allocate(size_t size, AllocationType type, bool throwEx)
{
	// cast necessary since this is C++ (note the additional bytes for the header)
	unsigned char *ptr = static_cast<unsigned char*>(malloc(size + sizeof(AllocationHeader)));
	// if there was a problem getting memory, either throw an exception or nullptr depending on what version of new
	// was used
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

	// stick the pertinent information for the allocation in the header
	AllocationHeader *header = reinterpret_cast<AllocationHeader*>(ptr);
	header->rawSize = size;
	header->type = type;

	// only store the address of the memory we give to the user, not the (header + the mem) address, since they will 
	// release it with that address
	AddAllocationToList(size, type, ptr + sizeof(AllocationHeader));

	// update stats
	currentBlocks++;
	if(currentBlocks > peakBlocks)
	{
		peakBlocks = currentBlocks;
	}
	currentMemory += size;
	if(currentMemory > peakMemory)
	{
		peakMemory = currentMemory;
	}

	if(showAllAllocs)
	{
		cout << "Allocation >\n\tSize: " <<  size << "\n\tAlloc Type: " << GetAllocTypeAsString(type) << "\n\n";
	}
	return ptr + sizeof(AllocationHeader);
}

void MemoryManager::Deallocate(void *ptr, AllocationType type, bool throwEx)
{
	// nothing happens if a nullptr is passed in
	if(ptr)
	{
		unsigned char *rawPtr = static_cast<unsigned char*>(ptr);
		AllocationHeader *header = reinterpret_cast<AllocationHeader*>(rawPtr - sizeof(AllocationHeader));
		if(showAllDeallocs)
		{
			cout << "Deallocation >\n\tSize: " <<  header->rawSize << "\n\tAlloc Type: " 
				<< GetAllocTypeAsString(header->type);
		}
		RemoveAllocationFromList(ptr, type);
		currentMemory -= header->rawSize;
		currentBlocks--;
		// free the header address, since that points to the block originally alloc'd through malloc
		free(header);
	}
}

void MemoryManager::DisplayAllocations(bool displayNumberOfAllocsFirst, bool displayDetail)
{
	int totalAllocsNew = 0, totalAllocsNewArray = 0;
	auto DisplayAllocs = [=](MemInfoNode *head, AllocationType type, int &allocTotal)
	{
		for(MemInfoNode *temp = head; temp; temp = temp->next)
		{
			// check if there are any current allocations for the current block size
			if(temp->numberOfAllocations != 0)
			{
				cout << "\t";
				if(displayNumberOfAllocsFirst)
				{
					cout << temp->numberOfAllocations << "\tallocation(s) of size: " << temp->size;
				}
				else
				{
					cout << "Size: " << temp->size << "\t# of allocations: " << temp->numberOfAllocations;
				}
				allocTotal += temp->numberOfAllocations;
				if(displayDetail)
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

void MemoryManager::DisplayStatTable()
{
	
}

long long MemoryManager::GetCurrentBlocks()
{
	return currentBlocks;
}

size_t MemoryManager::GetCurrentMemory()
{
	return currentMemory;
}

long long MemoryManager::GetPeakBlocks()
{
	return peakBlocks;
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


// Non-array versions

// exception version
void* operator new(size_t size)
{
	return MemoryManager::Get().Allocate(size, ALLOC_NEW, true);
}

// non-exception version
void* operator new(size_t size, const std::nothrow_t&)
{
	return MemoryManager::Get().Allocate(size, ALLOC_NEW);
}

// exception version
void operator delete(void *ptr)
{
	MemoryManager::Get().Deallocate(ptr, ALLOC_NEW, true);
}

// non-exception version
void operator delete(void *ptr, const std::nothrow_t&)
{
	MemoryManager::Get().Deallocate(ptr, ALLOC_NEW);
}


// Array versions

// exception version
void* operator new[](size_t size)
{
	return MemoryManager::Get().Allocate(size, ALLOC_NEW_ARRAY, true);
}

// non-exception version
void* operator new[](size_t size, const std::nothrow_t&)
{
	return MemoryManager::Get().Allocate(size, ALLOC_NEW_ARRAY);
}

// exception version
void operator delete[](void *ptr)
{
	MemoryManager::Get().Deallocate(ptr, ALLOC_NEW_ARRAY, true);
}

// non-exception version
void operator delete[](void *ptr, const std::nothrow_t&)
{
	MemoryManager::Get().Deallocate(ptr, ALLOC_NEW_ARRAY);
}