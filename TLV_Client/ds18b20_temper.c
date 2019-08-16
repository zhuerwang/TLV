#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ds18b20_temper.h"

float ds18b20_get_temper()
{
    int  fd =-1;
    int  rv =-1;
    double  num= 0;
    char *p;
    char buf[1024];

    if((fd=open("/sys/bus/w1/devices/28-041731f7c0ff/w1_slave",O_RDONLY))<0)
    {
        printf("open file is failure:%s\n",strerror(errno));
        goto cleanup;
    }

    memset(buf,0,sizeof(buf));
    if((rv=read(fd,buf,sizeof(buf)))<0)
    {
        printf("read temperature failure:%s\n",strerror(errno));
        goto cleanup;
    }

    if((p=strstr(buf,"t="))!=NULL)
    
    {
        num=(atof(p+2)/1000);
        return num;
    }
cleanup:
    close(fd);
}
