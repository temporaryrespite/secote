#!/usr/bin/env bash

#src: blugon v1.11.3 https://github.com/jumper149/blugon/blob/baf79b5b19f3387b4186fc5dacd31a7c47e22144/backends/tty/tty.sh#L1

EscStart="\e]P"
# man 4 console_codes
#       ESC ]     OSC      (Should  be: Operating system command) ESC ] P nr‐
#                          rggbb: set palette,  with  parameter  given  in  7
#                          hexadecimal  digits after the final P :-(.  Here n
#                          is the color  (0–15),  and  rrggbb  indicates  the
#                          red/green/blue  values  (0–255).   ESC  ] R: reset
#                          palette
#funny they don't say it should be hex!


kelvin3000=(
zero  #ignore this line which is so that I don't have to use a zero-based counter to access these array elements

#bugged: fix: https://github.com/jumper149/blugon/pull/6
#0000
#1aa00
#20760
#3aa3b0
#40049
#5aa049
#607649
#7aa7649
#8553b24
#9ff3b24
#A55b124
#Bffb124
#C553b6d
#Dff3b6d
#E55b16d
#Fffb16d

#fixed:
0000000
1aa0000
2007600
3aa3b00
4000049
5aa0049
6007649
7aa7649
8553b24
9ff3b24
A55b124
Bffb124
C553b6d
Dff3b6d
E55b16d
Fffb16d
)

echo "!$#!" >/tmp/tty.log
count=1
for i in "$@"; do
  echo "!$count!$i!" >> /tmp/tty.log
  #TODO: only test if kelvin is 3000
  exp="${kelvin3000[$count]}"
  if test "${exp}" != "$i"; then
    echo "Fail: got '$i' expected '$exp'" >&2
  fi
  echo -en "${EscStart}${i}"
  (( count++ ))
done

