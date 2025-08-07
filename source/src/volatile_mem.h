/* FrogGBA PSP Volatile Memory Support
 *
 * This provides access to an additional 4MB of RAM on PSP
 * through the volatile memory partition (partition 5).
 * Based on implementation from DaedalusX64.
 */

#ifndef VOLATILE_MEM_H
#define VOLATILE_MEM_H

#include <pspkernel.h>
#include <pspsysmem.h>
#include <psppower.h>
#include <pspsuspend.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize volatile memory system
// Returns 1 on success, 0 on failure
int volatile_mem_init(void);

// Shutdown volatile memory system
void volatile_mem_shutdown(void);

// Allocate memory from volatile partition
// Falls back to regular malloc if volatile memory unavailable
void* volatile_mem_alloc(size_t size);

// Free volatile memory
void volatile_mem_free(void* ptr);

// Check if pointer is in volatile memory range
int is_volatile_mem(void* ptr);

// Get available volatile memory size
size_t volatile_mem_available(void);

#ifdef __cplusplus
}
#endif

#endif // VOLATILE_MEM_H