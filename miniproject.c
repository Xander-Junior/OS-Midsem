#include <stdio.h>
#include <stdlib.h> // For rand()
#include <time.h>   // For time()

typedef struct {
    unsigned int valid:1;    // Valid bit: 1 if page is in physical memory, 0 otherwise - to prevent page faults
    unsigned int frameNumber:4; // Frame number: Assuming a max of 16 frames, 4 bits needed
} PageTableEntry;

#define TOTAL_PAGES 256 
PageTableEntry pageTable[TOTAL_PAGES];

#define TOTAL_FRAMES 16
typedef struct {
    unsigned int used:1; // Used bit: 1 if frame is allocated, 0 if free
} Frame;
Frame physicalMemory[TOTAL_FRAMES];

#define MAX_PROCESSES 10 // Assuming a max of 10 processes for simplicity

typedef struct {
    PageTableEntry* pageTable; // Pointer to a process's page table
    int isActive; // Simple flag to indicate if this process's page table is active
} MasterPageTableEntry;

MasterPageTableEntry masterPageTable[MAX_PROCESSES];

// When creating or loading a process, it would be assigned a page table and recorded in the master page table:
void initializeMasterPageTable() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        masterPageTable[i].pageTable = NULL; // Initially, no page table assigned
        masterPageTable[i].isActive = 0; // Process is not active
    }
}

void assignPageTableToProcess(int processId, PageTableEntry* pageTable) {
    if (processId < MAX_PROCESSES) {
        masterPageTable[processId].pageTable = pageTable;
        masterPageTable[processId].isActive = 1; // Mark the process as active
    }
}

unsigned int translateAddressForProcess(int processId, unsigned int virtualAddress) {
    if (processId < MAX_PROCESSES && masterPageTable[processId].isActive) {
        PageTableEntry* pageTable = masterPageTable[processId].pageTable;
        // Use pageTable to translate the address as before
    }
    // Handle error or invalid processId
}


void fetchPageFromSecondaryStorage(unsigned int pageNumber) {
    printf("Fetching page %u from secondary storage.\n", pageNumber);
}

int findFreeFrameOrEvict() {
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (!physicalMemory[i].used) return i;
    }
    // Placeholder for eviction logic; should implement a page replacement algorithm
    return 0; // Simplified for example purposes
}

void handlePageFault(unsigned int pageNumber) {
    int frameNumber = findFreeFrameOrEvict();
    // Evict page if necessary (not shown here for simplicity)
    fetchPageFromSecondaryStorage(pageNumber);
    physicalMemory[frameNumber].used = 1;
    pageTable[pageNumber].valid = 1;
    pageTable[pageNumber].frameNumber = frameNumber;
}

void displayMemoryUtilization() {
    int usedFrames = 0;
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (physicalMemory[i].used) usedFrames++;
    }
    printf("Memory Utilization: %d of %d frames used.\n", usedFrames, TOTAL_FRAMES);
}

void initializeMemory() {
    for (int i = 0; i < TOTAL_PAGES; i++) {
        pageTable[i].valid = 0;
    }
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        physicalMemory[i].used = 0;
    }
}

unsigned int translateAddress(unsigned int virtualAddress) {
    unsigned int pageNumber = virtualAddress / 256;
    if (pageTable[pageNumber].valid) {
        unsigned int frameNumber = pageTable[pageNumber].frameNumber;
        unsigned int offset = virtualAddress % 256;
        unsigned int physicalAddress = (frameNumber * 256) + offset;
        return physicalAddress;
    } else {
        handlePageFault(pageNumber);
        return 0xFFFFFFFF; // Indicate page fault handled
    }
}

void simulateMemoryAccess() {
    srand(time(NULL)); // Seed the random number generator
    for (int i = 0; i < 100; i++) { // Simulate 100 memory accesses
        unsigned int virtualAddress = (rand() % TOTAL_PAGES) * 256 + (rand() % 256); // Random page and offset
        translateAddress(virtualAddress); // This now handles page faults internally
    }
}

int main() {
    initializeMemory(); // Initialize page table and frames only once

    simulateMemoryAccess(); // Run the memory access simulation
    displayMemoryUtilization(); // Display final memory utilization stats

    // The rest of your main function logic can go here

    return 0;
}
