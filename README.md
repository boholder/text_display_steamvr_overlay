# Text Display SteamVR Overlay

An OpenVR overlay that displays text sent from other software.

## Build

> [!IMPORTANT]
> You need [Vulkan SDK](https://vulkan.lunarg.com/) and a C++ compiler like [GCC](https://gcc.gnu.org/), [Clang](https://clang.llvm.org/), [MSVC](https://visualstudio.microsoft.com/downloads/?q=build+tools), etc. to build this project, make sure you have them installed.

> [!IMPORTANT]
> This project uses CMake to copy all the required resources from `resources` to the build directory, if you're using custom build environment you need to copy them manually.

Fetch dependencies through `git submodule`:

```sh
git submodule init && git submodule update
```

Directly build it in Visual Studio or JetBrains Clion with [CMake presets](CMakePresets.json).
If you prefer to build it in the command line without a preset or an IDE:

```sh
cmake -B build
cmake --build build
```

Run the binary file from the build directory.

## Credit

- This project is based on Nyabsi's [SteamVR Overlay Example](https://github.com/Nyabsi/text_display_steamvr_overlay), which is licensed under [Mozilla Public License Version 2.0](licenses/NYABSI_LICENSE).
- The rest of the project is licensed under [Mozilla Public License Version 2.0](LICENSE).
- Libraries that this project depends on are listed in [.gitmodules](.gitmodules), we respect their licenses.
