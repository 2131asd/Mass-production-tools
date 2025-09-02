#ifndef __disp_mannager_h //防止头文件重复包含
#define __disp_mannager_h

#include <common.h>

#ifndef  NULL
#define NULL (void *) 0
#endif


/*
显示缓冲区结构体
*/
typedef struct dispbuff{
    int ixres;      // X 方向分辨率（像素数，如 800）
    int iyres;      // Y 方向分辨率（像素数，如 480）
    int ibpp;       // 每个像素的位数（如 32 表示 RGBA8888）
    char * buff;    // 缓冲区首地址（映射到用户空间的帧缓冲内存）
}dispbuff,*pdispbuff;

/*
common.h中有了不用再定义
//显示区域结构体
typedef struct region{
    int x;// 区域左上角 X 坐标
    int y; // 区域左上角 Y 坐标
    int width;// 区域宽度（像素数）
    int height; // 区域高度（像素数）
} region,*pregion;
*/

//显示操作结构体
//包含显示操作的函数指针和相关数据
typedef struct dispopr{
    char *name;
    int (*deviceinit)(void);//结构体初始化
    int (*deviceexit)(void);//清理结构体
    int (*getbuffer)(pdispbuff ptdispbuff);//获取帧缓冲区,XY坐标和像素B
    int (*flushregion)(p_region ptregion, pdispbuff ptdispbuff);// 刷新区域：将 buffer 内容刷到 ptregion 描述的区域
    struct dispopr *ptnext;//链表指针，用于挂载多个显示设备（如同时支持LCD、虚拟屏）
}dispopr,*pdispopr;


void registerdisplay(pdispopr ptdispopr);
void display_system_register(void);
int selectdefaultdisplay(char *name);
int initdefaultdisplay(void);
int putpixel(int x, int y, unsigned int dwcolor);
int flushdisplayregion(p_region ptregion, pdispbuff ptdispbuff);

int getdisplaybuffer(void);

void drawfontbitmap(p_fontbitmap pt_fontbitmap,unsigned int dwcolor);
void draw_region(p_region pt_region,unsigned int dwcolor);
void drawtext_inregioncentral(char *name, p_region pt_region, unsigned int dwcolor);

#endif

