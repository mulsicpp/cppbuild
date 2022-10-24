# cppbuild <!-- omit in toc -->

**cppbuild** is a buildsystem for C/C++, that allows for compilation on Windows and Linux using the MSVC and G++ compilers.
<hr>

## Index <!-- omit in toc -->
- [How it works](#how-it-works)
  - [**Analysis phase**](#analysis-phase)
  - [**Build phase**](#build-phase)
- [Setup](#setup)
  - [On Windows](#on-windows)
  - [On Linux](#on-linux)
- [Creating a project](#creating-a-project)
  - [Commands](#commands)
    - [**Set the project output**](#set-the-project-output)
    - [**Excluding source files**](#excluding-source-files)
    - [**Linking a static library**](#linking-a-static-library)
    - [**Adding include paths**](#adding-include-paths)
    - [**Adding macros**](#adding-macros)
    - [**Adding library paths**](#adding-library-paths)
    - [**Setting the language standard**](#setting-the-language-standard)
    - [**Require another project**](#require-another-project)
- [Building a project](#building-a-project)
  - [Flags:](#flags)
    - [**Run application**](#run-application)
    - [**Force building**](#force-building)
    - [**Set project path**](#set-project-path)
    - [**Set architecture**](#set-architecture)
    - [**Set configuration**](#set-configuration)

## How it works

The builder basically works in two phases:

### **Analysis phase**
In this phase the builder gathers all the information needed for building. This includes things like the type of output(application/library), name of the output, the C++ standard as well as command line flags (--run, --force, ...). Source file discovery and the loading of the heaader dependencies also occurs in this phase. The builder stores this information and later uses it in the build phase.

### **Build phase**
In this phase the actual build process starts. Every source file gets compiled into a binary file (*.o). This step may be skipped, if the source file itself or none of its included header files have been changed. After this all the binary files are linked to an executable or a library, depending on the output type.


## Setup

### On Windows
On `Windows` you need to have installed CMake as well as Visual Studio. The version is not relevant, but newer versions are recommended. After cloning the repository you can compile `cppbuild` using the Batch script 'compile.bat' in the repository root directory. You will now get the builder as an executable file `cppbuild.exe`. You can move this file wherever you like and you can then add its directory to `PATH` enviornment variable to call `cppbuild` from anywhere. Before you can build your C++ projects, you need to call the following command:
```
cppbuild --setup
```

You then have to enter the path to your MSVC compiler and to your Windows Kits directory. It should look something like this:
```
MSCV path: C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.31.31103
Windows Kits path: C:\Program Files (x86)\Windows Kits\10
```

### On Linux
On *Linux* you need to have installed `Make` and `G++`. You have to call *make* to get the executable file *cppbuild*. You can move this file wherever you like. To be able to call `cppbuild` from anywhere, you need to insert the following line in your `'~/.bashrc'` file:
```sh
export PATH=$PATH:<path_to_cppbuild_directory>
```
## Creating a project

*cppbuild* allows you to relatively free when it comes to organising your project. However every `cppbuild` project needs to have a `cppbuild.script` file in its root directory. This build script contains all the commands to build your project.

### Commands

#### **Set the project output**
```
output <type> <filename>
```

<type\> can be:
- app: to generate an application
- lib: to generate a static library

<filename\> is the name of the generated file.

**Info:** In the future dynamic library support will be added.

#### **Excluding source files**
```
ignore <filename>
```
By default all source files (*.c, *.cpp), that are inside the project directory, will be included in the project. Use this command to exclude a file.

#### **Linking a static library**
```
lib <filename>
```

Adds the library as linker argument for the project. The library has to have a specific format ('lib\*.a' on Linux and \*.lib on Windows (**See:** [format fuctions](#format-functions))) or it may not be recognized by the linker. If the output type of the project is a static library, all libraries are ignored.

Alternatively you can use this command:

```
linklib <libname>
```

If you use this way the library has to be in one of the library paths(**See:** [adding library paths](#adding-library-paths)) and only the name should be passed (for example `linklib bla` for `bla.lib/libbla.a`). These libraries will also not be included in the update checks. Therefore this command should be used for system libraries.

#### **Adding include paths**
```
incpath <path>
```

Adds the path to the additional include paths for compilation.

#### **Adding macros**
```
define <macro> [<value>]
```

Adds the macro with an optional value as a compiler definition.

#### **Adding library paths**
```
libpath <path>
```

Adds the path to the library paths of the compiler.

#### **Setting the language standard**
```
std <std>
```
Sets the C/C++ standard ('c++11', 'c++14', 'c++17', ...).

#### **Require another project**
```
require <project_path>
```
Requires a different project. This means, that the specified project needs to be build before the current one. The other project is not linked to or used by the current project. This command simply ensures, that the other project is up to date.

## Building a project
To build a project simply call the following command in the root directory of the project:
```console
cppbuild
```

### Flags:
You can add flags to achieve different functionalities over the command line.

#### **Run application**
```console
--run
```
If the output of the build is an application, it runs after it is build.

#### **Force building**
```console
--force
```
All the checks for compilation and linking get ignored. Compilation and linking always happens.

#### **Set project path**
```console
--path=<path>
```
Sets the path to the project to be build. If this flag isn't specified, the current directory is used.

#### **Set architecture**
```console
--arch=<arch>
```
Sets the architecture to be build for. 'x64' and 'x86' are allowed. If this flag isn't specified, the architecture of the current machine is used.

#### **Set configuration**
```console
--config=<config>
```
Sets the configuration to be build for. 'release' and 'debug' are allowed. If this flag isn't specified, 'release' is assumed.
