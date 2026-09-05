#!/usr/bin/env bash
# Verify that a release DMG passes the same Gatekeeper checks users encounter.

set -euo pipefail

if [[ $# -ne 1 || ! -f "$1" ]]; then
    echo "Usage: $0 /path/to/DISSCO-<version>-Darwin.dmg" >&2
    exit 2
fi

dmg_path="$1"
temp_root="${TMPDIR:-/tmp}"
mount_dir="$(mktemp -d "${temp_root%/}/dissco-dmg.XXXXXX")"
mounted=0
cleanup() {
    if [[ ${mounted} -eq 1 ]]; then
        /usr/bin/hdiutil detach "${mount_dir}" >/dev/null 2>&1 || true
    fi
    rmdir "${mount_dir}" >/dev/null 2>&1 || true
}
trap cleanup EXIT

/usr/bin/hdiutil verify "${dmg_path}"
/usr/bin/hdiutil attach \
    -readonly \
    -nobrowse \
    -mountpoint "${mount_dir}" \
    "${dmg_path}" >/dev/null
mounted=1

shopt -s nullglob
apps=("${mount_dir}"/*.app)
if [[ ${#apps[@]} -ne 1 ]]; then
    echo "Expected exactly one app in ${dmg_path}; found ${#apps[@]}." >&2
    exit 1
fi
app_path="${apps[0]}"

/usr/bin/codesign --verify --deep --strict --verbose=2 "${app_path}"
/usr/sbin/spctl --assess --type execute --verbose=4 "${app_path}"

signature_info="$(
    /usr/bin/codesign --display --verbose=4 "${app_path}" 2>&1
)"
if ! grep -Fq 'Authority=Developer ID Application:' <<<"${signature_info}"; then
    echo "${app_path} is not signed with a Developer ID Application certificate." >&2
    exit 1
fi
if ! grep -Eq '^TeamIdentifier=[A-Z0-9]+$' <<<"${signature_info}"; then
    echo "${app_path} does not have an Apple Team ID." >&2
    exit 1
fi

/usr/bin/codesign --verify --verbose=2 "${dmg_path}"
/usr/bin/xcrun stapler validate "${dmg_path}"
/usr/sbin/spctl \
    --assess \
    --type open \
    --context context:primary-signature \
    --verbose=4 \
    "${dmg_path}"

echo "Validated signed and notarized DMG: ${dmg_path}"
