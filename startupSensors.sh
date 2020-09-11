#!/bin/bash

#competing sensors
./sensorx 1 1 2345 100 30 45 1  &
./sensorx 2 1 77 100 14 25 1  &
#complementary sensors
./sensorx 5 2 569 100 14 25 1  &
./sensorx 6 3 111 100 14 25 1  &
