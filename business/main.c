#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <disp_manager.h>
#include <stdlib.h>

#include <disp_manager.h>
#include <font_manager.h>
#include <page_manager.h>
#include <input_manager.h>
#include <ui.h>
#include <common.h>

int main(int argc,char **argv)
{
    int error;
    
    if(argc !=2)
    {
        printf("usage:%s <font_size>" ,argv[0]);
        return -1;
    }

    //初始化显示系统
    display_system_register(); //以前是displayinit();
    // 初始化显示设备管理器
    selectdefaultdisplay("fb"); // 选择默认显示设备（这里是"fb"，即帧缓冲设备）
    initdefaultdisplay();// 初始化默认显示设备（如打开/dev/fb0）

    //初始化输入系统
    input_system_register();//以前是input_init();

    input_deviceinit();

    //初始化文字系统 ， 注册所有字体引擎（这里会注册FreeType）
    font_system_register();//以前为fontsregister();

    //选择并初始化字体引擎
    error = selectandinitfont("freetype", argv[1]);
    if(error)
    {
        printf("selectandinitfont err\n");
        return -1;
    }

    //初始化页面系统
    page_system_register();//以前是page_register();

    //运行业务系统的主页面
    page("main")->run(NULL); // 调用 main 页面动作的执行函数
   
    return 0;
}
