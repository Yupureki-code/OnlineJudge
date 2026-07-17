#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${OJ_BUILD_DIR:-"$ROOT/build-review"}
DIST_DIR=${OJ_DIST_DIR:-"$ROOT/dist/phase8"}

cmake --build "$BUILD_DIR" --target oj_server oj_admin judge_service -j"${OJ_BUILD_JOBS:-4}"

rm -rf "$DIST_DIR/bin" "$DIST_DIR/lib"
mkdir -p "$DIST_DIR/bin" "$DIST_DIR/lib"
for binary in oj_server oj_admin judge_service; do
    install -m 0555 "$ROOT/output/$binary" "$DIST_DIR/bin/$binary"
done

copy_dependency() {
    local path=$1
    case "$path" in
        /usr/local/lib/*|*/libodb-*.so*|*/libctemplate.so*)
            cp -L "$path" "$DIST_DIR/lib/$(basename "$path")"
            ;;
    esac
}

for binary in "$DIST_DIR"/bin/*; do
    while read -r name arrow path rest; do
        if [[ "$arrow" == "=>" && "$path" == /* ]]; then
            copy_dependency "$path"
        elif [[ "$name" == /* ]]; then
            copy_dependency "$name"
        fi
    done < <(ldd "$binary")
done

cat > "$DIST_DIR/metadata.env" <<EOF
architecture=$(uname -m)
kernel=$(uname -s)
build_os=ubuntu:24.04
generated_at=$(date -u +%Y-%m-%dT%H:%M:%SZ)
EOF

(
    cd "$DIST_DIR"
    sha256sum bin/* lib/* > SHA256SUMS
)

for binary in "$DIST_DIR"/bin/*; do
    LD_LIBRARY_PATH="$DIST_DIR/lib" ldd "$binary"
    if LD_LIBRARY_PATH="$DIST_DIR/lib" ldd "$binary" | grep -q "not found"; then
        echo "unresolved dependency in $binary" >&2
        exit 1
    fi
done
