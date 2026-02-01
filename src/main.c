#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/module_manager.h"
#include "../include/logger.h"

// ============================================================================
// CLI Commands
// ============================================================================

void print_usage(const char *prog) {
    printf("Kernel Module Manager (KMM) - Load/Unload kernel modules easily\n\n");
    printf("Usage: %s <command> [options]\n\n", prog);
    printf("Commands:\n");
    printf("  load       <path> [params]   Load kernel module from .ko file\n");
    printf("  unload     <name>            Unload kernel module by name\n");
    printf("  list                         List all loaded modules\n");
    printf("  check      <name>            Check if module is loaded\n");
    printf("  get_info   <name>            Get detailed info about a module\n");
    printf("  help                         Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s load ./my_driver.ko\n", prog);
    printf("  %s load ./my_driver.ko param1=value1\n", prog);
    printf("  %s unload my_driver\n", prog);
    printf("  %s list\n", prog);
    printf("  %s check my_driver\n", prog);
    printf("  %s get_info my_driver\n", prog);
}

int cmd_load(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "[Error] load: missing module path\n");
        fprintf(stderr, "Usage: kmm load <path> [params]\n");
        KMM_log("Failed to load module: missing path", NULL);
        return 1;
    }

    const char *path = argv[2];
    const char *params = (argc > 3) ? argv[3] : NULL;

    printf("[INFO] Loading module from: %s\n", path);
    if (params) {
        printf("[INFO] Parameters: %s\n", params);
    }

    int ret = mm_load(path, params);
    if (ret != 0) {
        fprintf(stderr, "[Error] Failed to load module: %s\n", mm_last_error());

        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Failed to load module %s", mm_last_error());
        KMM_log(buffer, NULL);

        return 1;
    }

    printf("[OK] Module loaded successfully\n");
    KMM_log("module loaded successfully", NULL);
    return 0;
}

int cmd_unload(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "[Error] unload: missing module name\n");
        KMM_log("Failed to unload module: missing name", NULL);
        fprintf(stderr, "Usage: kmm unload <name>\n");
        return 1;
    }

    const char *name = argv[2];

    printf("[INFO] Unloading module: %s\n", name);
    KMM_log("Attempting to unload module", NULL);

    int ret = mm_unload(name);
    if (ret != 0) {
        fprintf(stderr, "[Error] Failed to unload module: %s\n", mm_last_error());

        char buffer [256];
        snprintf(buffer, sizeof(buffer), "Failed to unload module %s", mm_last_error());
        KMM_log(buffer, NULL);

        return 1;
    }

    printf("[OK] Module unloaded successfully\n");
    return 0;
}

int cmd_list(void) {
    printf("\n=== Loaded Kernel Modules ===\n\n");
    printf("%-25s %10s %8s %s\n", "Name", "Size (KB)", "RefCnt", "Dependencies");
    printf("%-25s %10s %8s %s\n", "----", "--------", "------", "------------");

    ModuleInfo *list = NULL;
    int count = 0;

    int ret = mm_list(&list, &count);
    if (ret != 0) {
        fprintf(stderr, "[Error] Failed to list modules: %s\n", mm_last_error());
        
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Failed to list modules: %s", mm_last_error());
        KMM_log(buffer, NULL);

        return 1;
    }

    if (count == 0) {
        printf("(No modules loaded)\n");
        return 0;
    }

    for (int i = 0; i < count; i++) {
        const char *deps = list[i].deps[0] != '\0' ? list[i].deps : "-";
        printf("%-25s %10u %8d %s\n", 
               list[i].name, 
               list[i].size / 1024, 
               list[i].refcount, 
               deps);
    }

    printf("\nTotal: %d module(s)\n", count);
    free(list);
    return 0;
}

int cmd_check(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "[Error] check: missing module name\n");
        KMM_log("Failed to check module: missing name", NULL);
        fprintf(stderr, "Usage: kmm check <name>\n");
        return 1;
    }

    const char *name = argv[2];

    int ret = mm_is_loaded(name);
    if (ret < 0) {
        fprintf(stderr, "[Error] Failed to check module: %s\n", mm_last_error());
        KMM_log("Failed to check module", mm_last_error());
        return 1;
    }

    if (ret == 1) {
        printf("[OK] Module '%s' is loaded\n", name);
        KMM_log("Module is loaded", name);
        return 0;
    } else {
        printf("[INFO] Module '%s' is NOT loaded\n", name);
        KMM_log("Module is not loaded", name);
        return 1;
    }
}

int cmd_get_info_module(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "[Error] get_info: missing module name\n");
        KMM_log("Failed to get module info: missing name", NULL);
        fprintf(stderr, "Usage: kmm get_info <name>\n");
        return 1;
    }

    const char *name = argv[2];
    ModuleInfo info = {0};

    struct kmod_ctx *ctx = kmod_new(NULL, NULL);
    if (!ctx) {
        fprintf(stderr, "[Error] Failed to create kmod context\n");
        KMM_log("Failed to create kmod context", NULL);
        return 1;
    }

    struct kmod_module *mod = NULL;
    int ret = kmod_module_new_from_name(ctx, name, &mod);
    if (ret < 0 || !mod) {
        fprintf(stderr, "[Error] Module '%s' not found\n", name);
        KMM_log("Module not found", name);
        kmod_unref(ctx);
        return 1;
    }

    // Fill in module info
    strncpy(info.name, kmod_module_get_name(mod), sizeof(info.name) - 1);
    info.size = kmod_module_get_size(mod);
    info.refcount = kmod_module_get_refcnt(mod);
    
    struct kmod_list *deps = kmod_module_get_dependencies(mod);
    if (deps) {
        struct kmod_list *d;
        int dep_idx = 0;
        kmod_list_foreach(d, deps) {
            struct kmod_module *dep_mod = kmod_module_get_module(d);
            if (dep_idx > 0) strncat(info.deps, ", ", sizeof(info.deps) - 1);
            strncat(info.deps, kmod_module_get_name(dep_mod), sizeof(info.deps) - 1);
            dep_idx++;
        }
    }

    int initstate = kmod_module_get_initstate(mod);
    if (initstate == 1) {
        strcpy(info.state, "Live");
    } else if (initstate == 2) {
        strcpy(info.state, "Builtin");
    } else {
        strcpy(info.state, "Unknown");
    }

    printf("\n=== Module Info: %s ===\n", info.name);
    printf("Size: %u KB\n", info.size / 1024);
    printf("RefCount: %d\n", info.refcount);
    printf("Dependencies: %s\n", info.deps[0] != '\0' ? info.deps : "-");
    printf("State: %s\n", info.state);

    kmod_module_unref(mod);
    kmod_unref(ctx);
    return 0;
}


// ============================================================================
// Main
// ============================================================================

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *cmd = argv[1];
    int ret = 0;

    // Initialize module manager
    if (mm_init() != 0) {
        fprintf(stderr, "[Error] Failed to initialize module manager: %s\n", mm_last_error());

        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Failed to initialize module manager: %s", mm_last_error());
        KMM_log(buffer, NULL);

        return 1;
    }

    // Dispatch commands
    if (strcmp(cmd, "load") == 0) {
        ret = cmd_load(argc, argv);
    }
    else if (strcmp(cmd, "unload") == 0) {
        ret = cmd_unload(argc, argv);
    }
    else if (strcmp(cmd, "list") == 0) {
        ret = cmd_list();
    }
    else if (strcmp(cmd, "check") == 0) {
        ret = cmd_check(argc, argv);
    }
    else if (strcmp(cmd, "get_info") == 0) {
        ret = cmd_get_info_module(argc, argv);
    }
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
        print_usage(argv[0]);
        ret = 0;
    }
    else {
        fprintf(stderr, "[Error] Unknown command: %s\n", cmd);
        KMM_log("Unknown command attempted", cmd);
        print_usage(argv[0]);
        ret = 1;
    }

    // Cleanup
    mm_cleanup();
    return ret;
}
