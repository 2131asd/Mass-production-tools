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
#include <ui.h>

#define FONTDATAMAX 4096



int main(int argc,char **argv)
{
    pdispbuff pt_buffer; // 显示缓冲区指针（存储待显示的像素数据）
    int error;
    button t_button; // 按钮结构体实例
    region t_region; // 按钮区域（x, y, width, height）

    //检查命令行参数数量是否正确（必须传入 2 个参数）
    if(argc != 2)
    {
        printd("usage:%s <font_size> \n",argv[0]);
        return -1;
    }

    displayinit();// 初始化显示设备管理器
    selectdefaultdisplay("fb"); // 选择默认显示设备（这里是"fb"，即帧缓冲设备）
    initdefaultdisplay();// 初始化默认显示设备（如打开/dev/fb0）
    pt_buffer = getdisplaybuffer();// 获取显示缓冲区（用于绘制像素数据）

    // 注册所有字体引擎（这里会注册FreeType）
    fontsregister(); 
    // 选择并初始化FreeType字体引擎，加载指定的字体文件
    error = selectandinitfont("freetype",argv[1]);
    if(error)
    {
        printf("selectandinitfont err\n");
        return -1;
    }

    t_region.x = 200;
    t_region.y = 200;
    t_region.width = 300;
    t_region.height = 100;

    init_button(&t_button, "text", &t_region, NULL, NULL); // 初始化按钮，使用默认的绘制和按下函数
    t_button.on_draw(&t_button, pt_buffer); // 绘制按钮
    while(1)
    {
        t_button.on_pressed(&t_button, pt_buffer, NULL); // 按下按钮
        sleep(2); // 每秒钟按下一次按钮
        
    }
    return 0;
}
