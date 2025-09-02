/*
这些函数构成了显示核心管理中间层，核心目标是解耦上层业务与底层硬件，具体体现为：

设备无关性：通过 dispopr 链表和 g_dispdefault，上层可透明切换显示设备（如从帧缓冲切换到虚拟屏），无需修改业务代码。
功能封装：将复杂的硬件操作（如像素格式转换、内存地址计算）封装在 putpixel 等函数中，上层只需调用简单接口即可实现绘制。
可扩展性：新增显示设备时，只需实现 dispopr 接口并注册，无需修改中间层；新增绘制功能（如绘制图片）只需添加类似 draw_region 的函数。
流程标准化：统一 “注册→选择→初始化→绘制→刷新” 的显示流程，降低开发复杂度。
四、文件所在层级
结合项目架构（参考之前的目录分析），该文件属于中间层（显示管理层），位于：

下层：对接硬件驱动（如 framebuffer.c 实现的帧缓冲设备），通过 dispopr 接口调用具体设备的初始化、刷新等方法。
上层：为业务层（如页面管理 page_manager.c、UI 组件）提供绘制接口（如画点、文本、区域），支撑具体的界面显示功能。

是连接硬件驱动与业务逻辑的核心枢纽，实现了 “硬件抽象” 和 “功能复用”
向下：对接具体字体模块（如 FreeType），通过 registerfont 接收模块注册，通过 g_pt_defaultfontopr 调用模块接口；
向上：为上层提供标准化的字体操作入口，屏蔽底层差异，是字体系统的 “中枢神经”。
 */

#include <stdio.h>
#include <disp_manager.h>
#include <font_manager.h>
#include  <common.h>

/*管理底层的LCD、WEB*/
static pdispopr g_dispdevs = NULL;// 显示设备链表头指针（管理所有注册的显示设备）
static pdispopr g_dispdefault = NULL;// 默认选中的显示设备（上层实际使用的设备）
static dispbuff g_tdispbuff;// 全局显示缓冲区（存储屏幕像素数据）
static int line_width; // 屏幕每行的字节数
static int pixel_width; // 每个像素的字节数（如32位像素为4字节）


/*
@1 显示系统注册（注册framebuffer）
调用后会自动调用其他两个函数把framebuffer结构体指针放入链表
让当前文件的g_dispdevs变为framebuffer结构体
framebuffer结构体的ptnext为空
*/ 
void display_system_register(void)
{
    extern void framebuffer_register(void);
    framebuffer_register();      
}
// @1.3  把底层实现的结构体放入链表
//也就把底层构造的framebuffer操作接口地址赋值给显示设备链表头指针g_dispdevs
//让中间层能调用底层实现的framebuffer操作接口
void registerdisplay(pdispopr ptdispopr)
{
    ptdispopr->ptnext = g_dispdevs;
    g_dispdevs = ptdispopr; 
}



/*
@2 
根据名字选择默认的显示器链表
*/ 
int selectdefaultdisplay(char *name)
{
    pdispopr ptmp = g_dispdevs;
    while(ptmp)
    {
        if(strcmp(ptmp->name, name) == 0)
        {
            // 找到匹配的显示设备
            g_dispdefault = ptmp;
            return 0;
        }
        ptmp = ptmp->ptnext;
    }
    return -1; // 未找到匹配的显示设备
}

/*
@ 3 
初始化显示设备
得到屏幕各种信息
根据framebuffer设备参数填充全局显示缓冲区
通过全局显示缓冲区进而获得屏幕每行的字节数，每个像素字节数
*/  
int initdefaultdisplay(void)
{
    int ret;
    ret = g_dispdefault->deviceinit();
    if(ret)
    {
        printf("Failed to init device\n");
        return -1;
    }
    ret = g_dispdefault->getbuffer(&g_tdispbuff); 
    if(ret)
    {
        printf("Failed to get buffer\n");
        return -1;
    }

    line_width = g_tdispbuff.ixres * g_tdispbuff.ibpp / 8;
    pixel_width = g_tdispbuff.ibpp / 8;
    return 0;
}



/*
@4 
获取显示缓冲区
输出参数：显示缓冲区指针
*/ 
int getdisplaybuffer(void)
{
    return &g_tdispbuff;
}


/*
@5   
绘制像素
输入参数：x坐标，y坐标，颜色
*/ 
int putpixel(int x, int y, unsigned int dwcolor)
{
    // 根据输入的的（x，y）计算(x,y)坐标在缓冲区中的内存地址（起始地址 + 行偏移 + 列偏移）
    unsigned char *pen_8 = (unsigned char *)g_tdispbuff.buff + y * line_width + x * pixel_width;  //8位像素指针
    unsigned short *pen_16 ;
    unsigned int *pen_32;

    unsigned int red,green,blue;
    pen_16 = (unsigned short *)pen_8;// 16位像素指针
    pen_32 = (unsigned int *)pen_8;// 32位像素指针

    switch (g_tdispbuff.ibpp)
    {
        case 8:
        {
            *pen_8 = dwcolor;
            break;
        }
        case 16:
        {
            red = (dwcolor >> 16) & 0xFF;
            green = (dwcolor >> 8) & 0xFF;
            blue = (dwcolor >> 0) & 0xFF;
            dwcolor = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
            *pen_16 = dwcolor;  
            break;
        }
        case 32:
        {
            *pen_32 = dwcolor;
            break;
        }
        default:
        {
            printf("Unsupported pixel format: %d bits per pixel\n", g_tdispbuff.ibpp);
            return -1;
            break;
        }
    }
    return 0;
}

/*
@5 
绘制字符，到位图缓冲区
输入参数：位图数据结构体指针，颜色
*/ 

void drawfontbitmap(p_fontbitmap pt_fontbitmap,unsigned int dwcolor)
{
    int i,j,p,q;
    // 获取字符绘制的区域坐标（左上角x,y）和区域范围（x_max,y_max）
    int x = pt_fontbitmap->t_region.x;
    int y = pt_fontbitmap->t_region.y;
    int x_max = x + pt_fontbitmap->t_region.width;
    int y_max = y + pt_fontbitmap->t_region.height;
    int width = pt_fontbitmap->t_region.width;
    unsigned char *buffer = pt_fontbitmap->puc_buffer;

    //遍历字符区域的每个像素，根据位图数据绘制
    for(j = y,q = 0;j < y_max;j++,q++)
    {
        for(i = x,p = 0;i < x_max;i++,p++)
        {
            if(i < 0    || j < 0  ||
                i >=g_tdispbuff.ixres || j >= g_tdispbuff.iyres)
            continue;

            if(buffer[q*width + p])
                putpixel(i,j,dwcolor);
        }
    }
}

/*
@5 
绘制显示区域,默认初始颜色为dwcolor色
输入参数：绘制区域指针，绘制颜色
*/
void draw_region(p_region pt_region,unsigned int dwcolor)
{
    int x = pt_region->x;
    int y = pt_region->y;
    int width = pt_region->width;
    int height = pt_region->height;
    int i, j;
    for (j = y; j < y + height; j++)
    {
        for (i = x; i < x + width; i++)
        {
            putpixel(i, j, dwcolor);
        }
    }
}

/*
@5 
绘制字符在区域中央
输入参数：字符（字符串指针），绘制区域指针，颜色
*/
void drawtext_inregioncentral(char *name, p_region pt_region, unsigned int dwcolor)
{
    fontbitmap t_fontbitmap; // 字符位图结构体（存储单个字符的位图信息）
    region_cartesian t_regioncar;//显示区域结构体（笛卡尔坐标系）

    int i_originx,i_originy;
    int i = 0;
    int error;

    //计算字符串的外框
    getstring_regioncar(name,&t_regioncar);

    i_originx = pt_region->x + (pt_region->width - t_regioncar.width) / 2 - t_regioncar.x;
    i_originy = pt_region->y + (pt_region->height - t_regioncar.height) / 2 + t_regioncar.y;

    //逐个绘制
    while(name[i])
    {
        // 1. 设置当前字符的绘制基点（起始坐标）
        t_fontbitmap.i_cur_originx = i_originx;
        t_fontbitmap.i_cur_originy = i_originy;
        // 2. 获取当前字符的位图信息，存放在t_fontbitmap（函数内部会自动计算下一个基点并写入i_next_originx/y）
        error = getfontbitmap(name[i],&t_fontbitmap);
        if(error)
        {
            printf("selectandinitfont err\n");
            return -1;
        }
        // 3. 绘制字符到位图缓冲区
        drawfontbitmap(&t_fontbitmap,dwcolor);
        
        i_originx = t_fontbitmap.i_next_originx;
        i_originy = t_fontbitmap.i_next_originy;
        i++;
    }
}

/*
@6  把绘制好的区域刷到硬件上
输入参数：显示区域指针，显示缓冲区指针
*/
int flushdisplayregion(p_region ptregion, pdispbuff ptdispbuff)
{
    return g_dispdefault->flushregion(ptregion, ptdispbuff);
}
