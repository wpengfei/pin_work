cd ~/Desktop/pin_work
gcc test.c -lpthread -o test

cd mypintool
make PIN_ROOT=../pin-3.2-81205-gcc-linux/ obj-ia32/mypintool.so
echo ================================================

time ../pin-3.2-81205-gcc-linux/pin -t obj-ia32/mypintool.so -- ~/Desktop/pin_work/test
echo ================================================

head pinatrace.out
