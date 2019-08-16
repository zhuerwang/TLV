

#include <stdio.h>
#include <time.h>
#include <string.h>
#include "get_time.h"

int get_sys_time(char *time_buf, int size)
{
    time_t time_s;
    time(&time_s);

    memset(time_buf, 0,size);
    strcpy(time_buf, ctime(&timep));
    
    return strlen(ctime(&timep));

}
