/* FrogGBA PSP Volatile Memory Support Implementation 
 * Based on DaedalusX64 implementation */

#include "volatile_mem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Volatile memory constants
#define VOLATILE_PARTITION_ID   5
#define VOLATILE_MEM_BOUNDARY   0x08800000  // Memory above this is not volatile

static int g_volatile_mem_available = 0;

int volatile_mem_init(void)
{
    if (g_volatile_mem_available)
        return 1;
    
    // Try to unlock volatile memory using the proper PSP function
    void* pointer = NULL;
    int size = 0;
    int result = sceKernelVolatileMemLock(0, &pointer, &size);
    
    if (result == 0) {
        // Success! Lock power to prevent suspension while using volatile memory
        scePowerLock(0);
        printf("Successfully unlocked volatile mem: %d KB\n", size / 1024);
        g_volatile_mem_available = 1;
        return 1;
    } else {
        printf("Failed to unlock volatile mem: %08x\n", result);
        g_volatile_mem_available = 0;
        return 0;
    }
}

void volatile_mem_shutdown(void)
{
    if (g_volatile_mem_available) {
        // Unlock power
        scePowerUnlock(0);
        g_volatile_mem_available = 0;
    }
}

void* volatile_mem_alloc(size_t size)
{
    // If volatile memory is not available, use regular malloc
    if (!g_volatile_mem_available) {
        return malloc(size);
    }
    
    // Add space for metadata: SceUID (4 bytes) + size (4 bytes) = 8 bytes
    // This matches the DaedalusX64 implementation
    SceUID uid = sceKernelAllocPartitionMemory(
        VOLATILE_PARTITION_ID,
        "",  // Empty name like DaedalusX64
        PSP_SMEM_Low,
        size + 8,  // Add 8 bytes for metadata
        NULL
    );
    
    if (uid >= 0) {
        // Get the memory block pointer
        u32* pointer = (u32*)sceKernelGetBlockHeadAddr(uid);
        if (pointer) {
            // Store metadata at the beginning: UID at offset 0, size at offset 4
            *pointer = uid;
            *(pointer + 1) = size;
            
            // Return pointer after the 8-byte header
            return (void*)(pointer + 2);
        } else {
            // Failed to get block address, free and fall back
            sceKernelFreePartitionMemory(uid);
        }
    }
    
    // Allocation failed, fall back to regular malloc
    return malloc(size);
}

void volatile_mem_free(void* ptr)
{
    if (!ptr)
        return;
    
    // Check if this is volatile memory (below the boundary)
    if ((uintptr_t)ptr < VOLATILE_MEM_BOUNDARY) {
        // This is volatile memory, get the UID from 8 bytes before the pointer
        u32* metadata_ptr = (u32*)ptr - 2;  // Go back 8 bytes (2 u32s)
        SceUID uid = *metadata_ptr;
        sceKernelFreePartitionMemory(uid);
    } else {
        // Regular memory above the boundary
        free(ptr);
    }
}

int is_volatile_mem(void* ptr)
{
    return ((uintptr_t)ptr < VOLATILE_MEM_BOUNDARY);
}

size_t volatile_mem_available(void)
{
    // This is an approximation - in practice we'd need to query the partition
    return g_volatile_mem_available ? (4 * 1024 * 1024) : 0;
}