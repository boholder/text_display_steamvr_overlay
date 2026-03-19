Get-ChildItem -Recurse -Path src -Include *.c,*.cc,*.cxx,*.cpp,*.h,*.hpp,*.hh |
 ForEach-Object { clang-format -i --style=file $_.FullName }
