 /* ********************************************************************************
  *      Copyright:  (C) 2019 454256587<454256587@qq.com>
  *                  All rights reserved.        
  *                                              
  *       Filename:  server.c          
  *    Description:  This file is a client of tlv 
  *                                    
  *        Version:  1.0.0(01/04/19)   
  *         Author:  454256587 <454256587@qq.com>
  *      ChangeLog:  1, Release initial version on "01/04/19 15:47:58"
  *                                    
  *********************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "sock_init.h"
#include "send_temper.h"
#include "get_time.h"
#include "ds18b20_temper.h"
#include "crc-itu-t.h"

#define THE_HEAD    0xFD
#define LHJ         0xAA
#define MIN_SIZE    5
#define delims      "|"

int p_stop = 0;

void sig_handler(int sig_t);
void sig_handler(int sig_t)
{
    if(SIGTERM==sig_t)
    p_stop = 1;
}

void printf_usage(char *command)
{
    printf("%s usage:\n",command);
    printf("-i(--IP):sepcify server IP address.\n");
    printf("-p(--port):sepcify port\n");
    printf("-t(--time space):sepcify temperature reporting time space.\n");
    printf("-h(--help):print this help information.\n");
}


int main(int argc , char *argv[])
{
    char pack_buf[64];
    char id_buf[16];
    char time_buf[16];
    int pointer     =   0;
   
    int fd          =   -1;
    int opt         =   -1;
    int port        =   0;
    int time_space  =   0;
    int daemon_run  =   0;
    int datalen     =   0;
    int pack_len    =   0;
    int crc16       =   0;
    int byte        =   0;
    char *command;
    char **pptr;
    char *host;
    const char *ipv4;
    struct hostent *infor;

    signal(SIGKILL,sig_handler);

    struct option opts[]={
        {"host" ,required_argument  , NULL,'i'},
        {"port" ,required_argument  , NULL,'p'},
        {"time space" ,required_argument  , NULL,'t'},
        {"daemon", no_argument, NULL, 'd'},
        {"help" ,no_argument        , NULL,'h'},
    };

    while((opt = getopt_long(argc, argv, "i:p:t:dh", opts, NULL)) != -1)
    {        
        switch(opt)
        {
            case 'i':
                host = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 't':
                time_space = atoi(optarg);
                break;
            case 'd':
                daemon(0,0);
            case 'h':
                printf("usage:-i[--host] -p[--port] -t[--time] -h[help]\n");
                break;
        }
    }
    
    if(!host||!port||!time_space)
    {
        printf_usage(argv[0]);
        return 0;
    }

    infor = gethostbyname(host);
    pptr = infor->h_addr_list;
    char buf[16];
    for(; *pptr != NULL;pptr++)
        ipv4 = inet_ntop(AF_INET , *pptr , buf , 16);

     if((fd = sock_init(ipv4, port)) < 0)
         return 1;


    printf("who are you :");
    scanf("%s",id_buf);
    /* if the id too large */
    while((!p_stop) && (strlen(id_buf) > 0) && (sizeof(pack_buf) > (strlen(id_buf)+MIN_SIZE)))
    {
        memset(time_buf, 0, sizeof(time_buf));
        memset(pack_buf, 0, sizeof(pack_buf));
        /* id part */
        pack_buf[pointer] = (unsigned char)THE_HEAD;// head
        pointer += 1;

        pack_buf[pointer] = LHJ;
        pointer += 1;

        datalen = strlen(id_buf)+6+2+2;
        pack_buf[pointer] = datalen;//lenth
        pointer += 1;
           
        datalen = strlen(id_buf);
        memcpy(pack_buf+pointer, id_buf, datalen);
        pointer += datalen;//value
        
        pack_buf[pointer] = '|';
        pointer += 1;
        /* time part */
        int byte = get_sys_time(time_buf);
        datalen = byte;
        //memcpy(pack_buf+pointer, time_buf, datalen);
            
        for(int i=0;i<datalen;i++,pointer++)
            pack_buf[pointer] = time_buf[i];
        /* temper part */
        pack_buf[pointer] = '|';
            pointer += 1;
        float temper = ds18b20_get_temper();
        int temper_p1;
        int temper_p2;
        temper_p1 = (int)temper;// integer 1 byte
        if(temper_p1 > temper)
            temper_p1 -= 1;

        temper_p2 = (int)((temper-temper_p1)*100);// decimal 1 byte
        if((sizeof(pack_buf)-pointer) < (2+2))
        {
            printf("have no more space to put temperature!\n");
            return 1;
        }
        

        pack_buf[pointer] = temper_p1;
        pointer += 1;
        pack_buf[pointer] = temper_p2;
        pointer += 1;

        crc16 = crc_itu_t(MAGIC_CRC, pack_buf, pointer);//CRC
        ushort_to_bytes(&pack_buf[pointer], crc16);
        pointer += 2;
        pack_len = pointer;

        printf("P:%d\n",pointer);
        send_temper(pack_buf, fd,pack_len);
        pointer = 0;
        sleep(time_space);
    }

}

