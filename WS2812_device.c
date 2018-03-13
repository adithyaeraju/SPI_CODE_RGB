#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include<linux/ioctl.h>
#include<linux/interrupt.h>
#include<asm/delay.h>
#include<linux/delay.h>
#include<linux/hrtimer.h>
#include<linux/ktime.h>
#include<linux/sysfs.h>
#include<linux/slab.h>
#include<linux/pinctrl/pinmux.h>
#include<linux/spi/spi.h>
#include<linux/spi/spidev.h>
#include<linux/compat.h>
#include<linux/of.h>
#include<linux/of_device.h>

static uint WS2812_bus= 1;
static uint WS2812_cs=1;
static uint freq_hz = 6400000;
int n=1;

module_param(n,int,0);

static struct spi_device *spiWS2812;

/*init function for module registers spi device*/
static int __init spi_init(void)
{
	int e;
	struct spi_master *gallileo_master;
  	struct spi_board_info GWS_info =
	  {
	    .modalias = "WS2812_D",
	    .max_speed_hz = freq_hz,
	    .bus_num = WS2812_bus,
	    .chip_select = WS2812_cs,
	    .mode = 0,
	  };

	gallileo_master = spi_busnum_to_master( GWS_info.bus_num);
	spiWS2812 = spi_new_device(gallileo_master,&GWS_info);
	e = spi_setup(spiWS2812);
	if(e)
	{
		spi_unregister_device(spiWS2812);
	}
	else
	{
		printk("\nWS2812 is registered\n");
	}
	return e;
}

/*exit functio unregisters spi device*/
static void __exit spi_exit(void)
{
	spi_unregister_device(spiWS2812);
}

module_init(spi_init);
module_exit(spi_exit);
MODULE_LICENSE("GPL");
