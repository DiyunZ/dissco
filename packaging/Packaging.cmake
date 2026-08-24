# Release packaging for DISSCO (LASSIE GUI + CMOD CLI).
#
# Produces:
#   - macOS: a .dmg containing lassie.app (with cmod embedded in
#     Contents/MacOS/) via CPack's DragNDrop generator. Qt frameworks are
#     bundled by an install(CODE) step that invokes macdeployqt.
#   - Windows: an NSIS installer .exe (lassie.exe + cmod.exe + Qt DLLs)
#     via CPack's NSIS generator. Qt DLLs are bundled by an install(CODE)
#     step that invokes windeployqt. The installer writes registry entries
#     under HKLM\Software\Classes for the .dissco extension; these
#     complement the per-user HKCU writes done by LASSIE's runtime
#     file-association helper (see LASSIE/src/win/file_association.cpp,
#     which lives on the file-association branch).
#   - Linux: an AppImage built by packaging/linux/build-appimage.sh, wired
#     up as a CMake custom target named `appimage` (not part of CPack,
#     since AppImage's tooling lives outside it).
#
# Trigger:
#   cmake --build build --target package        # macOS DMG / Windows .exe
#   cmake --build build --target appimage       # Linux AppImage
#   cmake --build build --target cmod-package   # CMOD-only platform package

set(CPACK_PACKAGE_NAME "DISSCO")
set(CPACK_PACKAGE_VENDOR "DISSCO Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "DISSCO: composition and sound design environment (LASSIE GUI + CMOD).")
set(CPACK_PACKAGE_VERSION_MAJOR "${DISSCO_VER_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${DISSCO_VER_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${DISSCO_VER_PATCH}")
set(CPACK_PACKAGE_VERSION "${DISSCO_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME "DISSCO-${DISSCO_VERSION}-${CMAKE_SYSTEM_NAME}")

if(APPLE)
    # Locate every tool used by the install-time app fixup. Missing optional
    # packaging tools are reported when `package` runs, not during a normal
    # developer build.
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    get_filename_component(_qt_root_dir "${_qt_bin_dir}" DIRECTORY)
    set(MACDEPLOYQT_EXECUTABLE "${_qt_bin_dir}/macdeployqt")
    find_program(DYLIBBUNDLER_EXECUTABLE NAMES dylibbundler)
    find_program(OTOOL_EXECUTABLE NAMES otool)
    find_program(BASH_EXECUTABLE NAMES bash REQUIRED)

    # Both app executables link libsndfile, while macdeployqt only deploys Qt
    # dependencies. Run the dedicated fixup after lassie and cmod are installed
    # so it can bundle third-party dylibs, deploy Qt, and smoke-test embedded
    # CMOD from the staged app before CPack creates the DMG.
    install(CODE "
        message(STATUS \"Fixing up \${CMAKE_INSTALL_PREFIX}/lassie.app\")
        execute_process(
            COMMAND \"${CMAKE_COMMAND}\" -E env
                \"APP_BUNDLE=\${CMAKE_INSTALL_PREFIX}/lassie.app\"
                \"DYLIBBUNDLER=${DYLIBBUNDLER_EXECUTABLE}\"
                \"MACDEPLOYQT=${MACDEPLOYQT_EXECUTABLE}\"
                \"OTOOL=${OTOOL_EXECUTABLE}\"
                \"QT_ROOT=${_qt_root_dir}\"
                \"${BASH_EXECUTABLE}\"
                \"${CMAKE_SOURCE_DIR}/packaging/macos/fixup-dissco-app.sh\"
            RESULT_VARIABLE _fixup_result
            OUTPUT_VARIABLE _fixup_out
            ERROR_VARIABLE _fixup_err
        )
        if(_fixup_out)
            message(\"\${_fixup_out}\")
        endif()
        if(_fixup_err)
            message(\"\${_fixup_err}\")
        endif()
        if(NOT _fixup_result EQUAL 0)
            message(FATAL_ERROR
                \"macOS app fixup failed with exit code \${_fixup_result}\")
        endif()
    " COMPONENT Runtime)

    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "DISSCO ${DISSCO_VERSION}")
    set(CPACK_DMG_FORMAT "UDZO")
    if(EXISTS "${CMAKE_SOURCE_DIR}/packaging/macos/LASSIE.icns")
        set(CPACK_DMG_VOLUME_ICON "${CMAKE_SOURCE_DIR}/packaging/macos/LASSIE.icns")
    endif()
endif()

if(WIN32)
    # windeployqt: walks lassie.exe's PE imports and copies the matching Qt
    # DLLs (and platform plugins, styles, etc.) into the same directory.
    # We pull the Qt6::windeployqt imported target's path so the path
    # tracks whatever Qt the build is configured against, not whatever
    # happens to be on PATH.
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt windeployqt6
        HINTS "${_qt_bin_dir}"
        REQUIRED
    )

    # Run windeployqt against the installed lassie.exe. Crucially this is a
    # separate invocation from any build-time windeployqt (e.g. the one in
    # LASSIE/CMakeLists.txt's POST_BUILD command, which targets the build
    # dir for dev convenience): the installer needs DLLs in CMAKE_INSTALL_PREFIX,
    # not the build dir.
    install(CODE "
        message(STATUS \"Running windeployqt on \${CMAKE_INSTALL_PREFIX}/bin/lassie.exe\")
        execute_process(
            COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                --verbose 0
                --no-compiler-runtime
                --no-translations
                --no-system-d3d-compiler
                --no-quick-import
                \"\${CMAKE_INSTALL_PREFIX}/bin/lassie.exe\"
            RESULT_VARIABLE _wdq_result
        )
        if(NOT _wdq_result EQUAL 0)
            message(FATAL_ERROR \"windeployqt failed with exit code \${_wdq_result}\")
        endif()
    " COMPONENT Runtime)

    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "DISSCO ${DISSCO_VERSION}")
    set(CPACK_NSIS_DISPLAY_NAME "DISSCO ${DISSCO_VERSION}")
    set(CPACK_NSIS_HELP_LINK "https://github.com/cmp-illinois/DISSCO")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/cmp-illinois/DISSCO")
    set(CPACK_NSIS_CONTACT "https://github.com/cmp-illinois/DISSCO/issues")
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "DISSCO ${DISSCO_VERSION}")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")

    # Start Menu and desktop shortcuts pointing at lassie.exe.
    set(CPACK_PACKAGE_EXECUTABLES "lassie;LASSIE")
    set(CPACK_CREATE_DESKTOP_LINKS "lassie")

    if(EXISTS "${CMAKE_SOURCE_DIR}/packaging/windows/LASSIE.ico")
        # NSIS wants Windows-native backslashes in these paths.
        file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/packaging/windows/LASSIE.ico" _nsis_icon)
        string(REPLACE "\\" "\\\\" _nsis_icon "${_nsis_icon}")
        set(CPACK_NSIS_MUI_ICON "${_nsis_icon}")
        set(CPACK_NSIS_MUI_UNIICON "${_nsis_icon}")
        set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\lassie.exe")
    endif()

    # File-association registry entries.
    #
    # Three layers describe the same `DISSCO.Project` ProgID, picked up by
    # Explorer in priority order:
    #
    #   1. Installer (this block) — HKLM, system-wide, written here at
    #      install time. Makes .dissco files openable immediately, no
    #      LASSIE launch required, benefits other users on the machine.
    #   2. Runtime helper — HKCU, per-user, written by LASSIE on first
    #      launch (LASSIE/src/win/file_association.cpp on the
    #      file-association branch). Naturally overrides HKLM per user;
    #      "last Release LASSIE launched wins" between installs / dev
    #      builds.
    #   3. Manual fallback — packaging/windows/dissco-association.reg.
    #      Documents the exact HKCU layout (1) and (2) write, for users
    #      doing scripted/offline self-install. Not wired into the build.
    #
    # The HKLM commands below are kept inline (not generated from the
    # .reg) so they can use $INSTDIR; the .reg file remains the canonical
    # human-readable reference.
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        WriteRegStr HKLM 'Software\\\\Classes\\\\.dissco' '' 'DISSCO.Project'
        WriteRegStr HKLM 'Software\\\\Classes\\\\DISSCO.Project' '' 'DISSCO Project'
        WriteRegStr HKLM 'Software\\\\Classes\\\\DISSCO.Project' 'FriendlyTypeName' 'DISSCO Project'
        WriteRegStr HKLM 'Software\\\\Classes\\\\DISSCO.Project\\\\DefaultIcon' '' '$INSTDIR\\\\bin\\\\lassie.exe,0'
        WriteRegStr HKLM 'Software\\\\Classes\\\\DISSCO.Project\\\\shell\\\\open\\\\command' '' '\\\"$INSTDIR\\\\bin\\\\lassie.exe\\\" \\\"%1\\\"'
        System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
    ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        DeleteRegKey HKLM 'Software\\\\Classes\\\\DISSCO.Project'
        DeleteRegKey HKLM 'Software\\\\Classes\\\\.dissco'
        System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
    ")
endif()

if(UNIX AND NOT APPLE)
    # AppImage is produced by linuxdeploy + linuxdeploy-plugin-qt operating on
    # a staged AppDir. We delegate to a shell script so the same recipe can be
    # run by hand or by CI without needing CMake. The script consumes an
    # already-`cmake --install`-ed tree.
    add_custom_target(appimage
        COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}"
                --prefix "${CMAKE_BINARY_DIR}/AppDir/usr"
                --component Runtime
        COMMAND ${CMAKE_COMMAND} -E env
                DISSCO_VERSION=${DISSCO_VERSION}
                APPDIR=${CMAKE_BINARY_DIR}/AppDir
                OUTPUT_DIR=${CMAKE_BINARY_DIR}
                SOURCE_DIR=${CMAKE_SOURCE_DIR}
                bash "${CMAKE_SOURCE_DIR}/packaging/linux/build-appimage.sh"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building DISSCO AppImage"
        VERBATIM
    )
endif()

# CMOD is also distributed as a standalone command-line package. These targets
# stage directly from the CMOD executable and leave the combined DISSCO Runtime
# component untouched.
if(UNIX AND NOT APPLE)
    add_custom_target(cmod-package
        COMMAND ${CMAKE_COMMAND} -E env
                "DISSCO_VERSION=${DISSCO_VERSION}"
                "APPDIR=${CMAKE_BINARY_DIR}/CmodAppDir"
                "OUTPUT_DIR=${CMAKE_BINARY_DIR}"
                "SOURCE_DIR=${CMAKE_SOURCE_DIR}"
                "CMOD_BINARY=$<TARGET_FILE:CMOD>"
                bash "${CMAKE_SOURCE_DIR}/packaging/linux/build-cmod-appimage.sh"
        DEPENDS CMOD
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Building standalone CMOD AppImage"
        VERBATIM
    )
elseif(APPLE)
    add_custom_target(cmod-package
        COMMAND ${CMAKE_COMMAND} -E env
                "DISSCO_VERSION=${DISSCO_VERSION}"
                "OUTPUT_DIR=${CMAKE_BINARY_DIR}"
                "SOURCE_DIR=${CMAKE_SOURCE_DIR}"
                "CMOD_BINARY=$<TARGET_FILE:CMOD>"
                bash "${CMAKE_SOURCE_DIR}/packaging/macos/build-cmod-archive.sh"
        DEPENDS CMOD
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Building standalone CMOD macOS archive"
        VERBATIM
    )
elseif(WIN32)
    find_program(POWERSHELL_EXECUTABLE
        NAMES pwsh powershell
        REQUIRED
    )
    add_custom_target(cmod-package
        COMMAND "${POWERSHELL_EXECUTABLE}"
                -NoProfile
                -ExecutionPolicy Bypass
                -File "${CMAKE_SOURCE_DIR}/Make-Portable-for-Windows.ps1"
                -ProjectRoot "${CMAKE_SOURCE_DIR}"
                -BuildDirectory "${CMAKE_BINARY_DIR}"
                -OutputDirectory "${CMAKE_BINARY_DIR}"
                -PackageName "CMOD-${DISSCO_VERSION}-Windows-x64"
                -CmodOnly
                -SkipBuild
                -SkipTests
        DEPENDS CMOD
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Building standalone CMOD Windows archive"
        VERBATIM
    )
endif()

include(CPack)
