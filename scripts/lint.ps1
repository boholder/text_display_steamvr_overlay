Get-ChildItem -Recurse -Path src -Include *.c,*.cc,*.cxx,*.cpp,*.h,*.hpp,*.hh |
 ForEach-Object { clang-tidy -p cmake-build/x64-release $_.FullName }
