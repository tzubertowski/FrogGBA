/* FrogGBA PSP Volatile Memory Support Implementation */

#include "volatile_mem.h"
#include <stdlib.h>
#include <string.h>

// Volatile memory constants
#define VOLATILE_PARTITION_ID   5
#define VOLATILE_MEM_START      0x08400000
#define VOLATILE_MEM_END        0x08800000
#define VOLATILE_MEM_SIZE       (4 * 1024 * 1024)  // 4MB

// Metadata structure stored before each allocation
typedef struct {
    SceUID uid;
    size_t size;
} volatile_mem_header;

static int g_volatile_mem_inited = 0;
static int g_volatile_mem_locked = 0;

int volatile_mem_init(void)
{
    if (g_volatile_mem_inited)
        return 1;
    
    // For PSP-1000/2000, we can try to use partition 5 directly
    // This partition is typically available for user programs
    // We'll test if we can allocate from it
    
    // Try a small test allocation from partition 5
    SceUID test_uid = sceKernelAllocPartitionMemory(
        VOLATILE_PARTITION_ID,
        "test",
        PSP_SMEM_Low,
        1024,
        NULL
    );
    
    if (test_uid >= 0) {
        // Success! We can use partition 5
        sceKernelFreePartitionMemory(test_uid);
        g_volatile_mem_locked = 1;
        g_volatile_mem_inited = 1;
        
        // Lock power to prevent suspension while using volatile memory
        scePowerLock(0);
        
        return 1;
    }
    
    // Partition 5 not available
    return 0;
}

void volatile_mem_shutdown(void)
{
    if (g_volatile_mem_inited && g_volatile_mem_locked) {
        // Unlock power
        scePowerUnlock(0);
        
        // Note: We don't unlock volatile memory as it might still be in use
        g_volatile_mem_locked = 0;
    }
    
    g_volatile_mem_inited = 0;
}

void* volatile_mem_alloc(size_t size)
{
    if (!g_volatile_mem_inited || !g_volatile_mem_locked) {
        // Fall back to regular malloc
        return malloc(size);
    }
    
    // Align size to 64 bytes for cache efficiency
    size_t aligned_size = (size + 63) & ~63;
    
    // Add space for header
    size_t total_size = aligned_size + sizeof(volatile_mem_header);
    
    // Allocate from volatile memory partition
    SceUID uid = sceKernelAllocPartitionMemory(
        VOLATILE_PARTITION_ID,
        "FrogGBA_Cache",
        PSP_SMEM_Low,  // Allocate from low addresses
        total_size,
        NULL
    );
    
    if (uid < 0) {
        // Allocation failed, fall back to regular malloc
        return malloc(size);
    }
    
    // Get the allocated memory pointer
    void* base_ptr = sceKernelGetBlockHeadAddr(uid);
    if (!base_ptr) {
        sceKernelFreePartitionMemory(uid);
        return malloc(size);
    }
    
    // Store metadata
    volatile_mem_header* header = (volatile_mem_header*)base_ptr;
    header->uid = uid;
    header->size = total_size;
    
    // Return pointer after header
    return (void*)((char*)base_ptr + sizeof(volatile_mem_header));
}

void volatile_mem_free(void* ptr)
{
    if (!ptr)
        return;
    
    // Check if this is volatile memory
    if (is_volatile_mem(ptr)) {
        // Get header
        volatile_mem_header* header = (volatile_mem_header*)((char*)ptr - sizeof(volatile_mem_header));
        
        // Free the memory block
        sceKernelFreePartitionMemory(header->uid);
    } else {
        // Regular memory
        free(ptr);
    }
}

int is_volatile_mem(void* ptr)
{
    uintptr_t addr = (uintptr_t)ptr;
    return (addr >= VOLATILE_MEM_START && addr < VOLATILE_MEM_END);
}

size_t volatile_mem_available(void)
{
    if (!g_volatile_mem_inited || !g_volatile_mem_locked)
        return 0;
    
    // This is an approximation - actual implementation would query partition info
    // For now, assume we can use most of the 4MB
    return VOLATILE_MEM_SIZE - (256 * 1024);  // Reserve 256KB for system use
}