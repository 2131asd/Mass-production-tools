#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <input_manager.h>

int main(int argc,char **argv)
{
    int ret;
    inputevent event;
    input_init();
    input_deviceinit();
    while(1)
    {
        ret = get_inputevent(&event);
        if(ret)
        {
            printf("get_inputevent err!\n");
            return -1;
        }
        else
        {
            if(event.i_type == INPUT_TYPE_TOUCH)
            {
                printf("tyep      :%d\n",event.i_type);
                printf("x         :%d\n",event.i_x);
                printf("y         :%d\n",event.i_y);
                printf("pressure  :%d\n",event.i_pressure);
            }
            else if(event.i_type == INPUT_TYPE_NET)
            {
                printf("tyep  :%d\n",event.i_type);
                printf("str   :%d\n",event.str);
            }           
        } 
    }
    return 0;
}