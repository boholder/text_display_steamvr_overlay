#!/usr/bin/env bash

find src -type f \( -name "*.c" -o -name "*.cc" -o -name "*.cxx" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.hh" \) \
  -exec clang-format -i --style=file {} +