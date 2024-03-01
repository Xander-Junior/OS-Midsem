#include <stdio.h>
#include <stdlib.h> // For rand()
#include <time.h>   // For time()
#include <string.h> // For strcmp


int pageFaults = 0; // To track the number of page faults
int pageFetches = 0; // To track the number of pages fetched from secondary storage
int frameEvictions = 0; // To track the number of frame evictions

typedef struct {
    unsigned int valid:1;    // Valid bit: 1 if page is in physical memory, 0 otherwise
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
} MasterPageTableEntry;

MasterPageTableEntry masterPageTable[MAX_PROCESSES];


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
// Initialization function for master page table and process page tables
void initializeMasterPageTable() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        masterPageTable[i].isActive = 0; // Process is not active initially
        masterPageTable[i].allocatedAddresses = NULL;
        masterPageTable[i].allocatedCount = 0;
        masterPageTable[i].pageTable = (PageTableEntry*)malloc(TOTAL_PAGES * sizeof(PageTableEntry));

        if (masterPageTable[i].pageTable != NULL) {
            for (int j = 0; j < TOTAL_PAGES; j++) {
                masterPageTable[i].pageTable[j].valid = 0;
                masterPageTable[i].pageTable[j].frameNumber = 0;
                masterPageTable[i].pageTable[j].processId = i; // Assign the process ID here
            }
        }
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
    return frameNumber;
}

// Function to find a free frame in memory (using a straightforward approach)
int findFreeFrame(Frame physicalMemory[]) {
    // Iterate through each frame
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        // If the frame is not in use, return its index
        if (!physicalMemory[i].used) return i;
    }
    // If no free frame is found, return -1
    return -1;
}





int findFreeFrameOrEvict(unsigned int processId) { // Added processId parameter
    // Frame allocation logic
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (!physicalMemory[i].used) {
            enqueue(i); // Enqueue this frame as it's now being used
            return i;
        }
    }

    // Evict the oldest frame
    int frameToEvict = dequeue();
    if (frameToEvict != -1) {
        // Invalidate the page using this frame in the specified process's page table
        PageTableEntry* processPageTable = masterPageTable[processId].pageTable;
        for (unsigned int pageNumber = 0; pageNumber < TOTAL_PAGES; pageNumber++) {
            if (processPageTable[pageNumber].valid && processPageTable[pageNumber].frameNumber == frameToEvict) {
                processPageTable[pageNumber].valid = 0; // Invalidate
                break;
            }
        }
        frameEvictions++;
        return frameToEvict;
    } else {
        printf("Error: FIFO queue is empty. Cannot evict a frame.\n");
        return -1; // Error case, queue was empty
    }
}



void handlePageFault(unsigned int pageNumber, unsigned int processId) {
    pageFaults++;
    PageTableEntry* processPageTable = masterPageTable[processId].pageTable; // Get the specific process's page table

    int frameNumber = findFreeFrameOrEvict(processId); // Adjusted to consider process ID
    if (frameNumber != -1) {
        fetchPageFromSecondaryStorage(pageNumber);
        physicalMemory[frameNumber].used = 1;
        physicalMemory[frameNumber].processId = processId;
        processPageTable[pageNumber].valid = 1;
        processPageTable[pageNumber].frameNumber = frameNumber;
    } else {
        // Error handling: No available frame and eviction not possible
        printf("Error: No available frame and eviction not possible.\n");
        printf("Failed to allocate frame for page %d.\n", pageNumber);
        // Optionally, increment an error counter or take other recovery actions
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
    printf("Caught Page Faults: %d\n", pageFaults);
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
        // Handle the page fault if the page is not valid (i.e., not in physical memory)
        handlePageFault(pageNumber, processId);
        return 0xFFFFFFFF; // Indicate that a page fault occurred and was handled
    }
}


typedef struct {
    unsigned int processId;
    unsigned int baseAddress; // This could be a bias or base for the process's logical address space
} Process;

Process processes[MAX_PROCESSES];


void simulateMalloc(int processId) {
    // Simulate requesting a random amount of memory, not just a single page
    int numPages = (rand() % 5) + 1; // For example, request between 1 and 5 pages

    printf("Process %d requests memory allocation of %d pages\n", processId, numPages);

    for (int i = 0; i < numPages; i++) {
        unsigned int virtualAddress = (rand() % TOTAL_PAGES) * 256; // Assuming page size is 256
        // This next step assumes the process is requesting contiguous pages, which might not always be realistic
        // A more sophisticated simulation would track allocated ranges and find suitable spots for each new request
        unsigned int physicalAddress = translateAddressForProcess(processId, virtualAddress + (i * 256));

        if (physicalAddress != 0xFFFFFFFF) {
            printf("Allocated Virtual Address %u (Page %d of %d) for Process %d\n", virtualAddress + (i * 256), i + 1, numPages, processId);
            // Here, you'd add logic to track this allocation in a process-specific table
        } else {
            printf("Failed to allocate memory for Process %d\n", processId);
            // Optionally, handle partial allocation failure (e.g., roll back or leave as fragmented)
            break;
        }
    }
}



void simulateFree(int processId, unsigned int virtualAddress) {
    printf("Process %d requests to free memory at Virtual Address %u\n", processId, virtualAddress);
    unsigned int pageNumber = virtualAddress / 256; // Assuming page size is 256

    // Check if the page is currently allocated
    PageTableEntry* processPageTable = masterPageTable[processId].pageTable;

    if (processPageTable[pageNumber].valid) {
        processPageTable[pageNumber].valid = 0; // Mark as free
        removeAllocatedAddress(&masterPageTable[processId], virtualAddress);

        printf("Virtual Address %u freed for Process %d\n", virtualAddress, processId);


    } else {
        printf("Virtual Address %u is not currently allocated for Process %d\n", virtualAddress, processId);
    }
}
void simulateRead(int processId, unsigned int virtualAddress) {
    printf("Process %d reads from Virtual Address %u\n", processId, virtualAddress);
    // You might check if the address is valid and allocated before reading.
}

void simulateWrite(int processId, unsigned int virtualAddress) {
    printf("Process %d writes to Virtual Address %u\n", processId, virtualAddress);
    // Similarly, check if the address is valid and allocated before writing.
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


int main() {
    initializeMasterPageTable(); // Initialize the master page table structure

    // Initialize processes and their page tables
    for (int i = 0; i < 2; i++) {
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

