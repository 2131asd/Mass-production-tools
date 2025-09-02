/*
该文件是触摸屏输入设备的驱动抽象层，核心目标是借助 tslib 库简化触摸屏操作，并将触摸数据封装为系统统一的输入事件，解耦上层业务与底层硬件

具体体现为：
简化硬件操作：通过 tslib 屏蔽触摸屏底层细节（如设备路径、坐标校准、驱动差异），避免直接操作 /dev/input/eventX 的复杂逻辑
            （如解析 input_event 结构体、处理多触点）；
统一输入接口：将触摸事件转换为 inputevent 格式，与网络输入、按键输入对齐，上层业务（如 UI 点击、滑动响应）无需修改代码即可适配触摸屏；
可管理与可扩展：通过 touchscreen_register 注册设备，输入管理器可动态管理多个输入设备（如同时启用触摸屏和按键）；
            新增输入设备时，只需按 inputdevice 接口实现并注册，无需改动核心逻辑；
资源安全：touchscreen_deviceinit 与 touchscreen_deviceexit 对称设计，确保初始化的触摸屏资源能被正确释放，避免设备文件泄漏。
五、文件所在层级
结合项目架构（参考输入 / 显示系统的分层设计），该文件属于输入驱动抽象层，位于：

下层：对接触摸屏硬件和 tslib 库（依赖 tslib 实现底层触摸数据读取、校准），是 “硬件能力到软件接口” 的桥梁；
上层：为输入管理器（input_manager.c）提供标准的 inputdevice 接口，支撑上层业务（如 UI 交互、触摸控制）。

*/

#include <tslib.h> // tslib 触摸屏库头文件（提供触摸屏初始化、读取、关闭等函数）

#include <stdio.h> // 标准输入输出（printf 调试信息）

#include <input_manager.h>

static struct tsdev *g_ts;//触摸设备句柄（tslib 库定义，代表触摸屏设备）

//设备初始化，获得触摸屏句柄 （输入设备初始化流程2.1）
static int touchscreen_deviceinit(void)
{
    // 调用 tslib 的 ts_setup 初始化触摸屏
    //参数1=设备名（NULL 表示自动探测，如 /dev/input/event0），参数2=非阻塞模式（0 表示阻塞）
    g_ts = ts_setup(NULL,0);
    if(!g_ts)
    {
        printf("ts_setup err\n");
        return -1;
    }
    return 0;
}


/*可以不写，为了对照结构体类型。触摸点数据结构（tslib 定义，存储一次触摸的坐标、压力、时间）
struct ts_sample
{
    int x;
    int y;
    unsigned int pressure;
    struct timeval tv;
}
*/

/*
触摸事件读取函数 (输入设备初始化 3.1)
读取一个触摸样本，向上报的数据结构中填充事件类型、坐标、压力、时间戳  
输入参数：上报的数据结构
*/
static int touchscreen_getinputevent(p_inputevent pt_inputevent)
{
    struct ts_sample samp;
    int ret;
    // 读取触摸数据：调用 tslib 的 ts_read，填充 samp
    ret = ts_read(g_ts,&samp,1);
    if(ret != 1)
    {
        return -1;
    }
    pt_inputevent->i_type   = INPUT_TYPE_TOUCH;
    pt_inputevent->i_x      = samp.x;
    pt_inputevent->i_y      = samp.y;
    pt_inputevent->i_pressure = samp.pressure;
    pt_inputevent->tTime      = samp.tv;
    return 0;
}

/*
设备清除
*/
static int touchscreen_deviceexit(void)
{
    ts_close(g_ts);
    return 0;
}

/*
触摸屏设备接口封装（inputdevice 结构体）
*/ 
static inputdevice g_t_touchscreen_dev ={
    .name = "touchscreen",                         // 设备名称："touchscreen" （上层按名称选择设备）
    .deviceinit         = touchscreen_deviceinit,      //绑定“设备初始化”函数（启动时调用）
    .get_inputevent     = touchscreen_getinputevent,   //绑定“读取事件”函数（核心功能）   
    .deviceexit         = touchscreen_deviceexit,      // 绑定“设备退出”函数 （关闭时调用）
};

//注册触摸屏结构体  （输入初始化流程1.1）
void touchscreen_register(void)
{
    register_inputdecive(&g_t_touchscreen_dev);
}

//测试是否能接收到数据
#if 0
int main(int argc ,char *argv)
{
    inputevent event;
    int ret;

    g_t_touchscreen_dev.deviceinit();
    while(1)
    {
        ret = g_t_touchscreen_dev.get_inputevent(&event);
        if(ret)
        {
            printf("getinputevent err!\n");
            return -1;
        }
        else
        {
            printf("type        :%d\n",event.i_type);
            printf("x           :%d\n",event.i_x);
            printf("i           :%d\n",event.i_y);
            printf("pressure    :%d\n",event.i_pressure);
        }  
    }
    return 0;
}    
#endif


