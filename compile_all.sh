#!/bin/bash
gcc -g -Wall -o city-manager main.c
echo "------------------------------"
gcc -g -Wall -o reports_monitor reports_monitor.c
echo "------------------------------"
gcc -g -Wall -o city_hub city_hub.c
echo "Compiled all C files"