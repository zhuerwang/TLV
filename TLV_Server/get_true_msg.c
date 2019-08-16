/*********************************************************************************
 *      Copyright:  (C) 2019 LingYun<lingyun@email.com>
 *                  All rights reserved.
 *
 *       Filename:  get_true_msg.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(02/04/19)
 *         Author:  LingYun <lingyun@email.com>
 *      ChangeLog:  1, Release initial version on "02/04/19 17:20:13"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <string.h>
#include "crc-itu-t.h"
#include "get_true_msg.h"

#define SIZE 64
#define MINSIZE 6

int get_true_msg(int *flag,char buf[SIZE],char true_buf[SIZE])
{
    int len         = 0;
    int byte        = 0;
    int crc         = 0;
    int crc_ago     = 0;


    for(int i=0;i<SIZE;i++)
    {
        if(buf[i] == 253)
        {
            if(*flag-i < MINSIZE)//if leave space < minsize 
            {
                memmove(buf, &buf[i], *flag-i);
                *flag = *flag-i;
                printf("get incomplete packet.\n");
                break;
            }
            else if(buf[i+1] == 170)//tag
            {
                len = buf[i+2];
                printf("%d\n",len);
                if(len+5 > *flag-i)//the message is incomplete
                {
                    memmove(buf, &buf[i], *flag-i);
                    *flag = *flag - i;
                    printf("get incomplete packet.\n");
                    break;
                }
                else//the message is complete and we will confirm if the crc right
                {
                    crc = crc_itu_t(MAGIC_CRC, &buf[i], len+3);
                    crc_ago = bytes_to_ushort(&buf[i+3+len],2);
                    
                    if(crc != crc_ago)
                    {
                        printf("crc is not match.");
                        continue;
                    }
                    else
                    {
                        memmove(true_buf,&buf[i+3],len);
                        byte += len;
                        continue;
                    }
                }
            }
        }
        *flag = 0;
        return byte;
    }
}
