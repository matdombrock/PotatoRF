#!/bin/bash
g++ test.cpp ssd1306/ssd1306.o ssd1306/linux_i2c.o -lgpiod -lpthread -std=c++17 -I ./ -o test.o || exit 1 
./test.o
