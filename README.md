# SMS dumper

This project is a very low cost USB reader/writer(1) for Sega Master System cartridges.
HID code is based on libopencm3 ARM Lib & paulfertser/stm32-tx-hid ( https://github.com/paulfertser/stm32-tx-hid)

Support :
* Sega Master System mapper (Sms/Mark3, SG-1000/SC-3000/Othello Multivision)
* Codemasters mapper
* Korean "large" mapper (type Jang Pung III)
* Read and write S-ram (Master System Mapper)
* can re-write flash eprom on my pcb(1)

Speed (can vary depend on USB) :
* Dump 128kB : 4s
* write 128kB : 21s (erase/write and verif)(1)

(1)only working with my sega mapper clone/pcb

This firmware is targetting the cheapest STM32F103 board as found in
numerous listings on eBay and Aliexpress (might have the following
keywords: ARM STM32F103C8T6 Minimum System Development Board Module
For Arduino).

* Board description
The board has an STM32F103C8T6 controller for USB communication
(no drivers needed, connected in HID) 
It uses two 74LS273 for multiplexing i/o data lines into address lines.

* Building the firmware
After cloning the repository "git submodule init; git submodule
update" needs to be run once, then "build.sh" to build everything
necessary.

It is assumed that "arm-none-eabi" toolchain is available in PATH,
"GCC ARM Embedded" is the recommended option. In case you need to
specify a path, CROSS_COMPILE make variable should be set
appropriately.

* Initial flashing procedure
There're two ways of flashing a blank board: via SWD and via serial
bootloader. For the first method a suitable debug adapter is needed
(e.g. ST-Link or J-Link), for the second one can use just about any
USB-UART bridge (the only requirement that is a bit unusual is to
support "even" parity).

The image to be flashed is "smsdumper_stm32_bootANDsoft.bin", it
contains both a DFU-compliant bootloader and the main firmware. 

If you have any suggestions or questions, feel free to contact me
directly via email.

Ichigo <github@ultimate-consoles.fr>

My friend X-death have made a silimar dumper but for Megadrive/Genesis (Md Dumper)

Available on his Github page : https://github.com/X-death25/STM32_Projects/tree/master/Megadrive_HID
