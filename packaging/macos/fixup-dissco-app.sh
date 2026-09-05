#!/usr/bin/env bash
# Make the staged DISSCO app self-contained and verify its embedded CMOD.

set -euo pipefail

: "${APP_BUNDLE:?APP_BUNDLE not set}"
: "${DYLIBBUNDLER:?DYLIBBUNDLER not set}"
: "${MACDEPLOYQT:?MACDEPLOYQT not set}"
: "${OTOOL:?OTOOL not set}"
: "${QT_ROOT:?QT_ROOT not set}"

if [[ ! -d "${APP_BUNDLE}" ]]; then
    echo "DISSCO app bundle was not found: ${APP_BUNDLE}" >&2
    exit 1
fi
for tool in "${DYLIBBUNDLER}" "${MACDEPLOYQT}" "${OTOOL}"; do
    if [[ ! -x "${tool}" ]]; then
        echo "Required macOS packaging tool was not found: ${tool}" >&2
        exit 1
    fi
done

lassie="${APP_BUNDLE}/Contents/MacOS/lassie"
cmod="${APP_BUNDLE}/Contents/MacOS/cmod"
frameworks="${APP_BUNDLE}/Contents/Frameworks"
for executable in "${lassie}" "${cmod}"; do
    if [[ ! -x "${executable}" ]]; then
        echo "DISSCO app executable was not found: ${executable}" >&2
        exit 1
    fi
done
mkdir -p "${frameworks}"

# LASSIE and CMOD both link libsndfile. Keep Qt for macdeployqt, and use
# dylibbundler only for the remaining third-party dylibs and their closure.
"${DYLIBBUNDLER}" \
    -od \
    -b \
    -x "${lassie}" \
    -x "${cmod}" \
    -d "${frameworks}/" \
    -p "@executable_path/../Frameworks/" \
    -i "${QT_ROOT}" \
    -i "/System/Library"

macdeployqt_args=(
    "${APP_BUNDLE}"
    "-executable=${cmod}"
    -always-overwrite
)
if [[ -n "${DISSCO_CODESIGN_IDENTITY:-}" ]]; then
    macdeployqt_args+=(
        "-sign-for-notarization=${DISSCO_CODESIGN_IDENTITY}"
    )
fi

set +e
macdeploy_output="$("${MACDEPLOYQT}" "${macdeployqt_args[@]}" 2>&1)"
macdeploy_result=$?
set -e

# macdeployqt reports these optional, absent Qt frameworks as errors even
# though it succeeds. Preserve every other line so genuine failures remain.
filtered_macdeploy_output="$(
    printf '%s\n' "${macdeploy_output}" |
        sed -E \
            -e '/ERROR: Cannot resolve rpath "@rpath\/Qt(Pdf|Svg|VirtualKeyboard[A-Za-z]*)\.framework/d' \
            -e '/ERROR:  using QList/d'
)"
if [[ -n "${filtered_macdeploy_output}" ]]; then
    printf '%s\n' "${filtered_macdeploy_output}"
fi
if [[ ${macdeploy_result} -ne 0 ]]; then
    echo "macdeployqt failed with exit code ${macdeploy_result}" >&2
    exit "${macdeploy_result}"
fi

if [[ -n "${DISSCO_CODESIGN_IDENTITY:-}" ]]; then
    /usr/bin/codesign --verify --deep --strict --verbose=2 "${APP_BUNDLE}"
    signature_info="$(
        /usr/bin/codesign --display --verbose=4 "${APP_BUNDLE}" 2>&1
    )"
    if ! grep -Fq 'Authority=Developer ID Application:' \
            <<<"${signature_info}"; then
        echo "DISSCO app is not signed with a Developer ID Application certificate." >&2
        exit 1
    fi
    if ! grep -Eq '^TeamIdentifier=[A-Z0-9]+$' <<<"${signature_info}"; then
        echo "DISSCO app signature does not contain an Apple Team ID." >&2
        exit 1
    fi
fi

otool_dependencies() {
    if [[ "$1" == *.dylib ]]; then
        # For a dylib, otool prints its LC_ID_DYLIB before its dependencies.
        # That install name is metadata for future linkers, not a path loaded
        # by the packaged application.
        "${OTOOL}" -L "$1" | tail -n +3
    else
        "${OTOOL}" -L "$1" | tail -n +2
    fi
}

# Exclude otool's heading and each dylib's install name so this report contains
# only paths the packaged executables and libraries will load at runtime.
dependency_report="$(otool_dependencies "${lassie}")"
dependency_report+=$'\n'
dependency_report+="$(otool_dependencies "${cmod}")"
while IFS= read -r dylib; do
    dependency_report+=$'\n'
    dependency_report+="$(otool_dependencies "${dylib}")"
done < <(find "${frameworks}" -type f -name '*.dylib' -print)

if grep -Eq '(/opt/homebrew|/usr/local|/opt/local|/Users)/' \
        <<<"${dependency_report}"; then
    echo "DISSCO app still contains build-machine library paths:" >&2
    echo "${dependency_report}" >&2
    exit 1
fi

env -i HOME="${HOME:-/tmp}" PATH="/usr/bin:/bin" \
    "${cmod}" --help | grep -Fq "Usage:"

echo "Validated ${APP_BUNDLE}"
