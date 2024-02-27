#include <stdio.h>
#include <stdlib.h> // For rand()
#include <time.h>   // For time()

typedef struct {
    unsigned int valid:1;    // Valid bit: 1 if page is in physical memory, 0 otherwise
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

///charles did this

#define MAX_PROCESSES 32

typedef struct {
    unsigned int processId;
    unsigned int baseAddress; // This could be a bias or base for the process's logical address space
} Process;

Process processes[MAX_PROCESSES];

///


int main() {
    initializeMemory(); // Initialize page table and frames only once

    simulateMemoryAccess(); // Run the memory access simulation
    displayMemoryUtilization(); // Display final memory utilization stats

    // The rest of your main function logic can go here

    return 0;
}
