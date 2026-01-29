#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/module_manager.h"

// ============================================================================
// CLI Commands
// ============================================================================

void print_usage(const char *prog) {
    printf("Kernel Module Manager (KMM) - Load/Unload kernel modules easily\n\n");
    printf("Usage: %s <command> [options]\n\n", prog);
    printf("Commands:\n");
    printf("  load   <path> [params]   Load kernel module from .ko file\n");
    printf("  unload <name>            Unload kernel module by name\n");
    printf("  list                     List all loaded modules\n");
    printf("  check  <name>            Check if module is loaded\n");
    printf("  help                     Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s load ./my_driver.ko\n", prog);
    printf("  %s load ./my_driver.ko param1=value1\n", prog);
    printf("  %s unload my_driver\n", prog);
    printf("  %s list\n", prog);
    printf("  %s check my_driver\n", prog);
}

int cmd_load(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "[Error] load: missing module path\n");
        fprintf(stderr, "Usage: kmm load <path> [params]\n");
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
        return 1;
    }

    printf("[OK] Module loaded successfully\n");
    return 0;
}

int cmd_unload(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "[Error] unload: missing module name\n");
        fprintf(stderr, "Usage: kmm unload <name>\n");
        return 1;
    }

    const char *name = argv[2];

    printf("[INFO] Unloading module: %s\n", name);

    int ret = mm_unload(name);
    if (ret != 0) {
        fprintf(stderr, "[Error] Failed to unload module: %s\n", mm_last_error());
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
        fprintf(stderr, "Usage: kmm check <name>\n");
        return 1;
    }

    const char *name = argv[2];

    int ret = mm_is_loaded(name);
    if (ret < 0) {
        fprintf(stderr, "[Error] Failed to check module: %s\n", mm_last_error());
        return 1;
    }

    if (ret == 1) {
        printf("[OK] Module '%s' is loaded\n", name);
        return 0;
    } else {
        printf("[INFO] Module '%s' is NOT loaded\n", name);
        return 1;
    }
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
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
        print_usage(argv[0]);
        ret = 0;
    }
    else {
        fprintf(stderr, "[Error] Unknown command: %s\n", cmd);
        print_usage(argv[0]);
        ret = 1;
    }

    // Cleanup
    mm_cleanup();
    return ret;
}
