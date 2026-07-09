Building on Windows 
=================
(WIP, not yet building without some hacking)
(Works good on WSL! please refer to [BUILDING_LINUX](BUILDING_LINUX.md) for more)

<<<<<<< Updated upstream
Preliminary Requirements
--------------------------
=======
This guide explains how to build and run DISSCO on Windows using MSVC, CMake, Ninja, Qt, vcpkg, Git for Windows, and LilyPond.
>>>>>>> Stashed changes

The following are *necessary* to anything:

- git,
- A C/C++ compiler (like MSVC `cl` or `clang-cl`), and
- cmake >= 3.25
<!-- - muparser >= 2.X -->

To compile LASS:

<<<<<<< Updated upstream
- [libsndfile](https://libsndfile.github.io/libsndfile/#download) >= 1.0,

To compile CMOD:
- muparser >= 2.X, and
- pugixml >= 1.15

> *Note*: the source code for both of these parsers are included in this repo, so you don't need to download them.

To compile LASSIE:

- [Qt](https://www.qt.io/development/download-qt-installer-oss) >= 6.8
=======
Before You Start
----------------

Use **x64 Native Tools Command Prompt for VS** for all build commands.

This guide uses **CMD syntax**. PowerShell uses different syntax for line continuation and environment variables.

The commands below assume these default paths:
>>>>>>> Stashed changes

> *Note*: there are several ways to get these packages onto your system. This doc will go through a fairly simple one.

<<<<<<< Updated upstream
Installing requirements and recommendations:
--------------------------------------------

When installing Qt via the graphical installer, you'll have the option to install additional tools. These include **CMake** and a **compiler**. Please choose a compiler for your computer's architecture (x86-64 for Intel and ARM for Arm).
=======
You may install these tools anywhere. If your paths are different, update the `set` commands and use your paths consistently.
>>>>>>> Stashed changes

Please be aware that the compiler architecture must match the architecture of the Qt version you install. You will get an error at link-time if they differ.

Installing DISSCO
-----------------
Just `git clone` this repo; explicitly:

    git clone https://github.com/cmp-illinois/DISSCO-2.2.0.git

<<<<<<< Updated upstream
Building
--------
=======
- Git for Windows
- Visual Studio or Visual Studio Build Tools with **Desktop development with C++**
- CMake 3.25 or newer
- Ninja
- Qt 6.8 or newer, **MSVC 64-bit** build
- vcpkg
- LilyPond for Windows

The repository already includes:

- muParser
- pugixml

Install Git
-----------

Using winget:

```cmd
winget install --id Git.Git -e
```

Git for Windows usually provides Unix-style tools such as `rm` and `mv` in:

```text
C:\Program Files\Git\usr\bin
```

Add this directory to User PATH later.

Install Visual Studio C++ Tools
-------------------------------

Install Visual Studio Community or Visual Studio Build Tools.

Select:

```text
Desktop development with C++
```

Make sure these components are installed:

- MSVC x64/x86 build tools
- Windows SDK
- C++ CMake tools for Windows

Open **x64 Native Tools Command Prompt for VS** and check:

```cmd
cl
```

You should see Microsoft C/C++ compiler information.

Install Qt
----------

Install Qt using the Qt Online Installer.

Select a Qt version with the MSVC 64-bit component, for example:

```text
Qt 6.x -> MSVC 2022 64-bit
```

Set `QT_ROOT` to your Qt MSVC 64-bit directory:

```cmd
set "QT_ROOT=C:\Qt\6.11.1\msvc2022_64"
```

If your Qt is installed somewhere else, change the path.

Install vcpkg Dependencies
--------------------------

Choose where to install vcpkg. This guide assumes `C:\dev\vcpkg`:

```cmd
cd /d C:\dev
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
```

Set `VCPKG_ROOT`:

```cmd
set "VCPKG_ROOT=C:\dev\vcpkg"
```

Install required packages:

```cmd
cd /d "%VCPKG_ROOT%"
vcpkg install libsndfile:x64-windows
vcpkg install dirent:x64-windows
```

Optional: save `VCPKG_ROOT` as a user environment variable:

```cmd
setx VCPKG_ROOT "%VCPKG_ROOT%"
```

Verify:

```cmd
dir "%VCPKG_ROOT%\installed\x64-windows\include\sndfile.h"
dir "%VCPKG_ROOT%\installed\x64-windows\include\dirent.h"
dir "%VCPKG_ROOT%\installed\x64-windows\lib\*sndfile*.lib"
```

Install LilyPond
----------------

Install or extract LilyPond for Windows.

Set `LILYPOND_BIN` to the directory containing `lilypond.exe`:

```cmd
set "LILYPOND_BIN=C:\Program Files\LilyPond\usr\bin"
```

If LilyPond is installed somewhere else, change the path.

Test:

```cmd
"%LILYPOND_BIN%\lilypond.exe" --version
```

Update PATH
-----------

Add these directories to your **User PATH**:

```text
%QT_ROOT%\bin
%LILYPOND_BIN%
C:\Program Files\Git\usr\bin
```

You can edit PATH from:

```text
System Properties -> Environment Variables -> User variables -> Path -> Edit
```

After editing PATH, close and reopen all terminals.

Verify:

```cmd
where qmake
where lilypond
where rm
where mv
lilypond --version
```

Clone DISSCO
------------

Choose where to put the source code. This guide assumes `C:\dev\DISSCO-2.2.0`:

```cmd
cd /d C:\dev
git clone https://github.com/cmp-illinois/DISSCO-2.2.0.git
cd DISSCO-2.2.0
```

Set `DISSCO_ROOT`:

```cmd
set "DISSCO_ROOT=C:\dev\DISSCO-2.2.0"
```

If the repository uses submodules, run:

```cmd
git submodule update --init --recursive
```

Configure with CMake
--------------------

Open **x64 Native Tools Command Prompt for VS**.

Set your paths. Change them if your installation paths are different:

```cmd
set "DISSCO_ROOT=C:\dev\DISSCO-2.2.0"
set "VCPKG_ROOT=C:\dev\vcpkg"
set "QT_ROOT=C:\Qt\6.11.1\msvc2022_64"
```

Go to the source directory:

```cmd
cd /d "%DISSCO_ROOT%"
```

Remove any old build directory:

```cmd
rmdir /s /q build
```

Configure:

```cmd
cmake -S . -B build -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DQt6_DIR="%QT_ROOT%/lib/cmake/Qt6" ^
  -DCMAKE_PREFIX_PATH="%QT_ROOT%" ^
  -DBUILD_SHARED_LIBS=OFF ^
  -DCMAKE_CXX_FLAGS="/EHsc /DNOMINMAX /DNOGDI /DMUPARSER_STATIC /D_CRT_SECURE_NO_WARNINGS" ^
  -DSNDFILE_INCLUDE_DIR="%VCPKG_ROOT%/installed/x64-windows/include" ^
  -DSNDFILE_LIBRARY="%VCPKG_ROOT%/installed/x64-windows/lib/sndfile.lib"
```

Notes:

- Qt, vcpkg packages, and MSVC must all be 64-bit.
- `MUPARSER_STATIC` is required for static muParser builds on MSVC.
- `NOMINMAX` avoids Windows `min` and `max` macro conflicts.
- `NOGDI` avoids Windows GDI name conflicts.
- `_CRT_SECURE_NO_WARNINGS` suppresses MSVC warnings for older C library calls.
- `/EHsc` enables standard C++ exception handling.

Build
-----
>>>>>>> Stashed changes

From the project's root directory (by default: `C:\path\to\DISSCO-X.X.X`), configure the build:

    cmake -S . -B build -G Ninja

If you installed `libsndfile` somewhere outside the standard locations, point CMake at it:

    cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="C:/path/to/libsndfile"

Then, from `build/`, do

    cmake --build .

<<<<<<< Updated upstream
to build.

By running this command in `build`, one generates a so-called *out-of-source* (OOS) build. The alternative, an in-source build, is heavily discouraged (including [by the CMake maintainers](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Getting%20Started.html#directory-structure)), and the root `CMakeLists.txt` reflects this distaste. The rationale is that OOS builds minimize clutter and collect all build files in one directory, whereas in-source builds put build files virtually everywhere. (This is bad.)
=======
Run LASSIE
----------
>>>>>>> Stashed changes

From `build`, you can clean `build` using `cmake --build . --target clean`. Alternatively, you can do `rmdir /s build` from outside of `build`.

Building a release installer
----------------------------
The release pipeline produces an NSIS installer `.exe` that bundles `LASSIE.exe`, `CMOD.exe`, the Qt runtime, and writes registry entries for the `.dissco` file association.

Extra requirement: **NSIS** (Nullsoft Scriptable Install System).

    choco install nsis

Then from Developer PowerShell at the project root:

    cmake -S . -B build -G Ninja `
        -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    cmake --build build --target package

<<<<<<< Updated upstream
This produces `build/DISSCO-<version>-Windows.exe`. Under the hood:

- CPack's NSIS generator wraps the install tree into a single-file installer.
- `windeployqt` runs at install time against the installed `LASSIE.exe` and copies the Qt DLLs + platform plugins next to it.
- The installer writes `HKLM\Software\Classes\.dissco` registry entries so `.dissco` files open LASSIE on double-click immediately after install (system-wide). If the file-association branch (`claude/sad-hermann-4b5591`) is merged, LASSIE.exe will additionally self-register per-user (HKCU) on first launch — both layers describe the same ProgID.
- The installer also installs a Start Menu shortcut and an optional desktop shortcut.
=======
Run CMOD Manually
-----------------
>>>>>>> Stashed changes

The icon (`packaging/windows/LASSIE.ico`) is a placeholder; regenerate it from updated artwork via `packaging/windows/make-ico.sh`.

<<<<<<< Updated upstream
**Code signing:** unsigned `.exe` installers trigger Windows SmartScreen warnings on download. Production releases should be signed with an EV or OV code-signing certificate. The GitHub Actions release workflow has a clean place to wire signtool in once the certificate is provisioned.
=======
```cmd
"%DISSCO_ROOT%\build\CMOD\CMOD.exe" "<path-to-project>\your_file.dissco"
```

Generate Score Output
---------------------

Before generating score output, make sure the project folder has a `ScoreFiles` directory:

```cmd
mkdir "<path-to-project>\ScoreFiles"
```

Then open the `.dissco` project in LASSIE and run score output.

If LilyPond is correctly installed and PATH is set, CMOD should be able to generate score files.

Cleaning and Rebuilding
-----------------------

Clean and rebuild:

```cmd
cd /d "%DISSCO_ROOT%"
cmake --build build --clean-first
```

Fully reset the build directory:

```cmd
cd /d "%DISSCO_ROOT%"
rmdir /s /q build
```

Then rerun the CMake configure command and build again.
>>>>>>> Stashed changes
