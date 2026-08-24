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

set +e
macdeploy_output="$(
    "${MACDEPLOYQT}" \
        "${APP_BUNDLE}" \
        "-executable=${cmod}" \
        -always-overwrite 2>&1
)"
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

otool_dependencies() {
    "${OTOOL}" -L "$1" | tail -n +2
}

# The first otool line is the inspected file's own path (normally under
# /Users/runner in CI), not a dependency, so exclude it from validation.
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
