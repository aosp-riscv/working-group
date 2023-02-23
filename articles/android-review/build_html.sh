#!/bin/bash

usage() {
    echo "Usage:"
    echo "  build_html.sh <date_after> <date_before> <output_file>"
    echo "  Run this script to create the URLs to get PR list between the date"
    echo "  and wrtie the URLs strings into the specified output file."
    echo "  after <date_after> and before <date_before>."
    echo "  - <date_after>: The date after the given date, inclusive."
    echo "                  Must be in the format YYYY-MM-DD."
    echo "  - <date_before>: The date before the given date, inclusive."
    echo "                   Must be in the format YYYY-MM-DD."
    echo "  - <output_file>: The file to be output."
    echo
    echo "  Example: ./build_html.sh 2022-01-31 2022-02-10 output.html"
    echo "  Which means you want to search the records in the range [2022/2/1, 2022/2/9],"
    echo "  and print the URLs strings into the output.html."
    echo
}

if [ $# -lt 3 ]; then
    usage
	exit 1
fi

# FIXME: date format has not been checked
DATE_AFTER=$1
DATE_BEFORE=$2
OUTPUT=$3

println() {  
    echo "$1" >> $OUTPUT
}

print_url() {
	local REPO=$1
	local FILTER="(risc)"
	local URL="https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+$FILTER"
	println	"<a href=$URL>$REPO, filter: risc</a><br>"	
}

print_url2() {
	local REPO=$1
	local FILTER="(riscv64+or+riscv)"
	local URL="https://android-review.googlesource.com/q/repo:+$REPO+AND+-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+$FILTER"
	println	"<a href=$URL>$REPO, filter: riscv64 or riscv</a><br>"	
}

print_others() {
	local FILTER="(riscv64+or+riscv)"
	local URL="https://android-review.googlesource.com/q/-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+$FILTER"
	println	"<a href=$URL>$REPO, filter: riscv64 or riscv</a><br>"
	local FILTER="(risc)"
	local URL="https://android-review.googlesource.com/q/-owner:+Auto+AND+merged+after:+$DATE_AFTER+AND+merged+before:+$DATE_BEFORE+AND+$FILTER"
	println	"<a href=$URL>$REPO, filter: risc</a><br>"
}

HTML_TITLE="AOSP RISC-V Progress Report [$DATE_AFTER ~ $DATE_BEFORE]"

println "<!DOCTYPE html>"
println "<html lang=\"en\">"

println "<head>"
println "<meta charset=\"UTF-8\">"
println "<title>$HTML_TITLE</title>"
println "</head>"

println "<body>"
println	"<h1>$HTML_TITLE</h1>"

println	"<h2>Build System</h2>"
print_url2 platform/manifest
print_url2 platform/build
print_url2 platform/build/soong
print_url2 platform/build/bazel

println	"<h2>Bionic</h2>"
print_url2 platform/bionic
print_url platform/bionic
print_url2 platform/external/kernel-headers

println	"<h2>Kernel</h2>"
print_url2 kernel/build
print_url kernel/configs
print_url kernel/common
print_url kernel/tests
print_url2 kernel/common-modules/virtual-device

println	"<h2>Toolchain</h2>"
print_url toolchain/binutils
print_url2 toolchain/llvm_android
print_url toolchain/llvm_android
print_url toolchain/llvm-project
print_url toolchain/rustc

println	"<h2>System</h2>"
print_url2 platform/system/core

println	"<h2>Framework</h2>"
print_url2 platform/art

println	"<h2>Emulator</h2>"
print_url platform/external/qemu
print_url platform/tools/emulator

println	"<h2>Others</h2>"
println "<p>Match all repositories, in case any item is missed.</p>"
print_others

println "</body>"

println "</html>"
