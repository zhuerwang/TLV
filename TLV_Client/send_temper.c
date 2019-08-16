#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void send_temper(char *str,int fd,int pack_len)
{

    int rv = 0;
    if(!(rv=write(fd,str,pack_len)))
    {
        printf("send to server failure:%s\n",strerror(errno));
        syslog(LOG_NOTICE,"send to server failure!\n");
    }
    else
    {
        printf("send temper to server successfully!\n");
        syslog(LOG_NOTICE,"send temper to server successfully!\n");
    }

}
