#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <libkmod.h>

#define MAX_ERROR_LEN 512

typedef struct {
    char name[256];
    unsigned int size;
    int refcount;
    char deps[512];
    char state[64];  // Live, Loading, Unloading
} ModuleInfo;

/**
 * Initialize module manager context
 * Returns 0 on success, -1 on error
 */
int mm_init(void);

/**
 * Cleanup module manager
 */
void mm_cleanup(void);

/**
 * Load a kernel module
 * @param path: Path to .ko file
 * @param params: Optional module parameters (can be NULL)
 * Returns 0 on success, -1 on error
 */
int mm_load(const char *path, const char *params);

/**
 * Unload a kernel module
 * @param name: Module name (without .ko)
 * Returns 0 on success, -1 on error
 */
int mm_unload(const char *name);

/**
 * List all loaded modules
 * @param list: Output array of ModuleInfo (caller must free)
 * @param count: Output count of modules
 * Returns 0 on success, -1 on error
 */
int mm_list(ModuleInfo **list, int *count);

/**
 * Get last error message
 * Returns error string
 */
const char *mm_last_error(void);

/**
 * Check if module is loaded
 * @param name: Module name
 * Returns 1 if loaded, 0 if not, -1 on error
 */
int mm_is_loaded(const char *name);

/**
 * Get detailed information about a module
 * @param name: Module name
 * @param info: Output ModuleInfo struct (will be filled with module details)
 * Returns 0 on success, -1 on error
 */
int mm_get_info(const char *name, ModuleInfo *info);

/**
 * Get module info (internal helper)
 * @param info: Output ModuleInfo struct
 * @param mod: kmod_module pointer
 * @param name_of_module: Name of the module
 * Returns 0 on success, -1 on error
 */
int mm_get_info_module(ModuleInfo *info, struct kmod_module *mod, char *name_of_module);

#endif
