/** @file MemoryTracer.h
@brief Internal implementation -- do not include this file directly.  Please include MemoryAnalyzer.h instead.
*/

#ifndef MEMORYTRACER_H
#define MEMORYTRACER_H


#include <assert.h>
#include <fstream>
#include <stdlib.h>
#include <typeinfo>


/** @enum AllocationType
This is used to differentiate between memory allocated through either new or new[]
*/
enum AllocationType
{
	ALLOC_NEW,			/**< Normal memory allocation */
	ALLOC_NEW_ARRAY		/**< Array memory allocation */
};

/** @class SourcePacket
@brief Temporary container class for macro-acquired file and line information.
*/
class SourcePacket
{
private:

	SourcePacket(const SourcePacket&);
	SourcePacket& operator=(const SourcePacket&);

public:

	/** @param file Source filename
		@param line Line number
	*/
	SourcePacket(const char *file, int line) : file(file), line(line)
	{}
	~SourcePacket()
	{}

	//! Source file in which the allocation was made
	const char *file;
	//! Line number of the source file in which the allocation was made
	int line;
};


template<typename T>
T* operator*(const SourcePacket& packet, T* p);


/** @class MemoryTracer
@brief Internal implementation of the MemoryAnalyzer tool.

This class intercepts and handles all allocations and deallocations. It can	display info such as peak memory, 
number of allocations, and address lists for allocations, to name a few pieces of information it stores. It is 
implemented as a singleton, has very few dependencies (all of which are standard), and is portable.
*/
class MemoryTracer
{
private:

	/** @struct AllocationHeader
	Information object placed directly before all memory upon allocation.
	*/
	struct AllocationHeader
	{
		//! Size of the the object in memory (not including the header)
		size_t rawSize;
		AllocationType type;
	};

	/** @struct AddrListNode
	Internal information container. Used to keep track of current memory allocations.
	*/
	struct AddrListNode
	{
		//! Allocation address
		void *address;
		//! Object type
		const char *type;
		//! Source file
		const char *file;
		//! Line number
		int line;
		AddrListNode *next;
	};

	/** @struct MemInfoNode
	Internal information container. Used to help organize individual memory information objects (AddrListNode's).
	*/
	struct MemInfoNode
	{
		//! Size of the allocations referenced within
		size_t size;
		//! Number of objects allocated with this object's size
		int numberOfAllocations;
		//! Linked list of current memory allocations with this object's size
		AddrListNode *addresses;
		MemInfoNode *next;
	};

	/** @struct TypeNode
	Internal information container. Used for memory summary purposes. Only tracks allocations which are caught and detailed
	by the memory manager.
	*/
	struct TypeNode
	{
		const char *type;
		long blocks;
		size_t memSize;
		TypeNode *next;
	};

	//! Linked list of normal allocations & information
	MemInfoNode *head_new;
	//! Linked list of array allocations & information
	MemInfoNode *head_new_array;
	//! Linked list of types (types, blocks, total size in memory)
	TypeNode *head_types;
	//! Pointer to last allocated block of memory.  The detailing function checks this variable first to save time.  If it
	//! the correct node, it will use it; otherwise, it will search for the node using RetrieveAddrNode.
	AddrListNode *mostRecentAllocAddrNode;
	
	size_t currentMemory;
	size_t peakMemory;
	long long currentBlocks;
	long long peakBlocks;
	const char *unknown;

	std::ofstream dumpFile;

	MemoryTracer();
	~MemoryTracer();
	MemoryTracer(const MemoryTracer&);
	MemoryTracer& operator=(const MemoryTracer&);

	/**	@brief Adds information for a single allocation to the internal list
	@param size Size of the allocation requested by the program
	@param type Allocation type
	@param ptr Pointer to the allocated memory which needs to be added to the internal list
	@param file Source filename from which the allocation was requested
	@param line Line number of the source file on which the allocation request occurred
	*/
	void AddAllocationToList(size_t size, AllocationType type, void *ptr);

	/** @brief Adds context information to the most recent allocation
	@param ptr Pointer to the allocated memory
	@param file Source filename from which the allocation was requested
	@param line Line number of the source file on which the allocation request occurred
	@param type Type of the allocation (i.e., int, Complex, Vector, etc.) determined using RTTI
	@param objectSize Size of the object; used to decrease lookup times if the node matching the ptr address is not the same
	as the one in mostRecentAllocAddrNode
	*/
	void AddAllocationDetails(void *ptr, const char *file, int line, const char *type, size_t objectSize);

	/** @brief Updates stats inn type information list
		@param type Object type name
		@param size Object's size in memory
	*/
	void AddToTypeList(const char *type, size_t size);

	/** @brief Allocates memory upon request from the overloaded new operator
	@param size Requested allocation size
	@param type Allocation type
	@param file Source filename from which the allocation was requested
	@param line Line number of the source file on which the allocation request occurred
	@param throwEx Indicates whether or not an exception should be thrown if memory couldn't be allocated (default: false)
	@return Pointer to allocated memory
	*/
	void* Allocate(size_t size, AllocationType type, bool throwEx = false);

	/** @brief Frees memory upon request from the overloaded delete operator
	@param ptr Pointer to memory which should be freed
	@param type Allocation type
	@param throwEx Indicates whether or not an exception should be thrown if memory couldn't be allocated (default: false)
	*/
	void Deallocate(void *ptr, AllocationType type, bool throwEx = false);

	/**	@brief Returns string version of allocation type enum
	@param type Allocation type to convert to a string
	@return Allocation type in string form
	*/
	const char* GetAllocTypeAsString(AllocationType type);

	/**	@brief Returns the head of the linked list for a desired allocation type
	@param type Allocation type of head of internal list (i.e., ALLOC_NEW for non-array allocation list, etc.)
	@return Pointer to head of allocation info list for desired type
	*/
	MemInfoNode* GetListHead(AllocationType type);

	/**	@brief Removes information for a single allocation from the internal list
	@param ptr Pointer to the freed memory which needs to be deleted from the internal list
	@param type Allocation type of ptr
	*/
	void RemoveAllocationFromList(void *ptr, AllocationType type);

	/** @brief Performs a stat update in the type list after a deallocation
		@param type String of the block's typename
		@param size Object's size in memory
	*/
	void RemoveFromTypeList(const char *type, size_t size);

	/** @brief Finds the information node associated with the address.
		@param ptr Address of the desired node
		@param objectSize Size of the object pointed to by ptr.  If the size is not known, this parameter can be ignored.
		@return Pointer to the AddrListNode containing information about the allocation
	*/
	AddrListNode* RetrieveAddrNode(void *ptr, size_t objectSize = -1);

	/** @brief Finds the size of the object pointed to
		@param ptr Address of the object
		@return Size of the object in bytes
	*/
	size_t RetrieveAddrSize(void *ptr);
	
public:

	/** Set to true to display information about every allocation in the console (default: false). Warning: Depending on
	program size/number of allocations, this may create a lot of messages.
	*/
	bool showAllAllocs;
	/** Set to true to display information about every deallocation in the console (default: false). Warning: Depending on
	program size/number of allocations, this may create a lot of messages.
	*/
	bool showAllDeallocs;
	/** Set to true to save all memory leaks and related information in a generated file, memleaks.log (default: false).
	*/
	bool dumpLeaksToFile;
	
	/** @brief Displays current memory allocations in the console according to criteria
	@param displayNumberOfAllocsFirst Set to true to display the list according to the number of allocations
	of a certain size (i.e., 10 allocs of size 2); otherwise, the list will be displayed according to
	the size of the allocations, followed by the number of allocations of that size (i.e., Size: 2, 10 allocs).
	(default: true)
	@param displayDetail Set to true to display extra detail about every allocation (address, file, line).
	Warning: Depending on program size/number of allocations, this may create a lot of messages. (default: false)
	*/
	void DisplayAllocations(bool displayNumberOfAllocsFirst = true, bool displayDetail = false);

	/** @brief Displays table with allocated object types, the number of times each type appears (i.e., # of blocks), and the
	percentage of total memory each collection of type <T> objects takes up.  Objects of unknown types are grouped by size
	and are indicated like so: Unknown type (size: 16 bytes).
	*/
	void DisplayStatTable();

	/** @brief Singleton access
	@return Reference to singleton object
	*/
	static MemoryTracer& Get();

	/** @brief Retrieves the number of blocks allocated at present
		@return Number of allocated blocks
	*/
	long long GetCurrentBlocks();

	/** @brief Retrieves the current allocated memory
	@return Currently allocated memory in bytes
	*/
	size_t GetCurrentMemory();

	/** @brief Retrieves the peak number of allocated blocks
		@return Largest number of blocks allocated at any time during program execution
	*/
	long long GetPeakBlocks();

	/** @brief Retrieves the peak allocated memory
	@return Largest amount of memory allocated at any time during program execution in bytes
	*/
	size_t GetPeakMemory();	

#ifdef _WIN32
	/** @brief Calls Windows-specific function to check the state of the heap and display a message in the console 
	indicating said state
	*/
	void HeapCheck();
#endif

	// These declarations make the new and delete operators friends to provide access to allocation and deallocation
	// routines (they are private to prevent users from arbitrarily calling them).
	
	/** @brief Non-array operator new.  Exception version.
		@param size Allocation size
		@return Void pointer to newly allocated memory
	*/
	friend void* operator new(size_t size);
	
	/** @brief Non-array operator new.
		@param size Allocation size
		@return Void pointer to newly allocated memory
	*/
	friend void* operator new(size_t size, const std::nothrow_t&);
	
	/** @brief Non-array operator delete.  Exception version.
		@param ptr Pointer to object to be deleted
	*/
	friend void operator delete(void *ptr);
	
	/** @brief Non-array operator delete
		@param ptr Pointer to object to be deleted
	*/
	friend void operator delete(void *ptr, const std::nothrow_t&);


	/** @brief Array operator new.  Exception version.
		@param size Allocation size
		@return Void pointer to newly allocated memory
	*/
	friend void* operator new[](size_t size);
	
	/** @brief Array operator new.
		@param size Allocation size
		@return Void pointer to newly allocated memory
	*/
	friend void* operator new[](size_t size, const std::nothrow_t&);
	
	/** @brief Array operator delete.  Exception version.
		@param ptr Pointer to object to be deleted
	*/
	friend void operator delete[](void *ptr);
	
	/** @brief Array operator delete
		@param ptr Pointer to object to be deleted
	*/
	friend void operator delete[](void *ptr, const std::nothrow_t&);
	
	template<typename T>
	friend T* operator*(const SourcePacket& packet, T* p);
};

/** @brief Tags allocations with filenames, lines, and types
	@param packet Container object containing filename and line.  This parameter is automatically filled in via a macro.
	@param p Newly created object
	@return Newly created object	
*/
template<typename T>
T* operator*(const SourcePacket& packet, T* p)
{
	if(p)
	{
		const char *type = nullptr;
		try
		{
			 type = typeid(*p).name();
		}
		// bad RTTI
		catch(std::bad_typeid& e)
		{
			cout << e.what() << "\n";
			return p;
		}

		MemoryTracer::Get().AddAllocationDetails(p, packet.file, packet.line, type, sizeof(*p));
		
		if(MemoryTracer::Get().showAllAllocs)
		{
			cout << "Allocation Information Trace >\n\tObject Type: " << type << "\n\tFile: " << packet.file 
				<< "\n\tLine: " << packet.line << "\n\n";
		}

		size_t objectSize = MemoryTracer::Get().RetrieveAddrSize(p);
		assert(objectSize != -1);
		// we need to send the size in case the ptr is pointing to an array, in which case sizeof(*p) would be wrong
		MemoryTracer::Get().AddToTypeList(type, objectSize);
	}
	return p;
}

#endif