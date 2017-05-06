cd ~/Desktop/pin_work
gcc test.c -lpthread -o test

cd mypintool
rm obj-ia32/pintool_main.so
rm obj-ia32/pintool_main.o
make PIN_ROOT=../pin-3.2-81205-gcc-linux/ obj-ia32/pintool_main.so
echo ================================================

time ../pin-3.2-81205-gcc-linux/pin -t obj-ia32/pintool_main.so -- ~/Desktop/pin_work/main_string
echo ================================================

#head pinatrace.out
