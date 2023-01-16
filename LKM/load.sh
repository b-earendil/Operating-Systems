#!/bin/bash

if [ ! -e /dev/simple_character_device ];
then
	echo "Creating device file"
	sudo mknod /dev/simple_character_device c 240 0
	echo "Setting permissions"
	sudo chmod 777 /dev/simple_character_device
fi

echo "Installing LKM"
sudo insmod my_driver.ko
