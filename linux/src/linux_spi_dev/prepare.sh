#!/bin/bash

INTERRUPT_LINE_PIN_NUM=200
CE_LINE_PIN_NUM=199

echo ${CE_LINE_PIN_NUM} > /sys/class/gpio/export
echo ${INTERRUPT_LINE_PIN_NUM} > /sys/class/gpio/export

# set a CE_LINE_PIN_NUM line as an output with low level of signal
echo low > /sys/class/gpio/gpio${CE_LINE_PIN_NUM}/direction

# make an INTERRUPT_LINE_PIN_NUM line an interrupt line with a 'falling' edge
# of a signal as a trigger of the interrupt
su -c "echo falling > /sys/class/gpio/gpio${INTERRUPT_LINE_PIN_NUM}/edge" -

# load spi host and generic drivers
modprobe spi-s3c64xx
modprobe spidev

sudo chown odroid:odroid /dev/spidev1.0 

echo "ok"
