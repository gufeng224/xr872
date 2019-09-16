#! /bin/bash

conf_file=.config

echo "*"
echo "* XRADIO SDK Configuration"
echo "*"

echo "Chip"
echo "  1. XR872"
echo "  2. XR808"
echo -n "choice[1-2]: "

read choice

if [[ $choice == 1 ]]; then
	echo "__CONFIG_CHIP_TYPE ?= xr872" > $conf_file
	exit
fi

if [[ $choice == 2 ]]; then
	echo "__CONFIG_CHIP_TYPE ?= xr808" > $conf_file
	exit
fi

echo "ERROR: Invalid choice!"

