#!/bin/bash

for f in /dev/hidraw*; do
    file=${f##*/}
    device="$(cat /sys/class/hidraw/$file/device/uevent | grep HID_NAME | cut -d '=' -f2)"

    if [[ $device != "CATEX TECH"* ]]; then
        continue
    fi

    # control device should not have input capability
    if [[ -e "/sys/class/hidraw/$file/device/input" ]]; then
        continue
    fi
    echo /dev/$file
done
