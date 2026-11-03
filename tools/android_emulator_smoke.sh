#!/usr/bin/env bash
# Single process for all statements: the emulator-runner splits an inline
# script by newline and executes each line with /bin/sh -c, breaking if/fi
# blocks and Python heredocs even after Android successfully boots.
set -euo pipefail
adb install -r apk/app-debug.apk
adb shell am start -n com.koolkatze.fm1offline/.MainActivity
sleep 14
# A freshly provisioned Pixel_4 system image sometimes shows the OS-level
# "Pixel Launcher isn't responding" ANR even though our Activity is alive
# behind that MODAL. Do not mistake Google's home-screen ANR for a Workbench
# failure. Dismiss only the known OS launcher dialog, retrying the real
# application UI assertions afterwards. Save diagnostics for unknown errors.
for attempt in 1 2 3 4 5; do
  adb shell uiautomator dump /sdcard/fm1-view.xml
  adb pull /sdcard/fm1-view.xml android-ui.xml
  if grep -Eq "Pixel Launcher isn.t responding" android-ui.xml; then
    echo "Dismiss emulator Pixel Launcher system ANR, retry Workbench foreground"
    python3 - <<'PY'
import re, subprocess, xml.etree.ElementTree as ET
root = ET.parse('android-ui.xml').getroot()
texts = [n.get('text', '') for n in root.iter('node')]
assert any('Pixel Launcher' in t and "responding" in t for t in texts), (
    'Do not dismiss an unrelated process failure')
buttons = [n for n in root.iter('node') if n.get('text') == 'Close app'
           and n.get('package') == 'android']
assert len(buttons) == 1, 'Expected exactly one OS launcher ANR close button'
coords = list(map(int, re.findall(r'\d+', buttons[0].get('bounds', ''))))
assert len(coords) == 4
subprocess.run(['adb', 'shell', 'input', 'tap',
    str((coords[0]+coords[2])//2), str((coords[1]+coords[3])//2)], check=True)
PY
    sleep 3
    adb shell am start -n com.koolkatze.fm1offline/.MainActivity
    continue
  fi
  if grep -q 'Mostrar controles' android-ui.xml && grep -q 'Datos de sonido' android-ui.xml; then
    echo "Workbench native toolbar ready on attempt ${attempt}"
    break
  fi
  if [[ "$attempt" -eq 5 ]]; then
    echo "::error::Workbench controls not ready; inspect android-ui.xml and screenshot"
    cat android-ui.xml
    adb logcat -d -s FM1Mobile:E > android-errors.log || true
    adb exec-out screencap -p > android-before-start.png
    exit 1
  fi
  echo "Workbench controls still loading (attempt ${attempt})"
  sleep 5
done
adb exec-out screencap -p > android-before-start.png
# In compact mode the native toolbar must be invisible by default.
grep -q 'Mostrar controles' android-ui.xml
grep -q 'Datos de sonido' android-ui.xml
if grep -q 'Iniciar firmware' android-ui.xml; then
  echo "The bulky control bar must start collapsed"
  exit 1
fi
# Open the native diagnostic dialog and verify real WebView
# readiness is shown there, not in a permanent status footer.
python3 - <<'PY'
import re, subprocess, xml.etree.ElementTree as ET
root = ET.parse('android-ui.xml').getroot()
def tap(label):
    matches = [node for node in root.iter('node')
               if node.get('content-desc') == label]
    assert matches, f'Missing visible {label} button'
    coordinates = list(map(int, re.findall(r'\d+', matches[0].get('bounds', ''))))
    assert len(coordinates) == 4, 'Missing coordinates'
    subprocess.run(['adb', 'shell', 'input', 'tap',
        str((coordinates[0] + coordinates[2]) // 2),
        str((coordinates[1] + coordinates[3]) // 2)], check=True)
tap('Datos de sonido')
PY
sleep 1
adb shell uiautomator dump /sdcard/fm1-info-ui.xml
adb pull /sdcard/fm1-info-ui.xml android-info-ui.xml
grep -q 'Preparado' android-info-ui.xml
adb shell input keyevent 4
# The compact chevron must restore the original full toolbar.
python3 - <<'PY'
import re, subprocess, xml.etree.ElementTree as ET
root = ET.parse('android-ui.xml').getroot()
nodes = [n for n in root.iter('node') if n.get('content-desc') == 'Mostrar controles']
assert nodes, 'Chevron missing'
nums = list(map(int, re.findall(r'\d+', nodes[0].get('bounds', ''))))
assert len(nums) == 4
subprocess.run(['adb','shell','input','tap',
    str((nums[0]+nums[2])//2),str((nums[1]+nums[3])//2)], check=True)
PY
sleep 1
adb shell uiautomator dump /sdcard/fm1-expanded-ui.xml
adb pull /sdcard/fm1-expanded-ui.xml android-expanded-ui.xml
grep -q 'Ocultar controles' android-expanded-ui.xml
grep -q 'Iniciar firmware' android-expanded-ui.xml
grep -q 'Pantalla' android-expanded-ui.xml
adb logcat -d -s FM1Mobile:E > android-errors.log
if grep -E 'WebView:.*(Uncaught|Failed to fetch|404|SyntaxError)' android-errors.log; then
  echo "WebView reported a fatal resource/JS error"
  exit 1
fi
