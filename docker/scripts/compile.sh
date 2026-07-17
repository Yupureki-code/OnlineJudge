#!/bin/sh
# The host waits for this exec and inspects its real exit status.

case "${CXX_STANDARD:-c++17}" in
    c++17|c++20) ;;
    *) exit 2 ;;
esac

exec g++ -o /home/judge/main /home/judge/source.cpp \
    "-std=${CXX_STANDARD:-c++17}" -O2 -pipe -Wall 2>/home/judge/compile_err
