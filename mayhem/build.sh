#!/usr/bin/env bash
# mayhem/build.sh — builds Veyon's fuzz harnesses (sanitized) and its unit tests (normal flags).
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS SRC

cd "$SRC"

# 3rdparty/kldap is a git submodule absent from the tree; plugins/ldap needs its sources at
# configure time. Populate it from the pinned copy baked into the image (offline-safe, idempotent).
KLDAP_SRC=/opt/toolchains/deps/kldap-2002b2c4ec81d05eb0fc5b2106ef112d404f5cea/src
if [ ! -f 3rdparty/kldap/src/core/ldapurl.cpp ]; then
	mkdir -p 3rdparty/kldap
	cp -r "$KLDAP_SRC"/. 3rdparty/kldap/
fi
FAKEKEY_SRC=/opt/toolchains/deps/libfakekey-7ad885912efb2131e80914e964d5e635b0d07b40/src
if [ ! -f 3rdparty/libfakekey/src/libfakekey.c ]; then
	mkdir -p 3rdparty/libfakekey
	cp -r "$FAKEKEY_SRC"/. 3rdparty/libfakekey/
fi

COMMON_OPTS=(
	-G Ninja
	-DWITH_TESTS=ON
	-DWITH_LTO=OFF -DWITH_PCH=OFF -DWITH_UNITY_BUILD=OFF
	-DWITH_TRANSLATIONS=OFF -DWITH_WEBAPI=OFF
	-DVEYON_X11VNC_EXTERNAL=ON
	-DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_FLAGS_RELWITHDEBINFO= -DCMAKE_CXX_FLAGS_RELWITHDEBINFO=
)

# 1) Fuzz build: the whole project with ASan+UBSan + SanCov, plus the harnesses in mayhem/fuzz
#    (added to upstream's configure via CMAKE_PROJECT_veyon_INCLUDE).
FUZZ_FLAGS="-O1 $SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS -Wno-error"
cmake -S . -B mayhem/build-fuzz "${COMMON_OPTS[@]}" \
	-DCMAKE_C_FLAGS="$FUZZ_FLAGS" -DCMAKE_CXX_FLAGS="$FUZZ_FLAGS" \
	-DWITH_ADDRESS_SANITIZER=ON \
	-DMAYHEM_LSAN_OFF="$SRC/mayhem/lsan_off.cc" \
	-DCMAKE_PROJECT_veyon_INCLUDE="$SRC/mayhem/fuzz/inject.cmake"
FUZZERS=(variantstream variantarraymessage vncclientprotocol vncserverprotocol featuremessage configimport hostaddress serverauth networkobjectdirectory networkobjectimport)
targets=(veyon-core linux-platform builtindirectory)
for f in "${FUZZERS[@]}"; do targets+=("$f" "$f-standalone"); done
cmake --build mayhem/build-fuzz -j"$MAYHEM_JOBS" --target "${targets[@]}"
# VeyonCore loads plugins from <binary dir>/../lib/veyon (VEYON_PLUGIN_DIR); the harnesses live in
# build-fuzz/mayhem-bin, so expose the built plugins at build-fuzz/lib/veyon.
mkdir -p mayhem/build-fuzz/lib/veyon
ln -sf ../../plugins/platform/linux/linux-platform.so ../../plugins/builtindirectory/builtindirectory.so \
	mayhem/build-fuzz/lib/veyon/

# 2) Unit tests with the project's normal flags (mayhem/test.sh only runs them).
cmake -S . -B mayhem/build-test "${COMMON_OPTS[@]}" \
	-DCMAKE_C_FLAGS="-O1 $COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="-O1 $COVERAGE_FLAGS"
cmake --build mayhem/build-test -j"$MAYHEM_JOBS" \
	--target VncProtocolRobustnessTest FilesystemSafeJoinTest PipeWireVncServerTest
