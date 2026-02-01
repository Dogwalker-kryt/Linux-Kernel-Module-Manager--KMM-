#include "../include/module_manager.h"
#include <libkmod.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

// Global error message buffer
static char g_error_msg[MAX_ERROR_LEN] = {0};

// Global libkmod context
static struct kmod_ctx *g_ctx = NULL;

// Helper for setting error message
static void set_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_error_msg, MAX_ERROR_LEN, fmt, args);
    va_end(args);
}

// Initialize module manager
int mm_init(void) {
    if (g_ctx != NULL) {
        set_error("Module manager already initialized");
        return -1;
    }

    g_ctx = kmod_new(NULL, NULL);
    if (g_ctx == NULL) {
        set_error("Failed to create kmod context");
        return -1;
    }

    int ret = kmod_load_resources(g_ctx);
    if (ret < 0) {
        set_error("Failed to load kmod resources: %s", strerror(-ret));
        kmod_unref(g_ctx);
        g_ctx = NULL;
        return -1;
    }

    memset(g_error_msg, 0, sizeof(g_error_msg));
    return 0;
}

// Cleanup module manager
void mm_cleanup(void) {
    if (g_ctx != NULL) {
        kmod_unref(g_ctx);
        g_ctx = NULL;
    }
}

// Load a module
int mm_load(const char *path, const char *params) {
    if (g_ctx == NULL) {
        set_error("Module manager not initialized");
        return -1;
    }

    if (path == NULL || path[0] == '\0') {
        set_error("Invalid module path");
        return -1;
    }

    struct kmod_module *mod = NULL;
    int ret = kmod_module_new_from_path(g_ctx, path, &mod);
    if (ret < 0) {
        set_error("Module lookup failed: %s", strerror(-ret));
        return -1;
    }

    // Try to insert module (load it)
    ret = kmod_module_insert_module(mod, 0, params);
    if (ret < 0) {
        set_error("Failed to insert module: %s", strerror(-ret));
        kmod_module_unref(mod);
        return -1;
    }

    kmod_module_unref(mod);
    return 0;
}

// Unload a module
int mm_unload(const char *name) {
    if (g_ctx == NULL) {
        set_error("Module manager not initialized");
        return -1;
    }

    if (name == NULL || name[0] == '\0') {
        set_error("Invalid module name");
        return -1;
    }

    struct kmod_module *mod = NULL;
    int ret = kmod_module_new_from_name(g_ctx, name, &mod);
    if (ret < 0) {
        set_error("Module lookup failed: %s", strerror(-ret));
        return -1;
    }

    // Try to remove module (unload it)
    ret = kmod_module_remove_module(mod, 0);
    if (ret < 0) {
        set_error("Failed to remove module: %s", strerror(-ret));
        kmod_module_unref(mod);
        return -1;
    }

    kmod_module_unref(mod);
    return 0;
}

// Check if module is loaded
int mm_is_loaded(const char *name) {
    if (g_ctx == NULL) {
        set_error("Module manager not initialized");
        return 0;
    }

    if (name == NULL || name[0] == '\0') {
        set_error("Invalid module name");
        return 0;
    }

    struct kmod_module *mod = NULL;
    int ret = kmod_module_new_from_name(g_ctx, name, &mod);
    if (ret < 0) {
        return 0;  // Not found = not loaded
    }

    int initstate = kmod_module_get_initstate(mod);
    kmod_module_unref(mod);

    return (initstate == 1) ? 1 : 0;  // 1 = MODULE_STATE_LIVE
}

// List loaded modules
int mm_list(ModuleInfo **list, int *count) {
    if (g_ctx == NULL) {
        set_error("Module manager not initialized");
        return -1;
    }

    if (list == NULL || count == NULL) {
        set_error("Invalid output parameters");
        return -1;
    }

    struct kmod_list *mod_list = NULL;
    struct kmod_list *itr = NULL;
    int ret;

    // Get list of loaded modules
    ret = kmod_module_new_from_loaded(g_ctx, &mod_list);
    if (ret < 0) {
        set_error("Failed to get loaded modules: %s", strerror(-ret));
        return -1;
    }

    // Count modules
    int mod_count = 0;
    kmod_list_foreach(itr, mod_list) {
        mod_count++;
    }

    if (mod_count == 0) {
        *list = NULL;
        *count = 0;
        kmod_module_unref_list(mod_list);
        return 0;
    }

    // Allocate output array
    ModuleInfo *info = malloc(mod_count * sizeof(ModuleInfo));
    if (info == NULL) {
        set_error("Memory allocation failed");
        kmod_module_unref_list(mod_list);
        return -1;
    }

    // Fill module info
    int i = 0;
    itr = NULL;
    kmod_list_foreach(itr, mod_list) {
        struct kmod_module *mod = kmod_module_get_module(itr);

        strncpy(info[i].name, kmod_module_get_name(mod), sizeof(info[i].name) - 1);
        info[i].name[sizeof(info[i].name) - 1] = '\0';

        info[i].size = kmod_module_get_size(mod);
        info[i].refcount = kmod_module_get_refcnt(mod);

        // Get dependencies
        struct kmod_list *dep_list = kmod_module_get_dependencies(mod);
        if (dep_list) {
            struct kmod_list *dep_itr;
            int dep_idx = 0;
            kmod_list_foreach(dep_itr, dep_list) {
                struct kmod_module *dep_mod = kmod_module_get_module(dep_itr);
                if (dep_idx > 0) strncat(info[i].deps, ", ", sizeof(info[i].deps) - 1);
                strncat(info[i].deps, kmod_module_get_name(dep_mod), sizeof(info[i].deps) - 1);
                dep_idx++;
            }
        } else {
            info[i].deps[0] = '\0';
        }

        // Get state via initstate
        int initstate = kmod_module_get_initstate(mod);
        if (initstate == 1) {
            strcpy(info[i].state, "Live");
        } else if (initstate == 2) {
            strcpy(info[i].state, "Builtin");
        } else {
            strcpy(info[i].state, "Unknown");
        }

        i++;
    }

    *list = info;
    *count = mod_count;
    kmod_module_unref_list(mod_list);
    return 0;
}

// Get last error message
const char *mm_last_error(void) {
    return (g_error_msg[0] != '\0') ? g_error_msg : "No error";
}

int mm_get_info_module(ModuleInfo *info, struct kmod_module *mod, char *name_of_module) {
    if (!info || !mod) {
        return -1;
    }

    // name
    strncpy(info->name, name_of_module, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';

    // size + refcount
    info->size = kmod_module_get_size(mod);
    info->refcount = kmod_module_get_refcnt(mod);

    // dependencies
    info->deps[0] = '\0'; // IMPORTANT: clear buffer

    struct kmod_list *dep_list = kmod_module_get_dependencies(mod);
    if (dep_list) {
        struct kmod_list *itr;
        int first = 1;

        kmod_list_foreach(itr, dep_list) {
            struct kmod_module *dep_mod = kmod_module_get_module(itr);
            const char *dep_name = kmod_module_get_name(dep_mod);

            if (!first) {
                strncat(info->deps, ", ", sizeof(info->deps) - strlen(info->deps) - 1);
            }
            strncat(info->deps, dep_name, sizeof(info->deps) - strlen(info->deps) - 1);

            first = 0;
        }

        kmod_module_unref_list(dep_list);
    }

    // state
    int initstate = kmod_module_get_initstate(mod);
    switch (initstate) {
        case KMOD_MODULE_LIVE:
            strcpy(info->state, "Live");
            break;
        case KMOD_MODULE_BUILTIN:
            strcpy(info->state, "Builtin");
            break;
        default:
            strcpy(info->state, "Unknown");
            break;
    }

    return 0;
}

// Get detailed info about a specific module by name
int mm_get_info(const char *name, ModuleInfo *info) {
    if (!g_ctx) {
        set_error("Module manager not initialized");
        return -1;
    }

    if (!name || !info) {
        set_error("Invalid arguments");
        return -1;
    }

    struct kmod_module *mod = NULL;
    int ret = kmod_module_new_from_name(g_ctx, name, &mod);
    if (ret < 0 || !mod) {
        set_error("Module not found: %s", name);
        return -1;
    }

    // Use existing function to fill info
    ret = mm_get_info_module(info, mod, (char *)name);
    kmod_module_unref(mod);
    return ret;
}
