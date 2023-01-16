obj-m:= my_driver.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=/home/kernel/adams_PA1 modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=/home/kernel/adams_PA1 clean
