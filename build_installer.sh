#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
IM=third_party/imgui
GLFW=third_party/glfw
VER=$(sed -n 's/^#define INSTALLER_VERSION "\(.*\)"/\1/p' src/version.h)
RC=$(mktemp -d)
trap 'rm -rf "$RC"' EXIT
version_header() {
  mkdir -p "$RC/$1"
  printf '#define VER_MAJOR %s\n#define VER_MINOR %s\n#define VER_STRING "%s"\n#define VER_FILETYPE VFT_APP\n#define VER_DESCRIPTION "FaTrainer launcher (%s)"\n#define VER_INTERNAL "%s"\n#define VER_FILENAME "%s.exe"\n' \
    "${VER%%.*}" "${VER#*.}" "$VER" "$2" "$3" "$3" > "$RC/$1/version_values.h"
}
version_header fatum "Fatum Version" FaTrainer
export RC
IMGUI="$IM/imgui.cpp $IM/imgui_draw.cpp $IM/imgui_tables.cpp $IM/imgui_widgets.cpp"

nix-shell -p pkgsCross.mingwW64.buildPackages.gcc --run "
  set -e
  x86_64-w64-mingw32-windres installer/installer.rc -O coff -o \$RC/fatum.res -I. -I\$RC/fatum
  windows_installer() {
    x86_64-w64-mingw32-g++ -O2 -std=c++17 -municode -mwindows -o \$1 \$2 installer/main_win.cpp $IMGUI \
      $IM/backends/imgui_impl_dx11.cpp $IM/backends/imgui_impl_win32.cpp \$4 \
      -I$IM -Isrc -Iinstaller -DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD -DIMGUI_DISABLE_DEMO_WINDOWS -DIMGUI_DISABLE_DEBUG_TOOLS -D_WIN32_WINNT=0x0A00 \
      -static -static-libgcc -static-libstdc++ -ld3d11 -ldxgi -ld3dcompiler -ldwmapi -lgdi32 -limm32 -lole32 -luuid -lshell32 -s \$3
  }
  windows_installer dist/FaTrainer.exe '' -lwinhttp \$RC/fatum.res
"

nix-shell -p zig libx11.dev libxrandr.dev libxinerama.dev libxcursor.dev libxi.dev libxext.dev libxrender.dev libxfixes.dev xorgproto --run "
  set -e
  export ZIG_GLOBAL_CACHE_DIR=\$PWD/.zig-cache ZIG_LOCAL_CACHE_DIR=\$PWD/.zig-cache
  TARGET=x86_64-linux-gnu.2.27
  OBJ=\$RC/glfw
  mkdir -p \$OBJ
  for f in context init input monitor platform vulkan window egl_context osmesa_context null_init null_monitor null_window null_joystick \
           posix_module posix_poll posix_thread posix_time x11_init x11_monitor x11_window xkb_unicode glx_context linux_joystick; do
    zig cc -target \$TARGET -O2 -D_GLFW_X11 -D_DEFAULT_SOURCE \$NIX_CFLAGS_COMPILE -I$GLFW/include -c $GLFW/src/\$f.c -o \$OBJ/\$f.o
  done
  linux_installer() {
    zig c++ -target \$TARGET -O2 -std=c++17 -o \$1 \$2 installer/main_linux.cpp $IMGUI \
      $IM/backends/imgui_impl_glfw.cpp $IM/backends/imgui_impl_opengl3.cpp \$OBJ/*.o \
      -I$IM -Isrc -Iinstaller -I$GLFW/include -DGLFW_INCLUDE_NONE -DIMGUI_DISABLE_DEMO_WINDOWS -DIMGUI_DISABLE_DEBUG_TOOLS -ldl -lpthread -lm -s
  }
  linux_installer dist/FaTrainer ''
"
echo "built: dist/FaTrainer[.exe]"
