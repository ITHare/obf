#!/bin/sh

nn=1024
if [ $# -gt 0 ]; then
  nn=$1
fi

gcc -O2 -o helper -std=c++1z -lstdc++ ../helper.cpp
if [ ! $? -eq 0 ]; then
  exit 1
fi

./helper $nn >obftemptest.sh
if [ ! $? -eq 0 ]; then
  exit 1
fi

chmod 700 obftemptest.sh
./obftemptest.sh
if [ ! $? -eq 0 ]; then
  exit 1
fi

rm obftemptest.sh



