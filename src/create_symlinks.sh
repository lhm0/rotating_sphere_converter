#!/bin/bash

# Wechsel in das Frameworks-Verzeichnis
cd "$(dirname "$0")/wxAppBundle.app/Contents/Frameworks" || exit 1

echo "Erstelle symbolische Links für wxWidgets-Bibliotheken..."

for full in libwx_*3.2.*.dylib; do
  if [[ $full =~ (libwx_.*-3\.2)\..*\.dylib ]]; then
    short="${BASH_REMATCH[1]}.dylib"
    if [[ ! -e $short ]]; then
      ln -s "$full" "$short"
      echo "  → Link erstellt: $short → $full"
    else
      echo "  ✔ Link existiert schon: $short"
    fi
  fi
done

echo "Fertig!"
