#ifndef __font_manager_h
#define __font_manager_h
#include  <sys/time.h>// 时间相关结构体（如 struct timeval，虽未显式用，为系统基础依赖兜底）

#include <common.h>



/*
位图数据结构体
统一描述 “单个字符的位图信息”，将字体的 “绘制位置”“点阵数据”“下一个字符位置” 封装为结构体，供上层绘制函数（如 drawfontbitmap）使用。
*/
typedef struct fontbitmap
{
    region t_region; // 字符位图的矩形区域（x/y 坐标、宽/高，描述字符在屏幕上的绘制范围）
    int i_cur_originx;//基点的x
    int i_cur_originy;//基点的y
    int i_next_originx;//下一个基点的x
    int i_next_originy;//下一个基点的y
    unsigned char *puc_buffer; //存储的是字符的位图点阵，而非字符串。 
}fontbitmap,*p_fontbitmap;

// FreeType结构体，字体操作的标准接口，用于抽象不同字体库（如 FreeType、其他字体引擎）的实现
typedef struct fontopr
{
    char *name; 
    int (*fontinit)(char *afinename);//初始化函数
    int (*setfontsize)(int ifontsize);//设置字体大小
    int (*getfontbitmap)(unsigned int dwcode,p_fontbitmap pt_fontbitmap);// 获取字符位图
    int (*getstring_regioncar)(char *str , p_region_cartesian pt_regioncar);// 获取字符串外框
    struct fontopr *pt_next;
}fontopr,*p_fontopr;

void registerfont(p_fontopr  pt_fontopr);
void font_system_register(void);

int selectandinitfont(char *a_fontoprname ,char *a_fontfilename);
int setfontsize(int i_fontsize);
int getfontbitmap(unsigned int dwcode ,p_fontbitmap pt_fontbitmap);

int getstring_regioncar(char *str , p_region_cartesian pt_regioncar);


#endif
