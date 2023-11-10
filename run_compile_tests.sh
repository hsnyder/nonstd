#!/bin/sh
echo "nonstd_arch"
echo "----------------------------------"
cc -I./nonstd -c -o /dev/null compile_test_nonstd_arch.c
echo
echo
echo "nonstd_base"
echo "----------------------------------"
cc -I./nonstd -c -o /dev/null compile_test_nonstd_base.c
echo
echo
echo "nonstd_str"
echo "----------------------------------"
cc -I./nonstd -c -o /dev/null compile_test_nonstd_str.c
echo
echo
echo "nonstd"
echo "----------------------------------"
cc -I./nonstd -c -o /dev/null compile_test_nonstd.c
echo
echo
echo "numerics"
echo "----------------------------------"
cc -I./numerics -c -o /dev/null compile_test_numerics.c
