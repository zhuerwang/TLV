
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include "sock_init.h"

int sock_init(const char *ip, int port)
{
    int sock_fd = -1;
    struct sockaddr_in serv_addr;
    if(!(sock_fd=socket(AF_INET,SOCK_STREAM,0)))
    {
        printf("creat socket failure:%s\n",strerror(errno));                                                 
        syslog(LOG_ERR,"creat socket failure");
        return -1;
    }
    else if(sock_fd > 0)
    {
        memset(&serv_addr,0,sizeof(serv_addr));
        serv_addr.sin_family = AF_INET; 
        serv_addr.sin_port = htons(port);
        inet_aton(ip,&serv_addr.sin_addr);
        if(connect(sock_fd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        {   
            printf("connect server failure:%s\n",strerror(errno));
            syslog(LOG_ERR,"connect server failure");
            return -1;
        }
        else
            return sock_fd;
    }
}
