#!/bin/bash

echo "Remove kernel module"
sudo rmmod my_driver
echo "Remove device file"
sudo rm /dev/simple_character_device
