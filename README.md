# Linux Kernel Module Manager (KMM)

A lightweight, user-friendly C utility for managing Linux kernel modules. Load, unload, list, and check kernel modules with a simple CLI interface powered by the **libkmod** library.

## Version

Current version:
    **v0.0.3**

last edits:
- get info of specific module added


## Features

-  **List loaded modules** - View all currently loaded kernel modules with details
-  **Load modules** - Insert kernel modules (.ko files) with optional parameters
-  **Unload modules** - Remove kernel modules by name
-  **Check module status** - Quickly verify if a specific module is loaded
-  **Dependency tracking** - View module dependencies and reference counts
-  **Error handling** - Clear, user-friendly error messages
-  **Lightweight** - Minimal dependencies, fast execution
-  **Generic** - Works with any kernel module, not hardware-specific

## Requirements

- Linux system with kernel module support
- GCC compiler (or compatible C compiler)
- `libkmod-dev` package installed
- `pkg-config` for build configuration
- `make` for building

## Installation

### 1. Install Dependencies (Debian/Ubuntu)

```bash
sudo apt-get update
sudo apt-get install libkmod-dev pkg-config build-essential
```

### 2. Clone and Build

```bash
cd /path/to/KMM
make
```

The compiled binary will be at: `./kmm`

### 3. Create log file for logger

```
mkdir ~/.local/share/KMM
touch ~/.local/shate/KMM/log_data.log
```

### 4. Verify Build

```bash
./kmm help
```

## Usage

### Commands

#### **help** - Show usage information
```bash
./kmm help
```

#### **list** - List all loaded kernel modules
```bash
./kmm list
```

Shows module name, size (KB), reference count, and dependencies.

**Example output:**
```
=== Loaded Kernel Modules ===

Name                       Size (KB)   RefCnt Dependencies
----                        --------   ------ ------------
vboxnetadp                        28        0 vboxdrv
vboxnetflt                        32        0 vboxdrv
vboxdrv                          680        2 -
snd_seq_dummy                     12        0 soundcore, snd, snd_timer
```

#### **check** - Check if a module is loaded
```bash
./kmm check <module_name>
```

**Examples:**
```bash
./kmm check vboxdrv        # Loaded
./kmm check nonexistent    # Not loaded
```

#### **load** - Load a kernel module (requires sudo)
```bash
sudo ./kmm load <path_to_module.ko> [parameters]
```

**Examples:**
```bash
sudo ./kmm load ./my_driver.ko
sudo ./kmm load ./my_driver.ko param1=value1 param2=value2
```

#### **unload** - Unload a kernel module (requires sudo)
```bash
sudo ./kmm unload <module_name>
```

**Example:**
```bash
sudo ./kmm unload my_driver
```

## Architecture

KMM uses a **3-layer architecture** for clean separation of concerns:

```
┌─────────────────────────────────────┐
│   CLI Layer (main.c)                │
│   - Command dispatcher              │
│   - User input parsing              │
│   - Output formatting               │
└──────────┬──────────────────────────┘
           │
┌──────────▼──────────────────────────┐
│   API Layer (module_manager.h/.c)   │
│   - Core operations                 │
│   - Error handling                  │
│   - Context management              │
└──────────┬──────────────────────────┘
           │
┌──────────▼──────────────────────────┐
│   libkmod (kernel interface)        │
│   - Kernel module database          │
│   - Load/unload via kernel          │
└─────────────────────────────────────┘
```

### Core Components

**module_manager.h** - Public API header defining:
- `ModuleInfo` struct with module metadata
- 6 core functions: `mm_init`, `mm_cleanup`, `mm_load`, `mm_unload`, `mm_list`, `mm_is_loaded`, `mm_last_error`

**module_manager.c** - Implementation using libkmod:
- Manages kernel module context (`kmod_ctx`)
- Centralizes error handling with `g_error_msg` buffer
- Iterates loaded modules via `/proc/modules`
- Handles module loading/unloading through kernel syscalls

**main.c** - CLI interface:
- Command dispatcher (load, unload, list, check, help)
- Pretty-printed output formatting
- Argument parsing and validation

## Building

### Standard Build
```bash
make          # Clean, build, and link
```

### Clean Build Artifacts
```bash
make clean
```

### Check Dependencies
```bash
make check-deps
```

### Build Options
```makefile
# View available make targets
make help
```

## Development

### Extending KMM

To add new functionality:

1. **Add API function** in `include/module_manager.h`
2. **Implement function** in `src/module_manager.c` using libkmod
3. **Add CLI command handler** in `src/main.c`
4. **Rebuild** with `make`

### Example: Adding a new operation

```c
// module_manager.h
int mm_get_size(const char *name);

// module_manager.c
int mm_get_size(const char *name) {
    // Implementation using libkmod
    struct kmod_module *mod;
    kmod_module_new_from_name(g_ctx, name, &mod);
    long size = kmod_module_get_size(mod);
    kmod_module_unref(mod);
    return size;
}

// main.c
int cmd_size(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: kmm size <name>\n");
        return 1;
    }
    long size = mm_get_size(argv[2]);
    printf("Module size: %ld bytes\n", size);
    return 0;
}
```

## Known Limitations

- **Requires root** to load/unload modules (use `sudo`)
- **Only lists loaded modules** (built-in modules may not appear)
- **No module parameter persistence** across reboots
- **Cannot load built-in modules** (already in kernel)

## Future Enhancements

- [ ] Logging to file (`~/.local/share/KMM/kmm.log`)
- [ ] Dependency resolution (prevent unload if depended upon)
- [ ] Module search in standard directories (`/lib/modules/...`)
- [ ] Module reload functionality
- [ ] JSON output format
- [ ] Configuration file support

## Troubleshooting

### Build Fails: "libkmod.h not found"
Install libkmod development headers:
```bash
sudo apt-get install libkmod-dev
```

### Permission Denied When Loading
Use `sudo`:
```bash
sudo ./kmm load ./my_module.ko
```

### Module Not Found
Verify module file path is correct and file exists:
```bash
ls -la /path/to/module.ko
```

### Module Already Loaded
Use `./kmm check` to verify current state:
```bash
./kmm check my_module
```

## File Structure

```
KMM/
├── README.md                 # This file
├── Makefile                  # Build configuration
├── include/
│   └── module_manager.h      # Public API header (55 lines)
├── src/
│   ├── main.c               # CLI dispatcher (176 lines)
│   └── module_manager.c     # libkmod implementation (230+ lines)
└── build/                   # Build artifacts (generated)
    ├── main.o
    ├── module_manager.o
    └── kmm                  # Final executable
```

## Performance

KMM is optimized for speed and minimal memory usage:
- Single-pass module listing (O(n) complexity)
- Lazy context initialization
- Immediate resource cleanup
- Typical execution time: <50ms for list operations

## License

This project is distributed under the [GPL-3.0 License](./LICENSE).

## Contributing

To contribute improvements:

1. Test thoroughly on your system
2. Ensure no memory leaks (`valgrind` recommended)
3. Keep code style consistent
4. Update README if adding features

## Testing

### Quick Test Suite
```bash
# Test help
./kmm help

# List modules
./kmm list | head -5

# Check specific modules
./kmm check vboxdrv
./kmm check nonexistent

# Load test (requires .ko file and sudo)
# sudo ./kmm load ./test_module.ko
```

## Support

For issues or questions:
1. Check the [Troubleshooting](#troubleshooting) section
2. Verify libkmod is installed: `pkg-config --modversion libkmod`
3. Check kernel module support: `ls /lib/modules/$(uname -r)/`

---

**Last Updated:** January 2026  
**Kernel Module Manager (KMM)** - Manage kernel modules with ease
