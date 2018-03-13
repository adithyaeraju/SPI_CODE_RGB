#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/gpio.h>
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
#include<linux/cdev.h>

#define device_name "WS2812"

#define IOCTL_Constant 'k'
#define RESET _IOW(IOCTL_Constant, 1, int)
#define END _IOW(IOCTL_Constant, 2, int)

/*function prototypes*/
static int WS2812_open(struct inode *inode, struct file *);
static int WS2812_release(struct inode *inode, struct file *);
static long WS2812_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);
static ssize_t WS2812_write(struct file *fp, const char *buf, size_t len, loff_t *off);
static int spi_trans(u8 b[], int bits);

/*global structures*/
static struct cdev *WS2812_cdev;
static dev_t WS2812_dev;
struct class *WS2812_class;
struct device *WS2812_device;
int flag=-1;

/*variable and array to store encoding information*/
u8 temp[384];
u8 reset_buf[384];
u8 a;


struct s_data
{
	struct spi_device *spi2812;
};
struct s_data *s1_data;

struct led_config {
	int led_nos;
	int G_int[16];
	int R_int[16];
	int B_int[16];
};
struct led_config *led;


static void spidev_complete(void *arg)
{
	complete(arg);
};

/*function used to check completion status and make call to async transfer API*/
static ssize_t WS2812_sync(struct s_data *s1_data, struct spi_message *m)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int stat;

	m->complete = spidev_complete;	
	m->context = &done;

	if (s1_data->spi2812 == NULL)
		stat = -ESHUTDOWN;
	else
		{
		stat = spi_async(s1_data->spi2812, m);
		}
		printk("%d",stat);
	if (stat == 0) {
		wait_for_completion(&done);
		stat = m->status;
		if (stat == 0)
			{
			stat = m->actual_length;
			}
	}
	return stat;
}

/*file operations for character device WS2812*/
static struct file_operations WS2812_fops = {
	.owner = THIS_MODULE,
	.open = WS2812_open,
	.release = WS2812_release,
	.write = WS2812_write,
	.unlocked_ioctl = WS2812_ioctl,
};

/*device id table to match the spi devices and its driver*/	
static const struct of_device_id dev_id_table[] =
{
	{.compatible = "WS2812_D"},
	{},
};
MODULE_DEVICE_TABLE(of,dev_id_table);

/*open function called when module is opened by user program*/
static int WS2812_open(struct inode *inode, struct file *fp)
{
	printk("\nWS2812 open function\n");
	return 0;
}

/*release function to close the module called at the end of execution of user program*/
static int WS2812_release(struct inode *inode, struct file *fp)
{
	printk("\nWS2812 release function\n");
	return 0;
}

/*probe function gets called when spi device and its driver match*/
static int spi_driver_probe(struct spi_device *spiWS2812)
{
	int err,req,exp,dir;
	printk("\nprobe is called\n");
	
	/*allocate memory to led data structure and spi driver structure*/
	led = kzalloc(sizeof(*led),GFP_KERNEL);				
	s1_data = kzalloc(sizeof(*s1_data),GFP_KERNEL);
				
	s1_data->spi2812=spiWS2812;
	
	/*request gpio pins*/
	req = gpio_request(24,"sysfs");
	gpio_request(44,"sysfs");
	gpio_request(72,"sysfs");
	
	/*export gpio pins*/
	exp = gpio_export(24,1);
	gpio_export(44,1);
	gpio_export(72,1);

	/*set direction and initial values for pinmux*/
	dir = gpio_direction_output(24, 0);
	gpio_direction_output(44,1);
	gpio_direction_output(72, 0);
	
	/*creating and registering char device*/
	err = alloc_chrdev_region(&WS2812_dev,0,1,device_name);
	printk("\nallocate_chrdev_region");
	WS2812_cdev = cdev_alloc();
	printk("\ncdev_allocate");
	err = cdev_add(WS2812_cdev, WS2812_dev,1);
	if(err<0)
	{
		goto cdev_add_out;
	}
	printk("\ncdev_add");
	WS2812_class = class_create(THIS_MODULE,device_name);
	if(IS_ERR(WS2812_class))
	{
		goto class_err;
	}
	printk("\nclass create");
	cdev_init(WS2812_cdev,&WS2812_fops);
	printk("\ncdev_init");
	WS2812_device = device_create(WS2812_class,NULL,WS2812_dev,NULL,device_name);
	if(IS_ERR(WS2812_device))
	{
		goto device_err;
	}
	printk("\ndevice_create");
	
	return 0;
	
device_err: device_destroy(WS2812_class,WS2812_dev);
class_err: class_unregister(WS2812_class);
		class_destroy(WS2812_class);
cdev_add_out: cdev_del(WS2812_cdev);

return 0;
}

/*function to transfer the array containing encoded led data by spi*/
static int spi_trans(u8 b[],int bits)
{	
	struct spi_transfer	t = {
			.tx_buf		= b,
			.len		= 384,
			.speed_hz	= 6400000,
		};
	struct spi_message m;
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	WS2812_sync(s1_data,&m);
	return 0;
}

/*write function for character device used to accept led data and pass to transfer function*/
static ssize_t WS2812_write(struct file *fp, const char *buf, size_t len, loff_t *off)
{
	int k=0,i,j,r=0;
	int G_bit[8],R_bit[8],B_bit[8];
	
	if(flag==-1)
	{
		led = (struct led_config *)buf;
		flag=0;
	}
	if(flag==0)
	{
		k=0;
		printk("\n Flag=0");
	
		/*clearing previously written value on the led ring*/
		for(i=0;i<384;i++)
		{
			temp[i] = 0xc0;
			reset_buf[i] = 0xc0;
		}
		spi_trans(temp,384);
	
		for(j=0;j<led->led_nos;j++)
		{	
			/*converting 0-255 decimal data of each colour given by user to 8bits*/
			for(i=7;i>=0;i--)
			{
				G_bit[i] = led->G_int[j]%2;
				led->G_int[j]=led->G_int[j]/2;
		
				R_bit[i] = led->R_int[j]%2;
				led->R_int[j]=led->R_int[j]/2;
		
				B_bit[i] = led->B_int[j]%2;
				led->B_int[j]=led->B_int[j]/2;
			}
		
			/*encoding the 8bit data and storing in buffer called temp*/
			for(i=0;i<8;i++)
			{
				if(G_bit[i]==0)
				{
					temp[k]=0xc0;
					reset_buf[k] = 0xc0;
				}
				else if(G_bit[i]==1)
				{
					temp[k]=0xfc;
					reset_buf[k] = 0xfc;
				}
				k++;
			}
			for(i=0;i<8;i++)
			{
				if(R_bit[i]==0)
				{
					temp[k]=0xc0;
					reset_buf[k] = 0xc0;
				}
				else if(R_bit[i]==1)
				{
					temp[k]=0xfc;
					reset_buf[k] = 0xfc;
				}
				k++;
			}
			for(i=0;i<8;i++)
			{
				if(B_bit[i]==0)
				{
					temp[k]=0xc0;
					reset_buf[k] = 0xc0;
				}
				else if(B_bit[i]==1)
				{
					temp[k]=0xfc;
					reset_buf[k] = 0xfc;
				}
				k++;
			}
		}
		spi_trans(temp,384);
		printk("\n\n");
		mdelay(1000);
		flag=1;
		}
		/*circularly shifting the array*/
	
	
		for(r=0;r<24;r++)
		{
			a=temp[383];
				for(j=383;j>0;j--)
				{
				
						temp[j] = temp[j-1];
				
				}
				temp[0] = a;
		
		}
	spi_trans(temp,384);
	mdelay(1000);
		
	return 0;
}

static long WS2812_ioctl(struct file *fp,unsigned int cmd, unsigned long arg)
{
	int inc,p;
	u8 temp1[384];
	gpio_free(5);
	printk("\n IOCTL called\n");
	switch(cmd)
	{
		case END:
		printk("\n END case");
		
		/*request gpio pins*/
		gpio_request(24,"sysfs");
		gpio_request(44,"sysfs");
		gpio_request(72,"sysfs");
		flag=-1;
		
		/*export gpio pins*/
		gpio_export(24,1);
		gpio_export(44,1);
		gpio_export(72,1);

		/*set direction and pinmuxing*/
		gpio_direction_output(24, 0);
		gpio_direction_output(44,1);
		gpio_direction_output(72, 0);
		
		for(inc=0;inc<384;inc++)
		{
			temp1[inc]= 0xc0;		//passing zeros to hold value of spi pin low for more than 150us
		}
		spi_trans(temp1,384);

		case RESET:
		printk("\n RESET case");
		for(inc=0;inc<384;inc++)
		{
			temp1[inc]= 0xc0;		//passing zeros to hold value of spi pin low for more than 150us
		}
		spi_trans(temp1,384);
		mdelay(1000);
		
		/*resetting initial pixel information obtained from user*/
		for(p=0;p<384;p++)
		{
			temp[p] = reset_buf[p];
		}
	}
	return 0;
}

/*Remove function for spi driver*/
static int spi_driver_remove(struct spi_device *spiWS2812)
{
	printk("\n Remove function\n");
return 0;
}

/*spi_driver data structure*/
static struct spi_driver sd = {
	.driver = {
	.name = "WS2812_D",
	.owner = THIS_MODULE,
	.of_match_table = of_match_ptr(dev_id_table),
	},
	.probe = spi_driver_probe,
	.remove = spi_driver_remove,
};

/*init function registers the spi driver*/
static int spi_driver_init(void)
{
	int e;
	
	e = spi_register_driver(&sd);	
	return e;
}

/*exit function frees gpio pin and enregisters and destroys character device*/
static void __exit spi_driver_exit(void)
{
	gpio_free(5);
	device_destroy(WS2812_class,WS2812_dev);
	printk("\ndevice_destroy");
	class_unregister(WS2812_class);
	printk("\nClass_unregister");
	class_destroy(WS2812_class);
	printk("\nclass_destroy");
	cdev_del(WS2812_cdev);
	printk("\ncdev_del");
	spi_unregister_driver(&sd);
	printk("\nspi_unregister_driver");

}
module_init(spi_driver_init);
module_exit(spi_driver_exit);
MODULE_LICENSE("GPL");
