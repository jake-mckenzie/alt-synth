#!/bin/sh
set -eu
cd "$(dirname "$0")/.."

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 VERSION PLATFORM" >&2
    exit 1
fi
version=$1
platform=$2
if ! printf '%s\n' "$version" | grep -Eq '^v[0-9]+\.[0-9]+\.[0-9]+$'; then
    echo "Invalid release version: $version" >&2
    exit 1
fi

# Use a fresh staging directory so packages contain only release files.
stage=$(mktemp -d)
stage=$(cd "$stage" && pwd)
trap 'rm -rf -- "$stage"' EXIT
package="$stage/alt-synth"
archive="$stage/alt-synth_${version}_${platform}"
mkdir -p "$package/licenses" "$package/assets" "$package/docs/images" dist
cp README.md "$package/"
cp docs/playing-guide.md "$package/docs/"
cp docs/images/piano-view.png docs/images/qwerty-view.png "$package/docs/images/"
cp assets/OFL.txt "$package/assets/"
cp .deps/raylib/LICENSE "$package/licenses/raylib-LICENSE.txt"

case "$platform" in
    linux_x86_64)
        cp bin/alt-synth "$package/"
        tar -czf "$archive.tar.gz" -C "$stage" alt-synth
        ;;
    windows_x86_64)
        cp bin/alt-synth.exe "$package/"
        (cd "$stage" && zip -q -r "$archive.zip" alt-synth)
        ;;
    macos_arm64)
        app="$package/alt-synth.app/Contents"
        mkdir -p "$app/MacOS"
        cp bin/alt-synth "$app/MacOS/"
        cat > "$app/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key><string>alt-synth</string>
    <key>CFBundleIdentifier</key><string>com.github.jake-mckenzie.alt-synth</string>
    <key>CFBundleName</key><string>alt-synth</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>CFBundleShortVersionString</key><string>${version#v}</string>
    <key>CFBundleVersion</key><string>${version#v}</string>
    <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
PLIST
        plutil -lint "$app/Info.plist"
        ditto -c -k --keepParent "$package" "$archive.zip"
        ;;
    *) echo "Unknown release platform: $platform" >&2; exit 1 ;;
esac
mv -- "$archive".* dist/
