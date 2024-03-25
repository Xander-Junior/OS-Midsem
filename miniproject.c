#include <stdio.h>
#include <stdlib.h> // For rand()
#include <time.h>   // For time()
#include <string.h> // For strcmp




/* Page size definition, representing the size of a single page in bytes. */
#define PAGE_SIZE 256  // Each page is 256 bytes

/* Total virtual memory size, calculated based on the total number of pages and page size. */
#define TOTAL_VIRTUAL_MEMORY (TOTAL_PAGES * PAGE_SIZE)  // Total virtual memory size

/* Total physical memory size, calculated based on the total number of frames and page size. */
#define TOTAL_PHYSICAL_MEMORY (TOTAL_FRAMES * PAGE_SIZE)  // Total physical memory size


/* Global counters for tracking total access attempts and successful accesses across all processes. */
int globalTotalAccessAttempts = 0;
int globalSuccessfulAccesses = 0;


/* Global counters for tracking page faults, pages fetched from secondary storage, and frame evictions. */
int pageFaults = 0; // To track the number of page faults
int pageFetches = 0; // To track the number of pages fetched from secondary storage
int frameEvictions = 0; // To track the number of frame evictions


/*
 * Struct: PageTableEntry
 * Description: Represents an entry in a page table with a validity indicator,
 * frame number, and the associated process ID.
 */
typedef struct {
    unsigned int valid:1;    // Valid bit: 1 if page is in physical memory, 0 otherwise - to prevent page faults
    unsigned int frameNumber:4; // Frame number: Assuming a max of 16 frames, 4 bits needed
    int processId; // Add process ID to track ownership
} PageTableEntry;

/* Array to represent the page table, with each entry corresponding to a page in virtual memory. */
#define TOTAL_PAGES 256 
PageTableEntry pageTable[TOTAL_PAGES];

/*
 * Struct: Frame
 * Description: Represents a frame in physical memory with an in-use status
 * and the process ID to which the frame is allocated.
 */
#define TOTAL_FRAMES 16
typedef struct {
    unsigned int used:1; // Used bit: 1 if frame is allocated, 0 if free
    unsigned int processId : 5; // Assuming a maximum of 32 processes, 5 bits needed for process ID
} Frame;
Frame physicalMemory[TOTAL_FRAMES];

#define MAX_PROCESSES 10 // Assuming a max of 10 processes for simplicity

/*
 * Struct: MasterPageTableEntry
 * Description: Represents a master page table entry for a process, including
 * its page table, activity status, allocated addresses, and access statistics.
 */
typedef struct {
    PageTableEntry* pageTable; // Existing page table pointer
    int isActive; // Existing activity flag
    unsigned int* allocatedAddresses; // Array to track allocated addresses
    int allocatedCount; // Number of allocated addresses
    int pageFaults; // Existing page fault counter
    int totalAccessAttempts; // Total attempts to access memory
    int successfulAccesses; // Successful memory accesses without a page fault
} MasterPageTableEntry;

// Array to represent the master page table, with each entry corresponding to a process.
MasterPageTableEntry masterPageTable[MAX_PROCESSES];


/*
 * Function: isAddressAllocatedToProcess
 * Parameters: int processId, unsigned int address
 * Returns: int (boolean)
 * Description: Checks if a given virtual address is already allocated to a specified process.
 */
int isAddressAllocatedToProcess(int processId, unsigned int address) {
    if (processId < 0 || processId >= MAX_PROCESSES) {
        return 0; // Invalid process ID
    }

    MasterPageTableEntry* processEntry = &masterPageTable[processId];
    for (int i = 0; i < processEntry->allocatedCount; i++) {
        if (processEntry->allocatedAddresses[i] == address) {
	    printf("Address is allocated to the process");
            return 1; // Address is allocated to the process
        }
    }

    return 0; // Address not found in the process's allocated addresses
}


// Global variable to track frame usage in the queue.
int frameInUse[TOTAL_FRAMES] = {0}; // 0: not in use, 1: in use.

/*
 * Function: addAllocatedAddress
 * Parameters: MasterPageTableEntry* entry, unsigned int address
 * Description: Dynamically adds an allocated virtual address to a process's tracking array.
 */
void addAllocatedAddress(MasterPageTableEntry* entry, unsigned int address) {
    entry->allocatedAddresses = realloc(entry->allocatedAddresses, (entry->allocatedCount + 1) * sizeof(unsigned int));
    if (entry->allocatedAddresses != NULL) {
        entry->allocatedAddresses[entry->allocatedCount] = address;
        entry->allocatedCount++;
    }
}


/*
 * Function: removeAllocatedAddress
 * Parameters: MasterPageTableEntry* entry, unsigned int address
 * Description: Removes an allocated virtual address from a process's tracking array.
 */
void removeAllocatedAddress(MasterPageTableEntry* entry, unsigned int address) {
    for (int i = 0; i < entry->allocatedCount; i++) {
        if (entry->allocatedAddresses[i] == address) {
            // Shift the rest of the addresses down
            for (int j = i; j < entry->allocatedCount - 1; j++) {
                entry->allocatedAddresses[j] = entry->allocatedAddresses[j + 1];
            }
            entry->allocatedCount--;
            entry->allocatedAddresses = realloc(entry->allocatedAddresses, entry->allocatedCount * sizeof(unsigned int));
            break;
        }
    }
}


/*
 * Function: initializeProcessPageTable
 * Returns: int (process ID)
 * Description: Dynamically assigns a process ID and initializes its page table.
 */
int initializeProcessPageTable() {
    for (int processId = 0; processId < MAX_PROCESSES; processId++) {
        if (!masterPageTable[processId].isActive) {
            // Allocate memory for the process's page table
            PageTableEntry* newPageTable = (PageTableEntry*)malloc(TOTAL_PAGES * sizeof(PageTableEntry));
            printf("Page table for process %d allocated at %p.\n", processId, (void*)newPageTable);

            if (newPageTable == NULL) {
                printf("Failed to allocate memory for page table.\n");
                continue;  // Try next processId or handle error appropriately
            }

            // Initialize the new page table entries
            for (int i = 0; i < TOTAL_PAGES; i++) {
                newPageTable[i].valid = 0;  // Mark all pages as invalid initially
                newPageTable[i].frameNumber = 0;  // Frame number is 0 (or any other default value)
            }

            // Assign the new page table to the process and mark as active
            masterPageTable[processId].pageTable = newPageTable;
            masterPageTable[processId].isActive = 1;
            
            return processId;  // Return the assigned process ID
        }
    }

    return -1;  // Indicate failure to find an available process ID
}


// When creating or loading a process, it would be assigned a page table and recorded in the master page table:
void initializeMasterPageTable() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        masterPageTable[i].pageTable = NULL; // Initially, no page table assigned
        masterPageTable[i].isActive = 0; // Process is not active
        masterPageTable[i].allocatedAddresses = NULL;
        masterPageTable[i].allocatedCount = 0;
    }
}

void assignPageTableToProcess(int processId, PageTableEntry* pageTable) {
    if (processId < MAX_PROCESSES) {
        masterPageTable[processId].pageTable = pageTable;
        masterPageTable[processId].isActive = 1; // Mark the process as active
    }
}



/*
 * Function: fetchPageFromSecondaryStorage
 * Parameters: unsigned int pageNumber
 * Description: Simulates fetching a page from secondary storage into a frame.
 */
void fetchPageFromSecondaryStorage(unsigned int pageNumber) {
	printf("Fetching page %u from secondary storage.\n", pageNumber);
	pageFetches++;
}


#define QUEUE_SIZE TOTAL_FRAMES // Assuming one queue entry per frame for simplicity

// Global variables to implement the FIFO queue
int fifoQueue[QUEUE_SIZE];
int front = -1, rear = -1;


/*
 * Function: enqueue
 * Parameters: int frameNumber
 * Description: Enqueues a frame number into the FIFO queue for frame management.
 */
void enqueue(int frameNumber) {
    frameInUse[frameNumber] = 1;
    if ((rear + 1) % QUEUE_SIZE == front) {
        // Queue is full, which should theoretically never happen if managed correctly
        printf("Queue is Full. This should not occur if frames are managed correctly.\n");
        return;
    }
    if (front == -1) { // If the queue is empty
        front = 0;
    }
    rear = (rear + 1) % QUEUE_SIZE; // Circular queue increment
    fifoQueue[rear] = frameNumber; // Place the frame number in the queue

    // Log or perform additional actions if necessary
    printf("Frame %d enqueued.\n", frameNumber);
}

/*
 * Function: dequeue
 * Returns: int (frame number)
 * Description: Dequeues a frame number from the FIFO queue for eviction.
 */
int dequeue() {
    while (front != -1) {
        int frameNumber = fifoQueue[front];
        if (frameInUse[frameNumber] == 0) {
            // Skip over this frame as it's no longer in use.
            front = (front + 1) % QUEUE_SIZE;
            if (front == rear) front = rear = -1; // Queue becomes empty.
        } else {
            // Dequeue logic for an in-use frame.
            return frameNumber; // This frame is actively in use and can be evicted.
        }
    }
    printf("Queue is Empty. No frames to evict.\n");
    return -1;
}


/*
 * Function: findFreeFrameOrEvict
 * Returns: int (frame number)
 * Description: Finds a free frame for allocation or selects a frame for eviction if necessary.
 */
int findFreeFrameOrEvict() {
    // Try to find a free frame
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (!physicalMemory[i].used) {
            printf("Found free frame: %d, allocating to process.\n", i);
            physicalMemory[i].used = 1; // Mark this frame as used
            enqueue(i); // Enqueue the newly used frame
            return i;
        }
    }

    // If no free frame is found, evict the oldest frame
    int frameToEvict = dequeue();
    if (frameToEvict != -1) {
        // Invalidate the page using this frame in both global and process-specific page tables
        for (unsigned int pageNumber = 0; pageNumber < TOTAL_PAGES; pageNumber++) {
            if (pageTable[pageNumber].valid && pageTable[pageNumber].frameNumber == frameToEvict) {
                pageTable[pageNumber].valid = 0;
                int pid = pageTable[pageNumber].processId;
                if (pid >= 0 && pid < MAX_PROCESSES) {
                    masterPageTable[pid].pageTable[pageNumber].valid = 0;
                }
                printf("Evicting frame %d, previously used by page %d of process %d.\n", frameToEvict, pageNumber, pid);
                physicalMemory[frameToEvict].used = 0; // IMPORTANT: Mark the frame as free
                frameInUse[frameToEvict] = 0; // Correctly update the frameInUse status
                break; // Assuming one page per frame
            }
        }
        frameEvictions++; // Increment the frame eviction counter
        return frameToEvict;
    } else {
        printf("Error: No frames available to allocate or evict. Queue might be improperly managed.\n");
        return -1;
    }
}



/*
 * Function: handlePageFault
 * Parameters: unsigned int pageNumber, unsigned int processId
 * Description: Handles a page fault by allocating a frame to the requested page.
 */
void handlePageFault(unsigned int pageNumber, unsigned int processId) {
    pageFaults++; // Increment global page faults counter
    masterPageTable[processId].pageFaults++; // Increment page faults counter for the specific process

    printf("Handling page fault for process %d, page %d.\n", processId, pageNumber); // Log the start of page fault handling

    int frameNumber = findFreeFrameOrEvict(); // Attempt to allocate a frame

    if (frameNumber != -1) {
        fetchPageFromSecondaryStorage(pageNumber); // Fetch the requested page

        MasterPageTableEntry* procEntry = &masterPageTable[processId];
        procEntry->pageTable[pageNumber].valid = 1;
        procEntry->pageTable[pageNumber].frameNumber = frameNumber;


        // Log the successful allocation
        printf("Page %d assigned frame %d after handling page fault for process %d.\n", pageNumber, frameNumber, processId);

        // Optionally, include logging for frame reallocation if needed
        printf("Frame %d reallocated to process %d for page %d.\n", frameNumber, processId, pageNumber);
    } else {
        // Log the failure to allocate a frame
        printf("Failed to handle page fault for process %d, page %d due to lack of frames.\n", processId, pageNumber);
    }
}


/*
 * Function: displayMemoryUtilization
 * Description: Displays the current utilization of physical memory frames.
 */
void displayMemoryUtilization() {
    int usedFrames = 0;
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (physicalMemory[i].used) usedFrames++;
    }
    printf("Memory Utilization: %d of %d frames used.\n", usedFrames, TOTAL_FRAMES);
}


/*
 * Function: displayPerformanceMetrics
 * Description: Displays performance metrics including page faults, fetches, and frame evictions.
 */
void displayPerformanceMetrics() {
    printf("Performance Metrics:\n");
    printf("Page Faults: %d\n", pageFaults);
    printf("Pages Fetched: %d\n", pageFetches);
    printf("Frame Evictions: %d\n", frameEvictions);
    displayMemoryUtilization(); // Display final memory utilization stats
}

/*
 * Function: initializeMemory
 * Description: Initializes the simulation's memory, marking all pages and frames as free.
 */
void initializeMemory() {
    for (int i = 0; i < TOTAL_PAGES; i++) {
        pageTable[i].valid = 0;
    }
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        physicalMemory[i].used = 0;
    }
}


/*
 * Function: translateAddress
 * Parameters: unsigned int virtualAddress, unsigned int processId
 * Returns: unsigned int (physical address)
 * Description: Translates a virtual address to a physical address for a given process.
 */
unsigned int translateAddress(unsigned int virtualAddress, unsigned int processId) {
    unsigned int pageNumber = virtualAddress / 256;
    if (pageTable[pageNumber].valid) {
        unsigned int frameNumber = pageTable[pageNumber].frameNumber;
        unsigned int offset = virtualAddress % 256;
        unsigned int physicalAddress = (frameNumber * 256) + offset;
        return physicalAddress;
    } else {
        handlePageFault(pageNumber, processId);
        return 0xFFFFFFFF; // Indicate page fault handled
    }
}




unsigned int translateAddressForProcess(int processId, unsigned int virtualAddress) {
    if (processId < 0 || processId >= MAX_PROCESSES || !masterPageTable[processId].isActive) {
        printf("Error: Invalid processId %d or process is not active.\n", processId);
        return 0xFFFFFFFF; // Error indicator
    }

    PageTableEntry* procPageTable = masterPageTable[processId].pageTable;
    unsigned int pageNumber = virtualAddress / PAGE_SIZE;
    
    if (pageNumber >= TOTAL_PAGES) {
        printf("Error: Virtual address %u out of bounds for process ID %d.\n", virtualAddress, processId);
        return 0xFFFFFFFF; // Error indicator
    }

    if (procPageTable[pageNumber].valid) {
        unsigned int frameNumber = procPageTable[pageNumber].frameNumber;
        unsigned int offset = virtualAddress % PAGE_SIZE;
        return (frameNumber * PAGE_SIZE) + offset;
    } else {
        handlePageFault(pageNumber, processId);
        // After handling the page fault, check again if the page is now valid.
        if (procPageTable[pageNumber].valid) {
            unsigned int frameNumber = procPageTable[pageNumber].frameNumber;
            unsigned int offset = virtualAddress % PAGE_SIZE;
            return (frameNumber * PAGE_SIZE) + offset;
        }
    }
    
    printf("Error: Failed to resolve page fault for process %d, address %u.\n", processId, virtualAddress);
    return 0xFFFFFFFF; // Page fault could not be resolved
}

/*
 * Function: simulateMalloc
 * Parameters: int processId
 * Description: Simulates a process requesting memory allocation (malloc).
 */
void simulateMalloc(int processId) {
    globalTotalAccessAttempts++;
    masterPageTable[processId].totalAccessAttempts++;

    unsigned int virtualAddress = (rand() % TOTAL_PAGES) * PAGE_SIZE; // Random virtual address within the process's address space

    if (isAddressAllocatedToProcess(processId, virtualAddress)) {
        printf("Error: Attempted to allocate an already allocated virtual address %u for Process %d.\n", virtualAddress, processId);
        return;
    }

    printf("Process %d requests memory allocation (malloc) for Virtual Address %u\n", processId, virtualAddress);
    unsigned int physicalAddress = translateAddressForProcess(processId, virtualAddress);

    if (physicalAddress != 0xFFFFFFFF) {
        printf("Allocated Virtual Address %u to Physical Address %u for Process %d\n", virtualAddress, physicalAddress, processId);
        addAllocatedAddress(&masterPageTable[processId], virtualAddress);
        globalSuccessfulAccesses++;
        masterPageTable[processId].successfulAccesses++;
    } else {
        printf("Failed to allocate memory for Process %d\n", processId);
    }
}



/*
 * Function: simulateFree
 * Parameters: int processId, unsigned int virtualAddress
 * Description: Simulates a process requesting to free allocated memory.
 */
void simulateFree(int processId, unsigned int virtualAddress) {
    globalTotalAccessAttempts++;
    masterPageTable[processId].totalAccessAttempts++;

    if (!isAddressAllocatedToProcess(processId, virtualAddress)) {
        printf("Error: Process %d attempted to free an unallocated or not owned virtual address %u.\n", processId, virtualAddress);
        return;
    }

    unsigned int pageNumber = virtualAddress / PAGE_SIZE;
    masterPageTable[processId].pageTable[pageNumber].valid = 0; // Invalidate the page

    removeAllocatedAddress(&masterPageTable[processId], virtualAddress);
    printf("Virtual Address %u freed for Process %d\n", virtualAddress, processId);
    globalSuccessfulAccesses++;
    masterPageTable[processId].successfulAccesses++;
}


/*
 * Function: simulateWrite
 * Parameters: int processId, unsigned int virtualAddress
 * Description: Simulates a process writing to a specified virtual address.
 */
void simulateRead(int processId, unsigned int virtualAddress) {
    if (!isAddressAllocatedToProcess(processId, virtualAddress)) {
        printf("Error: Process %d attempted to read unallocated memory at address %u.\n", processId, virtualAddress);
        return;
    }
    printf("Process %d reads from Virtual Address %u\n", processId, virtualAddress);
    globalSuccessfulAccesses++;
    masterPageTable[processId].successfulAccesses++;
}


/*
 * Function: simulateWrite
 * Parameters: int processId, unsigned int virtualAddress
 * Description: Simulates a process writing to a specified virtual address.
 */
void simulateWrite(int processId, unsigned int virtualAddress) {
    if (!isAddressAllocatedToProcess(processId, virtualAddress)) {
        printf("Error: Process %d attempted to write unallocated memory at address %u.\n", processId, virtualAddress);
        return;
    }
    printf("Process %d writes to Virtual Address %u\n", processId, virtualAddress);
    globalSuccessfulAccesses++;
    masterPageTable[processId].successfulAccesses++;
}

/*
 * Function: simulateMemoryAccess
 * Description: Simulates random memory access operations (malloc, free, read, write) for testing.
 */
void simulateMemoryAccess() {
    srand(time(NULL)); // Seed the random number generator
    const char* actions[] = {"malloc", "free", "read", "write"};
    int actionsCount = sizeof(actions) / sizeof(actions[0]);

    for (int i = 0; i < 100; i++) {
        int processId = rand() % MAX_PROCESSES; // Ensure this matches your process ID range
        // Randomly select an action
        const char* action = actions[rand() % actionsCount];

        printf("Simulating action '%s' for process %d.\n", action, processId);


        if (strcmp(action, "malloc") == 0) {
            simulateMalloc(processId);
        } else if (strcmp(action, "free") == 0) {
            if (masterPageTable[processId].allocatedCount > 0) {
                // Select a previously allocated address at random for demonstration purposes
                int allocatedIndex = rand() % masterPageTable[processId].allocatedCount;
                unsigned int virtualAddress = masterPageTable[processId].allocatedAddresses[allocatedIndex];
                simulateFree(processId, virtualAddress);
            }
        } else if (strcmp(action, "read") == 0 || strcmp(action, "write") == 0) {
            if (masterPageTable[processId].allocatedCount > 0) {
                // Select a valid address from the allocated addresses for the process
                int allocatedIndex = rand() % masterPageTable[processId].allocatedCount;
                unsigned int virtualAddress = masterPageTable[processId].allocatedAddresses[allocatedIndex];
                if (strcmp(action, "read") == 0) {
                    simulateRead(processId, virtualAddress);
                } else {
                    simulateWrite(processId, virtualAddress);
                }
            }
        }
    }
}



/*
 * Function: displayPageAllocation
 * Description: Displays the current allocation status of all pages, indicating which pages
 * are allocated and to which process they belong. This function is useful for tracking
 * page allocation patterns and identifying potential issues with page allocation and eviction.
 * Usage: Call this function after a series of memory operations to visually verify the allocation
 * status of pages. It's particularly helpful for debugging page fault handling and ensuring
 * that pages are correctly allocated to and freed by processes.
 */
void displayPageAllocation() {
    printf("Page Allocation Status:\n");
    // Iterate through each process
    for (int processId = 0; processId < MAX_PROCESSES; processId++) {
        if (masterPageTable[processId].isActive) {  // Check if the process is active
            PageTableEntry* procPageTable = masterPageTable[processId].pageTable;
            // Iterate through the page table of the current process
            for (int i = 0; i < TOTAL_PAGES; i++) {
                if (procPageTable[i].valid) {  // Check if the page is valid (allocated)
                    printf("Page %d is allocated to Process %d\n", i, processId);
                }
            }
        }
    }
}



/*
 * Function: displayHitRates
 * Description: Calculates and displays hit rates for memory accesses, both globally and
 * per process. The hit rate is the percentage of memory accesses that were successfully
 * served without incurring a page fault. This metric is crucial for evaluating the efficiency
 * of your memory management strategy.
 * Usage: Use this function at the end of your simulation or periodically during long-running
 * simulations to evaluate and compare the effectiveness of different memory management
 * techniques or configurations.
 */
void displayHitRates() {
    printf("Global Hit Rate: %.2f%%\n", (double)globalSuccessfulAccesses / globalTotalAccessAttempts * 100);

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (masterPageTable[i].isActive) {
            printf("Process %d Hit Rate: %.2f%%\n", i, 
                   (double)masterPageTable[i].successfulAccesses / masterPageTable[i].totalAccessAttempts * 100);
        }
    }
}

/*
 * Function: main
 * Description: Entry point of the program. Initializes the simulation and runs memory access tests.
 */
int main() {
    initializeMasterPageTable(); // Initialize the master page table structure
    
    // Initialize processes and their page tables
    for (int i = 0; i < MAX_PROCESSES; i++) {
        int pid = initializeProcessPageTable(); // Attempt to initialize each process
        if (pid == -1) {
            printf("Failed to initialize process %d\n", i);
            // Handle error or break if critical
        } else {
            printf("Process %d activated.\n", pid);
            printf("Process %d's page table initialized.\n", pid); // Corrected to use 'pid'
        }
    }

    simulateMemoryAccess(); // Proceed with simulation after initialization
    displayMemoryUtilization();
    displayPerformanceMetrics();
    displayHitRates();
    displayPageAllocation();
    

    return 0;
}

