#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#define IOCTL_Constant 'k'

/*defining IOCTL commands*/
#define RESET _IOW(IOCTL_Constant, 1, int)
#define END _IOW(IOCTL_Constant, 2, int)

int fd,i,bits;
int ret1=0,ret2=0,c=1;
int ioctl_arg=0,w=1;

/*function prototypes*/
int wr(void);
int ict(void);

/*structure to store pixel information*/
struct led_config {
	int led_nos;
	int G_int[16];
	int R_int[16];
	int B_int[16];
};
struct led_config *led;

pthread_t w_thread,ioctl_thread;

int main(void)
{
	/*opening file*/
	fd = open("/dev/WS2812",O_RDWR);
	
	/*allocate memory for structure*/
	led = malloc(sizeof(struct led_config));
	
	if (fd < 0)
	{
		printf("Can not open device file 0\n");		
		return 0;
	}
	else
	{
		printf("\n File is open\n");
		
		/*accept pixel data from user*/
		printf("\n Note: When LED ring is running, enter 1 to send RESET command and 2 for END command\n");
		printf("\n Enter the no. of LEDs: ");
		scanf("%d",&led->led_nos);
		printf("\n No. of leds entered is: %d",led->led_nos);
		
		for(i=0;i<led->led_nos;i++)
		{
			printf("\n Settings for LED%d: ",i+1);
			
			printf("\n Enter the intensity for G: ");
			scanf("%d",&led->G_int[i]);
			
			printf("\n Enter the intensity for R: ");
			scanf("%d",&led->R_int[i]);
			
			printf("\n Enter the intensity for B: ");
			scanf("%d",&led->B_int[i]);
		}
		bits = led->led_nos*24;
		
		write(fd, led,sizeof(struct led_config));
		
		/*create threads to call IOCTL and write functions*/
		pthread_create(&w_thread, NULL,(void*)&wr,NULL);
		printf("\nwrite thread created\n");
		pthread_create(&ioctl_thread,NULL,(void*)&ict,NULL);
		printf("\nioctl thread created\n");
		
		pthread_join(w_thread,NULL);
		pthread_join(ioctl_thread,NULL);
	}
	free(led);
	close(fd);
	printf("\nclose called\n");
	return 0;
}

/*function executed by write thread*/
int wr(void)
{
	while(1)
	{
		write(fd, led,sizeof(struct led_config));
		if(ioctl_arg==2)
		{	
			ret1 = 1;
			pthread_exit(&ret1);
		}
	}
	return 0;
}

/*function executed by IOCTL thread*/
int ict(void)
{
	while(1)
	{
		int c=1;

		scanf("%d",&ioctl_arg);
	
		if(ioctl_arg==1)
		{
			ioctl(fd,RESET,&c);
			ioctl_arg=0;
		}
		if(ioctl_arg==2)
		{	
			ioctl(fd,END,&c);
			ret2 = 2;
			pthread_exit(&ret2);
			ioctl_arg=0;
		}
	}
	return 0;
}

