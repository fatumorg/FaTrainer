#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
[ -f src/items.h ] && [ -f src/skills.h ] && [ -f src/texts.h ] || python3 gen_data.py
IM=third_party/imgui
VER=$(sed -n 's/^#define FATRAINER_VERSION "\(.*\)"/\1/p' src/version.h)
NUM=${VER%% *}
RC=$(mktemp -d)
trap 'rm -rf "$RC"' EXIT
version_header() {
  mkdir -p "$RC/$1"
  printf '#define VER_MAJOR %s\n#define VER_MINOR %s\n#define VER_STRING "%s"\n#define VER_FILETYPE %s\n#define VER_DESCRIPTION "%s"\n#define VER_INTERNAL "%s"\n#define VER_FILENAME "%s"\n' \
    "${NUM%%.*}" "${NUM#*.}" "$VER" "$2" "$3" "$4" "$5" > "$RC/$1/version_values.h"
}
version_header dll VFT_DLL "FaTrainer | Dying Light" xinput1_3 xinput1_3.dll
export RC
nix-shell -p pkgsCross.mingwW64.buildPackages.gcc --run "
  set -e
  x86_64-w64-mingw32-windres src/version.rc -O coff -o \$RC/dll.res -I\$RC/dll
  x86_64-w64-mingw32-g++ -O2 -std=c++17 -shared -o xinput1_3.dll \
    src/dllmain.cpp $IM/imgui.cpp $IM/imgui_draw.cpp $IM/imgui_tables.cpp $IM/imgui_widgets.cpp \
    $IM/backends/imgui_impl_dx11.cpp $IM/backends/imgui_impl_win32.cpp src/xinput1_3.def \$RC/dll.res \
    -I$IM -Isrc -DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD -DIMGUI_USER_CONFIG=\\\"imconfig_dlt.h\\\" \
    -static -static-libgcc -static-libstdc++ -ld3d11 -ldxgi -ld3dcompiler -ldwmapi -lgdi32 -limm32 -luser32 -ldinput8 -ldxguid -lwinhttp
  x86_64-w64-mingw32-g++ -O2 -std=c++17 -shared -o xinput1_3-visuals.dll -DFATRAINER_VISUALS=1 \
    src/dllmain.cpp $IM/imgui.cpp $IM/imgui_draw.cpp $IM/imgui_tables.cpp $IM/imgui_widgets.cpp \
    $IM/backends/imgui_impl_dx11.cpp $IM/backends/imgui_impl_win32.cpp src/xinput1_3.def \$RC/dll.res \
    -I$IM -Isrc -DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD -DIMGUI_USER_CONFIG=\\\"imconfig_dlt.h\\\" \
    -static -static-libgcc -static-libstdc++ -ld3d11 -ldxgi -ld3dcompiler -ldwmapi -lgdi32 -limm32 -luser32 -ldinput8 -ldxguid
  x86_64-w64-mingw32-g++ -O2 -std=c++17 -o test/selftest.exe test/selftest.cpp -Isrc -static -ldinput8 -ldxguid
  x86_64-w64-mingw32-g++ -O2 -std=c++17 -o test/preview.exe test/preview.cpp -Isrc -static -ld3d11 -ldxgi
"
echo "built: $(pwd)/xinput1_3.dll and xinput1_3-visuals.dll"
