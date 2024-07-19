# Flare Game Engine

## Building

Run `/scripts/Windows_GenerateProjects.bat` to generate a Visual Studio 2022 solution.

### Dependecies

1. Vulkan SDK 1.3. `VulkanSDK/<version>/bin/` must be added to `PATH`. `VULKAN_SDK` environment variable must be set to `/VulkanSDK/<version>/`
2. Assimp is not included in the solution, and is built separatly as a static lib.
    Run `/Flare/vendor/assimp/GenerateAssimpProject.bat` to generate a Visual Studio solution and build both Debug & Release configurations.
3. All other dependecies, are included as submodules and are a part of the Visual Studio solution.

### Sandbox

Run `GenerateProject.bat` inside `/Sandbox/`, to generate a solution.

## Running

The editor can be started using Visual Studio, or from the command line by running the `.exe` from the `FlareEditor` folder.

### Command line arguments

`--project=<path>` - A project to open on editor startup. Must be a path to .flareproj file.

`--api=<name>` - API to use for rendering. Only `--api=vulkan` is supported, which is also a default.

`--vulkan-debug` - enables Vulkan validation layers and generation of debug names for Vulkan objects. Disabled by default.

`--device=<type>` - specify a type of GPU device to use for rendering. Can be one of two: `integrated` or `discrete`. `discrete` is the default.
