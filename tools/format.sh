#!/usr/bin/env bash
set -e -u -x -o pipefail

clang-format-3.8 -sort-includes -style=file -i $(git ls-files **/*.cc **/*.h)
