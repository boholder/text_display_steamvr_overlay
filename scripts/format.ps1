Get-ChildItem -Recurse -Path "src", "test" -Include *.c,*.cc,*.cxx,*.cpp,*.h,*.hpp,*.hh |
 ForEach-Object { clang-format -i --style=file $_.FullName }
