// Test Case 1: Initializing process page table
int processId = initializeProcessPageTable();
if (processId != -1) {
    printf("Process %d page table initialized successfully.\n", processId);
} else {
    printf("Failed to initialize process page table.\n");
}

// Test Case 2: Assigning page table to process
PageTableEntry* pageTable = (PageTableEntry*)malloc(TOTAL_PAGES * sizeof(PageTableEntry));
assignPageTableToProcess(processId, pageTable);
if (masterPageTable[processId].pageTable == pageTable) {
    printf("Page table assigned to process %d successfully.\n", processId);
} else {
    printf("Failed to assign page table to process %d.\n", processId);
}

// Test Case 3: Adding allocated addresses
unsigned int address1 = 0x1234;
unsigned int address2 = 0x5678;
addAllocatedAddress(&masterPageTable[processId], address1);
addAllocatedAddress(&masterPageTable[processId], address2);
if (masterPageTable[processId].allocatedCount == 2 &&
    masterPageTable[processId].allocatedAddresses[0] == address1 &&
    masterPageTable[processId].allocatedAddresses[1] == address2) {
    printf("Allocated addresses added successfully.\n");
} else {
    printf("Failed to add allocated addresses.\n");
}

// Test Case 4: Removing allocated addresses
removeAllocatedAddress(&masterPageTable[processId], address1);
if (masterPageTable[processId].allocatedCount == 1 &&
    masterPageTable[processId].allocatedAddresses[0] == address2) {
    printf("Allocated address removed successfully.\n");
} else {
    printf("Failed to remove allocated address.\n");
}

// Test Case 5: Handling page fault
unsigned int pageNumber = 5;
handlePageFault(pageNumber, processId);
if (pageTable[pageNumber].valid && pageTable[pageNumber].frameNumber >= 0) {
    printf("Page fault handled successfully.\n");
} else {
    printf("Failed to handle page fault.\n");
}

// Test Case 6: Translating virtual address
unsigned int virtualAddress = 0xABCDEF;
unsigned int physicalAddress = translateAddress(virtualAddress, processId);
if (physicalAddress != 0xFFFFFFFF) {
    printf("Virtual address translated successfully. Physical address: 0x%X\n", physicalAddress);
} else {
    printf("Failed to translate virtual address.\n");
}

// Test Case 7: Displaying memory utilization
displayMemoryUtilization();

// Test Case 8: Displaying performance metrics
displayPerformanceMetrics();

// Test Case 9: Initializing memory
initializeMemory();
printf("Memory initialized successfully.\n");

// Test Case 10: Translating virtual address for process
unsigned int processId2 = 1;
unsigned int virtualAddress2 = 0x123456;
unsigned int physicalAddress2 = translateAddressForProcess(processId2, virtualAddress2);
if (physicalAddress2 != 0xFFFFFFFF) {
    printf("Virtual address translated successfully for process %d. Physical address: 0x%X\n", processId2, physicalAddress2);
} else {
    printf("Failed to translate virtual address for process %d.\n", processId2);
}