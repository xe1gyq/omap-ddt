#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/videodev.h>


#include "lib.h"

static int usage(void)
{
	printf("Usage: settransck < o/p device [1:LCD 2:TV]> <key type [0:GFX DEST 1:VID SRC]> <RGB key value> \n");
	printf("RGB Key value for 0xF801 -> 63489\n");
	return 0;
}

int main (int argc, char *argv[])
{
	int output_dev, fd, ret,cktype=0;
	struct omap24xxvout_colorkey colorkey = {0};

	if (argc < 4)
		return usage();

	output_dev = atoi(argv[1]) + 3; 
	
	if ((output_dev != OMAP24XX_OUTPUT_LCD) && (output_dev != OMAP24XX_OUTPUT_TV)){
		printf("Output device should be 1 for LCD or 2 for TV \n");
		return usage();
	}

	cktype	= atoi(argv[2])+ 100;
	
	if((cktype != OMAP24XX_GFX_DESTINATION) && (cktype != OMAP24XX_VIDEO_SOURCE)){
		printf("Color Key type should be 1 for GFX destination and 2 for VID src\n");
		return usage();
	}

	fd = open (VIDEO_DEVICE1, O_RDONLY) ;
	if (fd <= 0) {
		printf("Could not open %s\n",VIDEO_DEVICE1);
		return -1;
	}
	else
		printf("openned %s\n", VIDEO_DEVICE1);

        colorkey.output_dev	= output_dev; 
        colorkey.key_type	= cktype;
	colorkey.key_val	= atoi(argv[3]);

	ret = ioctl (fd,VIDIOC_S_OMAP2_COLORKEY, &colorkey);
	if (ret < 0) {
		perror ("VIDIOC_S_OMAP2_COLORKEY");
		return 0;
	}

	close(fd);
}
