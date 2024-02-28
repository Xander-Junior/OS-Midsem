#include <stdio.h>
#include <stdlib.h> // For rand()
#include <time.h>   // For time()

int pageFaults = 0; // To track the number of page faults
int pageFetches = 0; // To track the number of pages fetched from secondary storage
int frameEvictions = 0; // To track the number of frame evictions

typedef struct {
    unsigned int valid:1;    // Valid bit: 1 if page is in physical memory, 0 otherwise - to prevent page faults
    unsigned int frameNumber:4; // Frame number: Assuming a max of 16 frames, 4 bits needed
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
	pageFetches++;
}

int findFreeFrameOrEvict() {
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (!physicalMemory[i].used) return i;
    }
    // Placeholder for eviction logic; should implement a page replacement algorithm
    return 0; // Simplified for example purposes
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



int findFreeFrameOrEvict() {
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (!physicalMemory[i].used) {
            enqueue(i); // Enqueue this frame as it's now being used
            return i;
        }
    }
    
    // If no free frame is found, proceed to evict the oldest frame
    int frameToEvict = dequeue();
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


void handlePageFault(unsigned int pageNumber) {
	pageFaults++; // Increment page faults
    	int frameNumber = findFreeFrameOrEvict();
    	if (frameNumber == -1) { // Check if a frame was successfully allocated or evicted
		fetchPageFromSecondaryStorage(pageNumber);
        	physicalMemory[frameNumber].used = 1;
        	physicalMemory[frameNumber].processId = processId; // Assign the process ID
        	pageTable[pageNumber].valid = 1;
        	pageTable[pageNumber].frameNumber = frameNumber;
	}else{
		printf("Error: No available frame and eviction not possible.\n");
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
        unsigned int processId = rand() % 32; // Random process ID
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
