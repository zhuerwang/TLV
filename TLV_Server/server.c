/*********************************************************************************
 *      Copyright:  (C) 2019 454256587<454256587@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  server.c
 *    Description:  This file is a server of tlv 
 *    
 *         Version:  1.0.0(01/04/19)
 *          Author:  454256587 <454256587@qq.com>
 *       ChangeLog:  1, Release initial version on "01/04/19 15:47:58"
 *                
 *********************************************************************************/
#include <stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include "sock_init.h"
#include "byte_to_str.h"
#include "get_true_msg.h"

#define MAX_EVENTS 1024
#define SIZE 64


int p_stop  =   0;

void sig_handler(int sig_t);
void printf_usage(char *command);

void sig_handler(int sig_t)
{
    if(SIGKILL==sig_t)
        p_stop=1;
}


int main(int argc,char *argv[])
{

    struct epoll_event event;
    struct epoll_event event_array[MAX_EVENTS];

    char *ip;
    char *command;
    int i           =   0;
    int port        =   0;
    int opt         =   -1;
    int daemon_run  =   0;
    int epoll_fd    =   -1;
    int sock_fd     =   -1;
    int new_fd      =   -1;
    int events      =   -1;
    int rv          =   -1;
    int on          =   1;
    int a           =   0;
    int *flag       =   &a;

    sqlite3 *db;
    char *zErrMsg;
    int rc  =   0;
    char sql[512];
    char result[3][32];

   
    signal(SIGKILL,sig_handler);

    struct option opts[]=
    {
        {"DAEMON"   ,no_argument        ,NULL,'d'},
        {"ip"     ,required_argument  ,NULL,'i'},
        {"PORT"     ,required_argument  ,NULL,'p'},   
        {"help"     ,no_argument        ,NULL,'h'},
        {NULL,0,NULL,0}
    };

    while((opt = getopt_long( argc, argv, "di::p:h",opts, NULL))!=-1)
    {
        switch(opt)
        {
            case 'd':
                daemon_run = atoi(optarg);
                break;
            case 'i':
                ip = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'h':
                printf_usage(argv[0]);
                return 0;
        }
    }


    if( !port )
    {
        printf("usage:%s[-d DAEMON] [-i IP] [-p port] [-h help]\n",argv[0]);
        return 0;
    }
    
    if(daemon_run)
        daemon(0,0);


    rc = sqlite3_open("test.db", &db);
    if( rc )
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(0);
    }
    else
    {
        fprintf(stdout, "Opened database successfully!\n");
    }

    char *sql_s =   "CREATE TABLE IF NOT EXISTS STUDENT(" \
            "NUMBER  INTEGER PRIMARY KEY     AUTOINCREMENT,"\
            "ID      CHAR(64)    NOT NULL,"\
            "TIME    CHAR(64)    NOT NULL,"\
            "TEMPER  CHAR(64)    NOT NULL);";

    rc = sqlite3_exec(db, sql_s, 0, 0, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Table created successfully\n");
    }
    sqlite3_close(db);

    while(!p_stop)
    {   
        if((sock_fd=sock_init(ip,port))<0)
        {
            printf("create socket_fd failure:%s\n",strerror(errno));
            return -1;
        }

        setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        
        if((epoll_fd=epoll_create(MAX_EVENTS))<0)
        {
            printf("create epoll_fd failure:%s\n",strerror(errno));
            return -1;
        }
        event.events=EPOLLIN;
        event.data.fd=sock_fd;
        if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,sock_fd,&event)<0)
        {
            printf("add socket fd failure:%s\n",strerror(errno));
            return -1;
        }

        for(;;)
        {
            events=epoll_wait(epoll_fd,event_array,MAX_EVENTS,-1);
            if(events<0)//have no events happend
            {
                printf("epoll failure:%s\n",strerror(errno));
                break;
            }
            else if(events==0)
            {
                printf("epoll get timeout\n");
                continue;
            }

            for(i=0;i<events;i++)
            {
                if((event_array[i].events&EPOLLERR)||(event_array[i].events&EPOLLHUP))
                {
                    printf("there is trouble with fd[%d]:%s\n",event_array[i].data.fd,strerror(errno));
                    epoll_ctl(epoll_fd ,EPOLL_CTL_DEL ,event_array[i].data.fd ,NULL );
                    close(event_array[i].data.fd);
                }
            
                if(event_array[i].data.fd == sock_fd)
                {
                    if((new_fd=accept(sock_fd ,(struct sockaddr *) NULL ,NULL))<0)
                    {

                        printf("create new fd failure:%s\n",strerror(errno));
                        continue;
                    }
                    
                    event.events=EPOLLIN;
                    event.data.fd=new_fd;
                    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,new_fd,&event)<0)
                    {
                        printf("epoll add new fd failure:%s\n",strerror(errno));
                        close(event_array[i].data.fd);
                        continue;
                    }
                    printf("epoll add new fd successfully!\n");
                }
                else
                {
                    char buf[SIZE];
                    char true_buf[SIZE];

                    if((rv=read(event_array[i].data.fd,&buf[*flag],sizeof(buf)))<=0)
                    {
                        printf("read from socket failure or disconnect:%s\n",strerror(errno));
                        close(event_array[i].data.fd);
                        continue;
                    }
                    else
                    {
                        printf("read %d byte from new fd[%d]\n",rv,event_array[i].data.fd);
                        *flag += rv;

                        int byte;
                        byte = get_true_msg(flag, buf, true_buf);

                        if(byte > 0 )
                        {
                            printf("analysis %d bytes.\n",byte);
                            byte_to_str(true_buf,result);
                        }
                        else
                        {
                            printf("got false message.\n");
                            break;
                        }
                        

                        rc = sqlite3_open("test.db", &db);
                        if( rc )
                            exit(0);
                        else
                            fprintf(stdout, "Opened database successfully!\n");

                        //strcpy(result[0],strtok(str_buf,delims));
                        //strcpy(result[1],strtok(NULL,delims));
                        //strcpy(result[2],strtok(NULL,delims));
                        snprintf(sql,sizeof(sql),"INSERT INTO STUDENT (NUMBER,ID,TIME,TEMPER) VALUES (NULL,'%s','%s','%s');", 
                                                result[0],result[1],result[2]);
                            
                        printf("%s\n",sql);
                        rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
                        if( rc != SQLITE_OK )
                        {
                            fprintf(stderr, "SQL error: %s\n", zErrMsg);
                            sqlite3_free(zErrMsg);
                        }
                        else
                            fprintf(stdout, "Records created successfully\n");
                        sqlite3_close(db);
                        
                    }
                
                }
            }

        }

    }


}

void printf_usage(char *command)
{
    printf("%susage:\n",command);
    printf("-d(--daemon):put process into background");
    printf("-i(--IP):sepcify server IP address.\n");
    printf("-p(--port):sepcify port\n");
    printf("-h(--help):print this help information.\n");
}
