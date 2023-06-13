#!/usr/bin/env bash
# Copyright (C) 2022 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

set -ex

# Get Google's build tools
git clone --depth 1 https://chromium.googlesource.com/chromium/tools/depot_tools.git

# HACK: Compile our own gn. The one included in depot_tools requires GLIBC_2.18,
# but CentOS 7 only ships with GLIBC_2.17.
git clone https://gn.googlesource.com/gn
# NOTE: Don't clone with --depth 1, this will make build/gen.py fail!
cd gn
python build/gen.py
ninja -C out
cd ..
# Put the path to our own gn build first
PATH="$(pwd)/gn/out:$(pwd)/depot_tools:$PATH"
export PATH

# Build Crashpad with Clang (default)
# Toolchain can be controlled with env vars CC, CXX and AR
mkdir crashpad
cd crashpad
git clone --depth 1 https://github.com/olive-editor/crashpad.git
gclient config https://github.com/olive-editor/crashpad.git
gclient sync
cd crashpad
# TODO: Do we want to set any special args here? For example:
# gn gen --args="target_cpu=\"x64\" is_debug=true" out/Default
gn gen out/Default
ninja -C out/Default

# Include list
echo 'out/Default/crashpad_handler' > /tmp/crashpad_include_list.txt
find . \( -type f -o -type l \) \
    -name "*.h" -o \
    -name "*.o" -o \
    -name "*.a" | cut -c3- >> /tmp/crashpad_include_list.txt

# Exclude list
echo '**/.git/**
compat/android/**
compat/ios/**
compat/mac/**
compat/non_elf/**
compat/win/**
handler/mac/**
handler/win/**
infra/**
minidump/test/**
out/Default/**_test*
snapshot/fuchsia/**
snapshot/ios/**
snapshot/mac/**
snapshot/win/**
test/**
third_party/fuchsia/**
third_party/gyp/gyp/test/**
third_party/mini_chromium/mini_chromium/base/fuchsia/**
third_party/mini_chromium/mini_chromium/testing/**
tools/mac/**
util/fuchsia/**
util/ios/**
util/mac/**
util/win/**' > /tmp/crashpad_exclude_list.txt

rsync -av \
    --files-from=/tmp/crashpad_include_list.txt \
    --exclude-from=/tmp/crashpad_exclude_list.txt \
    --prune-empty-dirs \
    . "${OLIVE_INSTALL_PREFIX}/crashpad"

cd ../..

# Build Breakpad for minidump_stackwalk
mkdir breakpad
cd breakpad
fetch breakpad
cd src
echo 'diff --git a/src/common/linux/dump_symbols.cc b/src/common/linux/dump_symbols.cc
index 7e639b0a..ce9c0da5 100644
--- a/src/common/linux/dump_symbols.cc
+++ b/src/common/linux/dump_symbols.cc
@@ -35,6 +35,35 @@
 
 #include <assert.h>
 #include <elf.h>
+#ifndef SHF_COMPRESSED
+  /* glibc elf.h does not define the ELF compression types before version 2.22. */
+  #define SHF_COMPRESSED       (1 << 11)  /* Section with compressed data. */
+
+  /* Section compression header.  Used when SHF_COMPRESSED is set.  */
+
+  typedef struct
+  {
+    Elf32_Word ch_type;        /* Compression format.  */
+    Elf32_Word ch_size;        /* Uncompressed data size.  */
+    Elf32_Word ch_addralign;   /* Uncompressed data alignment.  */
+  } Elf32_Chdr;
+
+  typedef struct
+  {
+    Elf64_Word ch_type;        /* Compression format.  */
+    Elf64_Word ch_reserved;
+    Elf64_Xword ch_size;       /* Uncompressed data size.  */
+    Elf64_Xword ch_addralign;  /* Uncompressed data alignment.  */
+  } Elf64_Chdr;
+
+  /* Legal values for ch_type (compression algorithm).  */
+  #define ELFCOMPRESS_ZLIB 1            /* ZLIB/DEFLATE algorithm.  */
+  #define ELFCOMPRESS_ZSTD 2            /* Zstandard algorithm.  */
+  #define ELFCOMPRESS_LOOS 0x60000000   /* Start of OS-specific.  */
+  #define ELFCOMPRESS_HIOS 0x6fffffff   /* End of OS-specific.  */
+  #define ELFCOMPRESS_LOPROC 0x70000000 /* Start of processor-specific.  */
+  #define ELFCOMPRESS_HIPROC 0x7fffffff /* End of processor-specific.  */
+#endif
 #include <errno.h>
 #include <fcntl.h>
 #include <limits.h>' > elf_shf_compressed.patch
git apply elf_shf_compressed.patch
rm elf_shf_compressed.patch
./configure --prefix="${OLIVE_INSTALL_PREFIX}"
make -j$(nproc)
make install
