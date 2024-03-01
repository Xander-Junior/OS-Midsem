#include <stdio.h>
#include <stdlib.h> // For rand()
#include <time.h>   // For time()
#include <string.h> // For strcmp


int pageFaults = 0; // To track the number of page faults
int pageFetches = 0; // To track the number of pages fetched from secondary storage
int frameEvictions = 0; // To track the number of frame evictions

#define PAGE_SIZE 256  // Each page is 256 bytes
#define TOTAL_VIRTUAL_MEMORY (TOTAL_PAGES * PAGE_SIZE)  // Total virtual memory size
#define TOTAL_PHYSICAL_MEMORY (TOTAL_FRAMES * PAGE_SIZE)  // Total physical memory size

int globalTotalAccessAttempts = 0;
int globalSuccessfulAccesses = 0;




typedef struct {
    unsigned int valid:1;    // Valid bit: 1 if page is in physical memory, 0 otherwise - to prevent page faults
    unsigned int frameNumber:4; // Frame number: Assuming a max of 16 frames, 4 bits needed
    int processId; // Add process ID to track ownership
} PageTableEntry;

#define TOTAL_PAGES 256 
PageTableEntry pageTable[TOTAL_PAGES];

#define TOTAL_FRAMES 16
typedef struct {
    unsigned int used:1; // Used bit: 1 if frame is allocated, 0 if free
    unsigned int processId : 5; // Assuming a maximum of 32 processes, 5 bits needed for process ID
} Frame;
Frame physicalMemory[TOTAL_FRAMES];

#define MAX_PROCESSES 10 // Assuming a max of 10 processes for simplicity


typedef struct {
    PageTableEntry* pageTable; // Existing page table pointer
    int isActive; // Existing activity flag
    unsigned int* allocatedAddresses; // Array to track allocated addresses
    int allocatedCount; // Number of allocated addresses
    int pageFaults; // Existing page fault counter
    int totalAccessAttempts; // Total attempts to access memory
    int successfulAccesses; // Successful memory accesses without a page fault
} MasterPageTableEntry;

MasterPageTableEntry masterPageTable[MAX_PROCESSES];



// Function to check if a given address is allocated to a process
int isAddressAllocatedToProcess(int processId, unsigned int address) {
    if (processId < 0 || processId >= MAX_PROCESSES) {
        return 0; // Invalid process ID
    }

    MasterPageTableEntry* processEntry = &masterPageTable[processId];
    for (int i = 0; i < processEntry->allocatedCount; i++) {
        if (processEntry->allocatedAddresses[i] == address) {
            return 1; // Address is allocated to the process
        }
    }

    return 0; // Address not found in the process's allocated addresses
}



// Dynamically resizing the allocated addresses array
void addAllocatedAddress(MasterPageTableEntry* entry, unsigned int address) {
    entry->allocatedAddresses = realloc(entry->allocatedAddresses, (entry->allocatedCount + 1) * sizeof(unsigned int));
    if (entry->allocatedAddresses != NULL) {
        entry->allocatedAddresses[entry->allocatedCount] = address;
        entry->allocatedCount++;
    }
}

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


// Function to dynamically assign a process ID and initialize its page table
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




void fetchPageFromSecondaryStorage(unsigned int pageNumber) {
	printf("Fetching page %u from secondary storage.\n", pageNumber);
	pageFetches++;
}


#define QUEUE_SIZE TOTAL_FRAMES // Assuming one queue entry per frame for simplicity

// Global variables to implement the FIFO queue
int fifoQueue[QUEUE_SIZE];
int front = -1, rear = -1;

// Function to enqueue a frame number into the FIFO queue
void enqueue(int frameNumber) {
    // Check if the queue is full
    if ((rear + 1) % QUEUE_SIZE == front) {
        printf("Queue is Full\n");
        return;
    }
    // If the queue is empty, set front to 0
    if (front == -1) front = 0;
    // Increment rear and wrap around if necessary
    rear = (rear + 1) % QUEUE_SIZE;
    // Enqueue the frame number
    fifoQueue[rear] = frameNumber;

    printf("Enqueuing frame %d. Current rear: %d, front: %d\n", frameNumber, rear, front);

}

// Function to dequeue a frame number from the FIFO queue
int dequeue() {
    // Check if the queue is empty
    if (front == -1) {
        printf("Queue is Empty\n");
        return -1;
    }
    // Dequeue the frame number
    int frameNumber = fifoQueue[front];
    // If front and rear are equal, the queue becomes empty
    if (front == rear) front = rear = -1;
    // Otherwise, increment front and wrap around if necessary
    else front = (front + 1) % QUEUE_SIZE;

    printf("Dequeueing frame. Current front: %d, rear: %d\n", front, rear);

    return frameNumber;
}



int findFreeFrameOrEvict() {
    int frameToEvict = dequeue();
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (!physicalMemory[i].used) {
            enqueue(i); // Enqueue this frame as it's now being used
            return i;
        }
    }
    
    // If no free frame is found, proceed to evict the oldest frame
    
    if (frameToEvict == -1) {
        printf("Error: FIFO queue is empty. Cannot evict a frame.\n");
        return -1; // Error case, queue was empty
    }
    
    // Before evicting, we need to find which page is currently using this frame
    // and invalidate that page in the page table. This requires a reverse lookup.
    for (unsigned int pageNumber = 0; pageNumber < TOTAL_PAGES; pageNumber++) {
        if (pageTable[pageNumber].valid && pageTable[pageNumber].frameNumber == frameToEvict) {
            pageTable[pageNumber].valid = 0; // Invalidate the page table entry
            break; // Assuming one page per frame, we can break once found
        }
    }
    
    // Note: Here you would ideally handle copying the frame's content back to secondary storage if needed
    return frameToEvict;
}


void handlePageFault(unsigned int pageNumber, unsigned int processId) {
    pageFaults++; // Assuming pageFaults is a global counter of page faults
    int frameNumber = findFreeFrameOrEvict(); // This can be a newly found free frame or an evicted frame.
    printf("Handling page fault for process %d, page %d.\n", processId, pageNumber);
    masterPageTable[processId].pageFaults++;

    // Correctly check for a successful frame allocation/eviction
    if (frameNumber != -1) {
        printf("Page %d assigned frame %d.\n", pageNumber, frameNumber);
        fetchPageFromSecondaryStorage(pageNumber);
        physicalMemory[frameNumber].used = 1;
        physicalMemory[frameNumber].processId = processId;
        pageTable[pageNumber].valid = 1;
        pageTable[pageNumber].frameNumber = frameNumber;
        enqueue(frameNumber); // Optionally re-enqueue the frame after allocation for tracking

        masterPageTable[processId].pageTable[pageNumber].valid = 1;
        masterPageTable[processId].pageTable[pageNumber].frameNumber = frameNumber;
        // If you want to log the eviction here, ensure you have a way to know an eviction occurred.
    } else {
        printf("Error: No available frame and eviction not possible.\n");
        printf("Failed to allocate frame for page %d.\n", pageNumber);
    }
}


void displayMemoryUtilization() {
    int usedFrames = 0;
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (physicalMemory[i].used) usedFrames++;
    }
    printf("Memory Utilization: %d of %d frames used.\n", usedFrames, TOTAL_FRAMES);
}


void displayPerformanceMetrics() {
    printf("Performance Metrics:\n");
    printf("Page Faults: %d\n", pageFaults);
    printf("Pages Fetched: %d\n", pageFetches);
    printf("Frame Evictions: %d\n", frameEvictions);
    displayMemoryUtilization(); // Display final memory utilization stats
}


void initializeMemory() {
    for (int i = 0; i < TOTAL_PAGES; i++) {
        pageTable[i].valid = 0;
    }
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        physicalMemory[i].used = 0;
    }
}

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
    // Check for invalid process ID or inactive process
    if (processId < 0 || processId >= MAX_PROCESSES || !masterPageTable[processId].isActive) {
        printf("Error: Invalid processId %d or process is not active.\n", processId);
        return 0xFFFFFFFF; // Indicate error
    }

    PageTableEntry* pageTable = masterPageTable[processId].pageTable;
    if (!pageTable) {
        // This check ensures that the process has an assigned and initialized page table.
        printf("Error: Page table for process ID %d not initialized.\n", processId);
        return 0xFFFFFFFF; // Indicate error
    }

    // Calculate the page number from the virtual address
    unsigned int pageNumber = virtualAddress / 256; // Assuming 256-byte pages
    if (pageNumber >= TOTAL_PAGES) {
        printf("Error: Virtual address %u out of bounds for process ID %d.\n", virtualAddress, processId);
        return 0xFFFFFFFF; // Indicate error
    }

    // If the page is valid, translate the virtual address to a physical address
    if (pageTable[pageNumber].valid) {
        unsigned int frameNumber = pageTable[pageNumber].frameNumber;
        unsigned int offset = virtualAddress % 256;
        unsigned int physicalAddress = (frameNumber * 256) + offset;
        return physicalAddress;
    } else {
        // Handle the page fault if the page is not valid (i.e., not in physical memory)e
        handlePageFault(pageNumber, processId);
        return 0xFFFFFFFF; // Indicate that a page fault occurred and was handled
    }
}


void simulateMalloc(int processId) {

    globalTotalAccessAttempts++;
    masterPageTable[processId].totalAccessAttempts++;

    unsigned int virtualAddress = (rand() % TOTAL_PAGES) * 256; // Assuming page size is 256

    // Check if the virtual address is already allocated to prevent double allocation
    if (isAddressAllocatedToProcess(processId, virtualAddress)) {
        printf("Error: Attempted to allocate an already allocated virtual address %u for Process %d.\n", virtualAddress, processId);
        return;
    }

    printf("Process %d requests memory allocation (malloc) for Virtual Address %u\n", processId, virtualAddress);
    unsigned int physicalAddress = translateAddressForProcess(processId, virtualAddress);

         // If we reach this point, the access is successful
    globalSuccessfulAccesses++;
    masterPageTable[processId].successfulAccesses++;

    if (physicalAddress != 0xFFFFFFFF) {
        printf("Allocated Virtual Address %u to Physical Address %u for Process %d\n", virtualAddress, physicalAddress, processId);
        // Remember to add the allocated address to the process's allocation list
        addAllocatedAddress(&masterPageTable[processId], virtualAddress);
    } else {
        printf("Failed to allocate memory for Process %d\n", processId);
    }
}


void simulateFree(int processId, unsigned int virtualAddress) {

    globalTotalAccessAttempts++;
    masterPageTable[processId].totalAccessAttempts++;

    printf("Process %d requests to free memory at Virtual Address %u\n", processId, virtualAddress);

    if (!isAddressAllocatedToProcess(processId, virtualAddress)) {
        printf("Error: Process %d attempted to free an unallocated or not owned virtual address %u.\n", processId, virtualAddress);
        return;
    }

         // If we reach this point, the access is successful
    globalSuccessfulAccesses++;
    masterPageTable[processId].successfulAccesses++;

    // Proceed to free the memory since it's verified to be allocated to the process
    unsigned int pageNumber = virtualAddress / 256; // Assuming page size is 256
    masterPageTable[processId].pageTable[pageNumber].valid = 0; // Mark as free
    removeAllocatedAddress(&masterPageTable[processId], virtualAddress);
    printf("Virtual Address %u freed for Process %d\n", virtualAddress, processId);
}



void simulateRead(int processId, unsigned int virtualAddress) {

    globalTotalAccessAttempts++;
    masterPageTable[processId].totalAccessAttempts++;

    if (!isAddressAllocatedToProcess(processId, virtualAddress)) {
        printf("Error: Process %d attempted to read unallocated memory at address %u.\n", processId, virtualAddress);
        return;
    }

     // If we reach this point, the access is successful
    globalSuccessfulAccesses++;
    masterPageTable[processId].successfulAccesses++;

    printf("Process %d reads from Virtual Address %u\n", processId, virtualAddress);
    // Proceed with reading...
}

void simulateWrite(int processId, unsigned int virtualAddress) {

    globalTotalAccessAttempts++;
    masterPageTable[processId].totalAccessAttempts++;

    if (!isAddressAllocatedToProcess(processId, virtualAddress)) {
        printf("Error: Process %d attempted to write unallocated memory at address %u.\n", processId, virtualAddress);
        return;
    }

      // If we reach this point, the access is successful
    globalSuccessfulAccesses++;
    masterPageTable[processId].successfulAccesses++;

    printf("Process %d writes to Virtual Address %u\n", processId, virtualAddress);
    // Proceed with writing...
}


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




void displayPageAllocation() {
    printf("Page Allocation Status:\n");
    for (int i = 0; i < TOTAL_PAGES; i++) {
        if (pageTable[i].valid) {
            printf("Page %d is allocated to Process %d\n", i, pageTable[i].processId);
        }
    }
}

void displayHitRates() {
    printf("Global Hit Rate: %.2f%%\n", (double)globalSuccessfulAccesses / globalTotalAccessAttempts * 100);

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (masterPageTable[i].isActive) {
            printf("Process %d Hit Rate: %.2f%%\n", i, 
                   (double)masterPageTable[i].successfulAccesses / masterPageTable[i].totalAccessAttempts * 100);
        }
    }
}


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

    return 0;
}

