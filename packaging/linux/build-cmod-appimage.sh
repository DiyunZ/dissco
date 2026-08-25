#!/usr/bin/env bash
# Build a CMOD-only AppImage from the compiled CMOD executable.

set -euo pipefail

: "${APPDIR:?APPDIR not set}"
: "${OUTPUT_DIR:?OUTPUT_DIR not set}"
: "${SOURCE_DIR:?SOURCE_DIR not set}"
: "${DISSCO_VERSION:?DISSCO_VERSION not set}"
: "${CMOD_BINARY:?CMOD_BINARY not set}"

resolved_appdir="$(realpath -m "${APPDIR}")"
resolved_output="$(realpath -m "${OUTPUT_DIR}")"
resolved_source="$(realpath -m "${SOURCE_DIR}")"
resolved_appdir_parent="$(dirname -- "${resolved_appdir}")"
if [[ "${resolved_appdir}" == "/" ||
      "${resolved_appdir}" == "${resolved_output}" ||
      "${resolved_appdir}" == "${resolved_source}" ||
      "${resolved_appdir_parent}" != "${resolved_output}" ]]; then
    echo "Refusing unsafe CMOD AppDir path: ${resolved_appdir}" >&2
    exit 1
fi

rm -rf -- "${resolved_appdir}"
mkdir -p "${resolved_appdir}/usr/bin"
mkdir -p "${resolved_appdir}/usr/share/applications"
mkdir -p "${resolved_appdir}/usr/share/icons/hicolor/256x256/apps"
mkdir -p "${resolved_output}"

install -m 755 "${CMOD_BINARY}" "${resolved_appdir}/usr/bin/cmod"
install -m 644 "${SOURCE_DIR}/packaging/linux/CMOD.desktop" \
    "${resolved_appdir}/usr/share/applications/CMOD.desktop"
install -m 644 "${SOURCE_DIR}/packaging/linux/LASSIE.png" \
    "${resolved_appdir}/usr/share/icons/hicolor/256x256/apps/CMOD.png"

arch="$(uname -m)"
tools_dir="${resolved_output}/.linuxdeploy"
mkdir -p "${tools_dir}"

fetch() {
    local url="$1" destination="$2"
    if [[ ! -x "${destination}" ]]; then
        echo "Downloading ${url}"
        curl -fSL -o "${destination}" "${url}"
        chmod +x "${destination}"
    fi
}

if [[ -n "${LINUXDEPLOY:-}" ]]; then
    linuxdeploy="${LINUXDEPLOY}"
else
    linuxdeploy="${tools_dir}/linuxdeploy-${arch}.AppImage"
    fetch \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${arch}.AppImage" \
        "${linuxdeploy}"
fi

output_name="CMOD-${DISSCO_VERSION}-Linux-${arch}.AppImage"
output_path="${resolved_output}/${output_name}"
rm -f -- "${output_path}"

# GitHub-hosted runners do not expose FUSE to nested AppImages.
export APPIMAGE_EXTRACT_AND_RUN=1

cd "${resolved_output}"
OUTPUT="${output_name}" \
"${linuxdeploy}" \
    --appdir "${resolved_appdir}" \
    --executable "${resolved_appdir}/usr/bin/cmod" \
    --desktop-file "${resolved_appdir}/usr/share/applications/CMOD.desktop" \
    --icon-file "${resolved_appdir}/usr/share/icons/hicolor/256x256/apps/CMOD.png" \
    --output appimage

if [[ ! -x "${output_path}" ]]; then
    echo "linuxdeploy did not create ${output_path}" >&2
    exit 1
fi

"${output_path}" --help | grep -Fq "Usage:"
echo "Built ${output_path}"
