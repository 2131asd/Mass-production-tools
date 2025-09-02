/*
底层驱动/硬件抽象层，直接操作linux帧缓冲设备（/dev/fb0）
封装了硬件操作细节（打开设备、内存映射、参数获取）
向上提供统一的显示操作接口 dispopr g_tframebufferopr，供上层应用
上层只需要调用接口无需知道底层怎么实现
*/
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>


#include <disp_manager.h>

static int fd_fb;                           // 帧缓冲设备文件描述符（/dev/fb0）
/*
 * struct fb_var_screeninfo {
    unsigned int xres;          // 屏幕X方向分辨率（像素）
    unsigned int yres;          // 屏幕Y方向分辨率（像素）
    unsigned int bits_per_pixel;// 每个像素的位数（如32位、24位）
    // 其他参数：像素格式（红/绿/蓝分量的位偏移和长度）、屏幕时序等
};
定义在 <linux/fb.h> 中
*/
static struct fb_var_screeninfo var ;       // 屏幕可变信息（分辨率、像素格式等）
static int screen_size;                     // 屏幕缓冲区总大小（字节）
static unsigned char *fb_base;              // 映射到用户空间的帧缓冲内存首地址
static unsigned int line_width;             // 屏幕每行的字节数
static unsigned int pixel_width;            // 每个像素的字节数（如32位像素为4字节）


/*
设备初始化
得到屏幕可变信息存放在var中
进而计算出参数：screen_size , fb_base , line_width , pixel_width
*/
static int fbdeviceinit(void)
{
    // 1. 打开帧缓冲设备文件 /dev/fb0
    // 可读可写方式打开
    fd_fb = open("/dev/fb0", O_RDWR);
    if (fd_fb < 0) 
    {
        printf("can't open /dev/fb0\n");
        return -1;
    }

    // 2. 获取屏幕可变信息（分辨率、像素位数等）
    //调用 ioctl 向 /dev/fb0 设备发送 FBIOGET_VSCREENINFO 命令，请求获取屏幕可变信息，并通过 &var 接收结果
    if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &var) )
    {
        printf("can't get var\n");
        return -1;
    }

    // 3. 计算屏幕缓冲区总大小（y 方向分辨率 × x 方向分辨率 × 每个像素位数 ÷ 8 转换为字节）
    screen_size = var.yres * var.xres * var.bits_per_pixel / 8;
    // 4. 内存映射：将帧缓冲设备内存映射到用户空间，PROT_READ|PROT_WRITE 表示可读写，MAP_SHARED 表示共享映射
    fb_base = (unsigned char*)mmap(NULL, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
    line_width = var.xres * var.bits_per_pixel / 8;
    pixel_width = var.bits_per_pixel / 8;
    // 7. 映射失败判断（mmap 返回 (void*)-1 表示失败）
    if(fb_base == (unsigned char *) -1)
    {
        printf("can't mmap\n");
        return -1;
    }
    return 0;
}

/*
设备退出清理
*/
static int fbdeviceexit(void)
{
    munmap(fb_base, screen_size);// 解除内存映射，释放用户空间映射的帧缓冲内存
    close(fd_fb);// 关闭帧缓冲设备文件描述符
    return 0;
}

/*
将屏幕X和Y方向分辨率（像素），每个像素的位数,映射到用户空间的帧缓冲内存首地址存入显示缓冲区结构体指针中
输入参数：显示缓冲区结构体指针
*/
static int fbgetbuffer(pdispbuff ptdispbuff)
{
    ptdispbuff->ixres = var.xres;
    ptdispbuff->iyres = var.yres;
    ptdispbuff->ibpp  = var.bits_per_pixel;
    ptdispbuff->buff  = fb_base;
    return 0;

}


/*
刷新区域和要刷新的内容（未实现）
输入参数：显示区域结构体指针，缓冲区结构体指针
flushregion 可能啥都不做（因为内存映射后写缓冲区直接显示）
*/
static int fbflushregion(p_region ptregion, pdispbuff ptdispbuff)
{ 
    return 0;
}


/*
用 dispopr 结构体封装的帧缓冲设备的操作接口
*/
static dispopr g_tframebufferopr = {
    .name = "fb",
    .deviceinit = fbdeviceinit,
    .deviceexit = fbdeviceexit,
    .getbuffer = fbgetbuffer,
    .flushregion = fbflushregion,
};

//  @1.2 // 将帧缓冲设备接口注册到管理器   
void framebuffer_register(void) 
{
    registerdisplay(&g_tframebufferopr);  
};