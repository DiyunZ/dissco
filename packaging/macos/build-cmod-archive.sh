#!/usr/bin/env bash
# Build a relocatable CMOD command-line archive for macOS.

set -euo pipefail

: "${OUTPUT_DIR:?OUTPUT_DIR not set}"
: "${SOURCE_DIR:?SOURCE_DIR not set}"
: "${DISSCO_VERSION:?DISSCO_VERSION not set}"
: "${CMOD_BINARY:?CMOD_BINARY not set}"

mkdir -p "${OUTPUT_DIR}"

if [[ -n "${DYLIBBUNDLER:-}" ]]; then
    dylibbundler="${DYLIBBUNDLER}"
else
    dylibbundler="$(command -v dylibbundler || true)"
fi
if [[ -z "${dylibbundler}" || ! -x "${dylibbundler}" ]]; then
    echo "dylibbundler is required to make the CMOD macOS archive" >&2
    exit 1
fi

if [[ -n "${OTOOL:-}" ]]; then
    otool="${OTOOL}"
else
    otool="$(command -v otool || true)"
fi
if [[ -z "${otool}" || ! -x "${otool}" ]]; then
    echo "otool is required to validate the CMOD macOS archive" >&2
    exit 1
fi

arch="$(uname -m)"
package_name="CMOD-${DISSCO_VERSION}-Darwin-${arch}"
archive_path="${OUTPUT_DIR}/${package_name}.tar.gz"
staging_root="$(mktemp -d "${OUTPUT_DIR}/.cmod-macos.XXXXXX")"
trap 'rm -rf "${staging_root}"' EXIT
package_root="${staging_root}/${package_name}"

mkdir -p "${package_root}/bin" "${package_root}/lib"
install -m 755 "${CMOD_BINARY}" "${package_root}/bin/cmod"
install -m 644 "${SOURCE_DIR}/LICENSE" "${package_root}/LICENSE"

"${dylibbundler}" \
    -od \
    -b \
    -x "${package_root}/bin/cmod" \
    -d "${package_root}/lib/" \
    -p "@executable_path/../lib/"

cat > "${package_root}/README.txt" <<EOF
CMOD ${DISSCO_VERSION} for macOS (${arch})
==========================================

Run:
  ./bin/cmod --help
  ./bin/cmod /path/to/project.dissco

CMOD and its linked audio libraries are included. LilyPond is not included;
install it separately and add it to PATH for score or PDF output. Audio
synthesis does not require LilyPond.
EOF

otool_dependencies() {
    "${otool}" -L "$1" | tail -n +2
}

dependency_report="$(otool_dependencies "${package_root}/bin/cmod")"
while IFS= read -r dylib; do
    dependency_report+=$'\n'
    dependency_report+="$(otool_dependencies "${dylib}")"
done < <(find "${package_root}/lib" -type f -name '*.dylib' -print)

if grep -Eq '(/opt/homebrew|/usr/local|/opt/local|/Users)/' \
        <<<"${dependency_report}"; then
    echo "CMOD archive still contains package-manager dylib paths:" >&2
    echo "${dependency_report}" >&2
    exit 1
fi

clean_path="/usr/bin:/bin"
env -i HOME="${HOME:-/tmp}" PATH="${clean_path}" \
    "${package_root}/bin/cmod" --help | grep -Fq "Usage:"

rm -f -- "${archive_path}"
tar -czf "${archive_path}" -C "${staging_root}" "${package_name}"
echo "Built ${archive_path}"
