#!/bin/bash

ARGS=""

# machine
ARGS="${ARGS} -M vee-stm32f405"

# executble
# ARGS="${ARGS} -kernel /home/guo/test_405.elf"
# ARGS="${ARGS} -kernel /home/guo/ssd1306_spi.elf"
ARGS="${ARGS} -kernel ./vee-test-kernel/ssd1306_spi_no_systemclock_config.elf"

# qmp monitor server
ARGS="${ARGS} -chardev socket,id=qmp,port=4444,host=localhost,server=on"
ARGS="${ARGS} -mon chardev=qmp,mode=control,pretty=off"

# devices
ARGS="${ARGS} -device vee-spi,id=vee-spi0"
ARGS="${ARGS} -device vee-ssd1306,id=vee-display0,vee-spi='/machine/peripheral/vee-spi0'"
ARGS="${ARGS} -device vee-line,id=vee-line0,vee-pins-path='/machine/soc/GPIOB/pin7,,/machine/peripheral/vee-display0/pin-rst'"
ARGS="${ARGS} -device vee-line,id=vee-line1,vee-pins-path='/machine/soc/GPIOB/pin6,,/machine/peripheral/vee-spi0/pin-cs'"
ARGS="${ARGS} -device vee-line,id=vee-line2,vee-pins-path='/machine/soc/GPIOB/pin5,,/machine/peripheral/vee-spi0/pin-mosi'"
ARGS="${ARGS} -device vee-line,id=vee-line3,vee-pins-path='/machine/soc/GPIOB/pin4,,/machine/peripheral/vee-display0/pin-dc'"
ARGS="${ARGS} -device vee-line,id=vee-line4,vee-pins-path='/machine/soc/GPIOB/pin3,,/machine/peripheral/vee-spi0/pin-sck'"

# cpu debug
# ARGS="${ARGS} -d cpu_reset,guest_errors,in_asm"
# ARGS="${ARGS} -trace clock"

# shorthand for -gdb tcp::1234
# ARGS="${ARGS} -s"

# freeze CPU at startup (use 'c' to start execution), in vee, use it wait for feed
ARGS="${ARGS} -S"

ARGS="${ARGS} -nographic"

echo qemu-system-arm ${ARGS}
echo ""

./build/qemu-system-arm ${ARGS}

stty sane
