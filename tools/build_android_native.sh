#!/usr/bin/env bash
# Build true Android FM-1 Rust/AAudio synth (not the browser AudioWorklet).
# Call this after setting up the Android SDK + JDK in GitHub Actions.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
SDK="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
if [[ -z "$SDK" ]]; then
  echo "Set ANDROID_SDK_ROOT or ANDROID_HOME" >&2
  exit 1
fi
NDK_VERSION="${FM1_ANDROID_NDK_VERSION:-27.2.12479018}"
NDK="$SDK/ndk/$NDK_VERSION"
if [[ ! -d "$NDK" ]]; then
  "$SDK/cmdline-tools/latest/bin/sdkmanager" "ndk;$NDK_VERSION"
fi
TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin"
test -d "$TOOLCHAIN"
rustup target add aarch64-linux-android x86_64-linux-android
export CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER="$TOOLCHAIN/aarch64-linux-android26-clang"
export CARGO_TARGET_X86_64_LINUX_ANDROID_LINKER="$TOOLCHAIN/x86_64-linux-android26-clang"
export CC_aarch64_linux_android="$TOOLCHAIN/aarch64-linux-android26-clang"
export CC_x86_64_linux_android="$TOOLCHAIN/x86_64-linux-android26-clang"
export AR_aarch64_linux_android="$TOOLCHAIN/llvm-ar"
export AR_x86_64_linux_android="$TOOLCHAIN/llvm-ar"
for mapping in aarch64-linux-android:arm64-v8a x86_64-linux-android:x86_64; do
  target="${mapping%%:*}"
  abi="${mapping#*:}"
  echo "[native] Compiling Rust FM-1/AAudio for $target -> $abi"
  cargo build --release -p fm1-android --target "$target" --locked
  output="android/app/src/main/jniLibs/$abi"
  mkdir -p "$output"
  cp "target/$target/release/libfm1_android.so" "$output/libfm1_android.so"
  test -s "$output/libfm1_android.so"
  file "$output/libfm1_android.so"
done
echo "[native] Both Android native synthesizer ABIs are available."
