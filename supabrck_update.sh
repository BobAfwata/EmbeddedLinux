#!/bin/bash

NEW_FW="/opt/firmware/firmware.hex"
#NEW_FW=`cat "$file"`
OLD_FW="/opt/firmware/old_firmware.hex"

checksum=$(md5sum /opt/firmware/firmware.hex)
checksum=$(echo $checksum | awk -F ' ' '{printf "%s",$1 }' | tail -c 4)

echo "SupaBRCK Firmware checksum is: "$checksum


while :
do
        sleep 1  
        # send to BSL mode
        echo "BSL" > /dev/ttyACM0
        sleep 3
        bsl_mode=$(lsusb | grep "2047:0200")
        if [ "$bsl_mode" != "" ]
        then
                #we triggered BSL mode
                sleep 1
                read -p "Press any key to program the supaBRCK board " -n1 -s
                python -m msp430.bsl5.hid -eEP -x 0x17F0 --password=$OLD_FW  $NEW_FW
                #backup this firmware
                sleep 3
                mv $NEW_FW $OLD_FW
                #wait for ttyACM0 to return
                while true
                do
                        echo "Waiting for CDC mode"
                        sleep 3
                        cdc_mode=$(lsusb | grep "2047:0300")
                        if [[ ! -z $cdc_mode ]]; then break; fi
                done
                #reset the firmware time
                echo "updating firmware time..."
                echo "RTCSTME$(date +%Y%m%d%H%M%S --date @$(( `date +%s`+3*3600)))" > /dev/ttyACM0
                #break out of the main loop
                break
        else
                echo "Unable to find BSL device"
        fi
done
