#!/bin/bash

#This compiles and runs, several times, with tests too. Last compiled binary is the release version.

cfile="./secote.c"
exefile="secote"
exefile="$(realpath -- "$exefile")"
#^ to handle the case when no path or relative path is given, and still be able to execute $exefile we tranform it into absolute path!
echo "exefile=${exefile}"
commonflags=(
  #optionals:
  -Wall
  -Werror
  -ggdb
  "-D_FORTIFY_SOURCE=2"
  "-std=c99"
  -O2
  #required:
  -lm
  -lX11
  -lXrandr
)
rm -- "$exefile" 2>/dev/null
#assert doesn't work for these 2:
gcc "${commonflags[@]}" -DAPPLY_TESTS -DNDEBUG -DDEBUG -o "${exefile}" "${cfile}" ; ec="$?"
if test "$ec" != "0"; then
  echo "!! Compilation failed, aborting." >&2
  exit "$ec"
fi
"${exefile}" 3000.0 ; echo "exit code=$?"
echo '-----'
gcc "${commonflags[@]}" -DAPPLY_TESTS -DNDEBUG -o "${exefile}" "${cfile}" && "${exefile}" 3000.0 ; echo "exit code=$?"
echo '-----'
#assert works for these two:
gcc "${commonflags[@]}" -DAPPLY_TESTS -DDEBUG -o "${exefile}" "${cfile}" && "${exefile}" 3000.0 ; echo "exit code=$?"
echo '-----'
gcc "${commonflags[@]}" -DAPPLY_TESTS -o "${exefile}" "${cfile}" && "${exefile}" 3000.0 ; echo "exit code=$?"
echo '-----'
"${exefile}" 2000
echo '-----'

#conclusion: assert works as long as NDEBUG isn't defined, regardless of whether or not DEBUG is!

gcc "${commonflags[@]}" -o "${exefile}" "${cfile}" && "${exefile}" 3000.0 ; echo "exit code=$?"
"${exefile}" 2000
echo '-----'
sleep 1
"${exefile}" 3000
echo '-----'
