#!/bin/sh -eu
#
# Copyright © 2017-2019 Samuel Holland <samuel@sholland.org>
# SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
#

binary=$1
output=$2

if "$binary" > "${output}.tmp"; then
  mv -f "${output}.tmp" "$output"
else
  error=$?
  cat "${output}.tmp"
  rm -f "${output}.tmp"
  exit "$error"
fi
