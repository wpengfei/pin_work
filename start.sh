#sudo sh -c "echo 0 > /proc/sys/kernel/randomize_va_space"

#CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/home/wpf/Desktop/jsoncpp/include/
#export CPLUS_INCLUDE_PATH

cd ~/Desktop/pin_work
#gcc test.c -lpthread -o test

cd mypintool
rm obj-ia32/pintool_main.so
rm obj-ia32/pintool_main.o
rm obj-ia32/monitor.so
rm obj-ia32/monitor.o
rm obj-ia32/replay.so
rm pattern_for_replay.log

make PIN_ROOT=../pin-3.2-81205-gcc-linux/  obj-ia32/monitor.so 
echo ================================================

#main_bank_lock   
#main_bank_nolock
#main_circular  
#log_proc_sweep  
#string_buffer
#mysql_169
#apache_httpd
#mysql_4012
#mozilla


time ../pin-3.2-81205-gcc-linux/pin -t  obj-ia32/monitor.so -- ~/Desktop/pin_work/test_dir/log_proc_sweep 
#time ../pin-3.2-81205-gcc-linux/pin -t obj-ia32/pintool_main.so -- ~/Desktop/pin_work/test_dir/FFT -p 2
#time ../pin-3.2-81205-gcc-linux/pin -t obj-ia32/pintool_main.so -- ~/Desktop/pin_work/test_cases/pfscan/pfscan -d file ~/Desktop/pin_work/test_cases/pfscan/pfscan.c
echo ================================================

#make PIN_ROOT=../pin-3.2-81205-gcc-linux/  obj-ia32/replay.so
#time ../pin-3.2-81205-gcc-linux/pin -t obj-ia32/replay.so -- ~/Desktop/pin_work/test_dir/main

