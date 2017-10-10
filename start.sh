cd ~/Desktop/pin_work
gcc test.c -lpthread -o test

cd mypintool
rm obj-ia32/pintool_main.so
rm obj-ia32/replay.so
rm pattern_for_replay.log

make PIN_ROOT=../pin-3.2-81205-gcc-linux/  obj-ia32/pintool_main.so
echo ================================================

#main_bank_lock   
#main_bank_nolock
#main_circular  
#log_proc_sweep  
#string_buffer

time ../pin-3.2-81205-gcc-linux/pin -t obj-ia32/pintool_main.so -- ~/Desktop/pin_work/test_dir/string_buffer
echo ================================================

make PIN_ROOT=../pin-3.2-81205-gcc-linux/  obj-ia32/replay.so
time ../pin-3.2-81205-gcc-linux/pin -t obj-ia32/replay.so -- ~/Desktop/pin_work/test_dir/string_buffer

#head pinatrace.out
