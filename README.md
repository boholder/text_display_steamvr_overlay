# Text Display SteamVR Overlay

> [!WARNING]
> Work in progress, not ready for use.

> [!WARNING]
> This project is made with Vulkan 1.3+ in mind, thus it couldn't run on pre-2016 GPU like GTX 7xx, even if the GPU is supported by SteamVR.

An OpenVR overlay that displays text sent from other software.

## Build

> [!IMPORTANT]
> You need [Vulkan SDK](https://vulkan.lunarg.com/) and a C++ compiler like [GCC](https://gcc.gnu.org/), [Clang](https://clang.llvm.org/), [MSVC](https://visualstudio.microsoft.com/downloads/?q=build+tools), etc. to build this project, make sure you have them installed.

> [!IMPORTANT]
> This project uses CMake to copy all the required resources from `resources` to the build directory, if you're using custom build environment you need to copy them manually.

Fetch dependencies through [scripts/update_third_libs.sh](scripts/update_third_libs.sh):

```sh
scripts/update_third_libs.sh y
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
- Libraries that this project depends on:
  - [OpenVR](https://github.com/ValveSoftware/openvr) is licensed under [BSD 3-Clause License](licenses/OPENVR_LICENSE).
  - [spdlog](https://github.com/gabime/spdlog) is licensed under [MIT License](licenses/SPDLOG_LICENSE).
  - [ImGui](https://github.com/ocornut/imgui) is licensed under [MIT License](licenses/IMGUI_LICENSE).
  - [SDL](https://github.com/libsdl-org/SDL) is licensed under [zlib License](licenses/SDL_LICENSE).
  - [glm](https://github.com/g-truc/glm) is licensed under [The Happy Bunny License or MIT License](licenses/GLM_LICENSE).
