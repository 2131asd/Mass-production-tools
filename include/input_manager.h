#ifndef __input_manager_h
#define __input_manager_h
#include  <sys/time.h>
#include <common.h>
#define INPUT_TYPE_TOUCH 1
#define INPUT_TYPE_NET 2

#ifndef  NULL
#define NULL (void *) 0
#endif

//上报的数据结构
typedef struct inputevent
{
    struct timeval tTime;
    int i_type;
    int i_x;
    int i_y;
    int i_pressure;
    char str[1024];
}inputevent,*p_inputevent;

//输入设备的数据结构
typedef struct inputdevice
{
    char *name;
    int (*get_inputevent)(p_inputevent pt_inputevent);
    int (*deviceinit)(void);
    int (*deviceexit)(void);
    struct inputdevice *pt_next;
}inputdevice,*p_inputdevice;

void register_inputdevice(p_inputdevice pt_inputdev);
void input_system_register(void);
void input_deviceinit(void);
int get_inputevent(p_inputevent pt_inputevent);

#endif
