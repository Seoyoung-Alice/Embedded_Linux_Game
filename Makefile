#Makefile for a basic kernel module

obj-m   := fpga_buzzer_driver.o
#obj-m	:= fpga_push_switch_driver.o
obj-m   := fpga_text_lcd_driver.o
obj-m   := fpga_fnd_driver.o
obj-m 	:= fpga_dot_driver.o

KDIR    :=/root/work/achroimx6q/kernel
PWD     :=$(shell pwd)

all: driver app
#all: driver 

driver : 
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules ARCH=arm

app : 
	arm-none-linux-gnueabi-gcc -static -o Total_app Total_app.c
install : 
	cp -a fpga_buzzer_driver.ko ~/nfsroot/TP_OTHERS
	cp -a fpga_test_buzzer ~/nfsroot/TP_OTHERS
#	cp -a fpga_push_switch_driver.ko ~/nfsroot/TP_OTHERS
#	cp -a fpga_test_push_switch ~/nfsroot/TP_OTHERS
	cp -a fpga_text_lcd_driver.ko ~/nfsroot/TP_OTHERS
	cp -a fpga_text_lcd ~/nfsroot/TP_OTHERS
	cp -a fpga_fnd_driver.ko ~/nfsroot/TP_OTHERS
	cp -a fpga_test_fnd ~/nfsroot/TP_OTHERS
	cp -a fpga_dot_driver.ko ~/nfsroot/TP_OTHERS
	cp -a fpga_dot_app ~/nfsroot/TP_OTHERS
clean:
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf *.o
	rm -rf Total_app
	rm -rf Module.symvers
	rm -rf modules.order
	rm -rf .buzzer*
	rm -rf .tmp*
