#!/bin/bash

# Name und Pfade
APP_NAME="wxApp"
APP_BUNDLE="${APP_NAME}Bundle.app"
CONTENTS="$APP_BUNDLE/Contents"
MACOS="$CONTENTS/MacOS"
FRAMEWORKS="$CONTENTS/Frameworks"
RESOURCES="$CONTENTS/Resources"
PLIST="$CONTENTS/Info.plist"
BINARY="./wxApp"

# Clean-up vorherige Version
echo "🧹 Bereinige vorherige App..."
rm -rf "$APP_BUNDLE"

# Ordnerstruktur erstellen
echo "📦 Erstelle App-Bundle-Struktur..."
mkdir -p "$MACOS"
mkdir -p "$FRAMEWORKS"
mkdir -p "$RESOURCES"

# Binary hinein kopieren
echo "🚀 Kopiere Binary..."
cp "$BINARY" "$MACOS/"

# Frameworks (z. B. OpenCV und andere dylibs) kopieren
echo "📚 Kopiere Frameworks..."
# Passe diesen Pfad ggf. an – du kannst auch alle benötigten libs manuell reinschmeißen
cp /opt/homebrew/opt/opencv/lib/*.dylib "$FRAMEWORKS/" 2>/dev/null || echo "⚠️ Keine OpenCV-dylibs gefunden."
cp /opt/homebrew/lib/libsharpyuv*.dylib "$FRAMEWORKS/" 2>/dev/null || echo "⚠️ Keine libsharpyuv gefunden."

# Info.plist erstellen
echo "📝 Erstelle Info.plist..."
cat > "$PLIST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" \
 "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>${APP_NAME}</string>
    <key>CFBundleIdentifier</key>
    <string>com.yourname.${APP_NAME}</string>
    <key>CFBundleName</key>
    <string>${APP_NAME}</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

# App signieren (Ad-hoc)
echo "🔏 Signiere App (ad-hoc)..."
codesign --force --deep --sign - "$APP_BUNDLE" || echo "❌ Signierung fehlgeschlagen. Vielleicht brauchst du sudo?"

echo "✅ App-Bundle erstellt unter: $APP_BUNDLE"
