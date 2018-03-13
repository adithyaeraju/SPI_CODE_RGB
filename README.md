# SPI_CODE_RGB

This project is developed to generate pattern on WS2812 RGB 16 LED strip using SPI communication protocol. 
The controller used in this project is Intel Galileo Board with Linux kernel version 1.7.2.

Some steps are mentioned below to run the project in the Galileo board
# Building the module
## On Ubuntu host
### Copy the following files to folder "WS2812"
	-WS2812_device.c
	-WS2812_driver.c
	-main.c
	-Makefile

### Invoke the following command in the folder WS2812
	make

The Makefile cross compiles the device driver and the main program to the Galileo board.

Copy the folder WS2812 to the Galileo board using the command
	sudo scp ./WS2812/* root@"device_ip":/home/WS2812

## Do the following on the Galileo board:
### Run the following commands in the folder WS2812
insmod WS2812_device.ko
insmod WS2812_driver.ko

### Run the user program using
./main

### Remove modules using:
rmmod WS2812_device
rmmod WS2812_driver

### Main program usage
-Main program performs the following:
 Opens the character device.
 Accepts the number of LEDs to be lit up and the pixel information for each LED.
 User may enter the intensity for each color from 0 to 255.
 Data entered by user is stored in a structure.
 Two threads for IOCTL and write function are created.
 Threads run simultaneously.
 Upon exit command, function closes the file descriptor and exits.

NOTE: Since infinite loops are used, user will have to enter data to terminate them.
	Enter 1 to RESET the LED pattern.
	Enter 2 to END the program.

# User level function exposure for WS2812_device.c
## 1. Initialize the module using module_init
static int __init spi_init(void)
-This function initialises the bus master and spi device using board info structure.

## 2. Exit the module using module_exit
static void __exit spi_exit(void)
-Unregisters the spi device.


# User level function exposure for WS2812_driver.c
## 1. Initialize the module using module_init
static int spi_driver_init(void)
-This function registers the spi driver using the spi_driver structure.

## 2. Opening the device
static int WS2812_open(struct inode *inode, struct file *fp)
-Called when device file is opened.

## 3. Releasing the device
static int WS2812_release(struct inode *inode, struct file *fp)
-Called when device is closed from the main function.

## 4. Probe function
static int spi_driver_probe(struct spi_device *spiWS2812)
-Function gets called when both device and driver modules are inserted and matched.
-Allocate memory to spi data and led_config structures.
-Initialize gpio and character device.

## 5. Writing to the device
static ssize_t WS2812_write(struct file *fp, const char *buf, size_t len, loff_t *off)
-Obtains pixel information from user program.
-Encodes the pixel data into format recognizable by WS2812 and stores in a globally declared array.
-This array is passed to spi_trans function that encodes it into a message to be transfered to the spi bus.
-Circularly shifts the array by 24 bits.

## 5. IOCTL function
static long WS2812_ioctl(struct file *fp,unsigned int cmd, unsigned long arg)
-Accepts 2 commands: RESET and END
-RESET command resets the gpio and all LEDs and restarts the rotation of ring from pattern provided by user. 
-END command turns OFF all LEDs and returns to user to end execution of main program.

## 6. Driver remove function
static int spi_driver_remove(struct spi_device *spiWS2812)
-Is called when the driver module is removed.

## 7. Exit the module using module_exit
static void __exit spi_driver_exit(void)
-Frees the GPIO pins and deregisters the character devices.
