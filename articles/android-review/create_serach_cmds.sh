#!/bin/bash

DATE_AFTER=2022-10-13
DATE_BEFORE=2022-10-28

OUTPUT=./search_cmds.txt
rm -f $OUTPUT
touch $OUTPUT

echo "# Build System">> $OUTPUT
REPO=platform/build
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+(riscv64+or+riscv)" >> $OUTPUT
echo >> $OUTPUT

REPO=platform/build/bazel
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+(riscv64+or+riscv)" >> $OUTPUT
echo >> $OUTPUT

REPO=platform/build/soong
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+(riscv64+or+riscv)" >> $OUTPUT
echo >> $OUTPUT

echo "# Bionic">> $OUTPUT
REPO=platform/bionic
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+(riscv64+or+riscv)" >> $OUTPUT
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+(risc)" >> $OUTPUT
echo >> $OUTPUT

echo "# Kernel">> $OUTPUT
REPO=platform/external/kernel-headers
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+(riscv64+or+riscv)" >> $OUTPUT
echo >> $OUTPUT

REPO=kernel/build
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+(riscv64+or+riscv)" >> $OUTPUT
echo >> $OUTPUT

REPO=kernel/configs
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE" >> $OUTPUT
echo >> $OUTPUT

REPO=kernel/common
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE" >> $OUTPUT
echo >> $OUTPUT

echo "# Toolchain">> $OUTPUT
REPO=toolchain/binutils
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE" >> $OUTPUT
echo >> $OUTPUT

REPO=toolchain/llvm_android
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+(riscv64+or+riscv)" >> $OUTPUT
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+(risc)" >> $OUTPUT
echo >> $OUTPUT

REPO=toolchain/llvm-project
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE" >> $OUTPUT
echo >> $OUTPUT

REPO=toolchain/rustc
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE" >> $OUTPUT
echo >> $OUTPUT

echo "# System">> $OUTPUT
REPO=platform/system/core
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+(riscv64+or+riscv)" >> $OUTPUT
echo >> $OUTPUT

echo "# Framework">> $OUTPUT
REPO=platform/art
echo "https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE" >> $OUTPUT
echo >> $OUTPUT