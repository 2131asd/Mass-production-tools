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

#define FONTDATAMAX 4096



int main(int argc,char **argv)
{
    pdispbuff pt_buffer; // 显示缓冲区指针（存储待显示的像素数据）
    int error;
     
    fontbitmap t_fontbitmap;// 字符位图结构体（存储单个字符的位图信息）
    char *str = "www.100ask.net"; // 待显示的字符串
    int i = 0; // 字符串索引（用于遍历字符）
    int lcd_x; // 字符串显示的起始坐标（x、y）
    int lcd_y;
    int font_size; // 字体大小（像素）

    //检查命令行参数数量是否正确（必须传入 4 个参数）
    //argv[0]：程序自身路径。
    //argv[1]：字体文件路径（如 ./simhei.ttf）。
    //argv[2]：字符串显示的起始 X 坐标（如 100）。
    //argv[3]：字符串显示的起始 Y 坐标（如 200）。
    //argv[4]：字体大小（如 24 像素）。
    if(argc != 5)
    {
        printd("usage:%s <font_file> <lcd_x> <lcd_y> <font_size>\n",argv[0]);
        return -1;
    }

    //strtol 函数：将字符串转换为长整数（支持十进制、八进制、十六进制，由 0 自动识别）
    //若 argv[2] 是 "100"，则 lcd_x 被赋值为 100。
    lcd_x = strtol(argv[2],NILL,0);
    lcd_y = strtol(argv[3],NILL,0);
    font_size = strtol(argv[4],NILL,0);
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
    // 设置字体大小（如24像素）
    setfontsize(font_size);
    //遍历字符串并逐个渲染字符
    while(str[i])
    {
        // 1. 设置当前字符的绘制基点（起始坐标）
        t_fontbitmap.i_cur_originx = lcd_x;
        t_fontbitmap.i_cur_originy = lcd_y;
        // 2. 获取当前字符的位图信息（函数内部会计算下一个基点并写入i_next_originx/y）
        error = getfontbitmap(str[i],&t_fontbitmap);
        if(error)
        {
            printf("selectandinitfont err\n");
            return -1;
        }
        // 3. 绘制字符到位图缓冲区（颜色为0xff0000，即红色）
        drawfontbitmap(&t_fontbitmap,0xff0000);
        // 4. 将字符所在区域刷新到LCD屏幕
        flushdisplayregion(&t_fontbitmap.t_region,pt_buffer);

        lcd_x = t_fontbitmap.i_next_originx;
        lcd_y = t_fontbitmap.i_next_originy;
        i++;
    }
    return 0;
}
