/*
结合项目架构，该文件属于字体库实现层也就是底层驱动/硬件抽象层，该文件是FreeType 字体库的封装实现
核心目标是 “将 FreeType 库的复杂操作抽象为标准化接口”，供上层字体管理系统使用，具体设计思路体现在 3 个方面：
1. 封装 FreeType 细节，简化上层使用
FreeType 库函数复杂（如 FT_Init_FreeType、FT_Load_Char 等），本模块通过 freetype_fontinit、freetype_getfontbitmap 等函数封装，上层无需了解 FreeType 内部机制；
处理单位转换（1/64 像素 → 像素）和坐标系差异（FreeType y 轴向上 → 屏幕 y 轴向下），避免上层重复处理。
2. 实现字体管理抽象层接口，支持多字体库兼容
通过 g_t_freetypeopr 结构体实现 fontopr 接口，与其他字体库（如 ASCII 点阵字体）保持统一，上层可通过 “名称选择” 动态切换字体库；
提供完整的字体操作流程（初始化→设置大小→获取位图→计算字符串外框），满足上层文本显示的所有需求。
3. 支持文本布局，为上层排版提供基础
freetype_getstring_regioncar 计算字符串外框，支持居中、换行等排版功能；
freetype_getfontbitmap 生成字符位图数据。


*/



#include <sys/mman.h>   // 内存映射（未显式用，为字体文件加载兜底）
#include <sys/types.h>  // 系统基础数据类型（如 FreeType 依赖的 int、size_t）
#include <sys/stat.h>   // 文件状态操作（打开字体文件时获取文件信息）
#include <unistd.h>     // 系统调用（如 close 关闭文件）
#include <linux/fb.h>   // 帧缓冲设备结构体（未显式用，为屏幕绘制兜底）
#include <fcntl.h>      // 文件操作（打开字体文件用 O_RDONLY 等标志）
#include <stdio.h>      // 标准输入输出（调试打印错误信息）
#include <string.h>     // 字符串操作（strlen 计算字符串长度）
#include <math.h>       // 数学运算（未显式用，为坐标计算兜底）
#include <wchar.h>      // 宽字符支持（处理 Unicode 编码）
#include <sys/ioctl.h>  // I/O 控制（未显式用，为设备交互兜底）

#include <ft2build.h>   // FreeType 库编译配置（必须放在 FT_FREETYPE_H 前）
#include FT_FREETYPE_H  // FreeType 核心头文件（字体加载、渲染等函数）
#include FT_GLYPH_H     // FreeType 字形操作（获取字符边界框等）

#include <font_manager.h>



//freetype库应用：定义静态全局变量g_tface，后续生成的FT_Face对象表示字体文件的核心结构
//g_tface包含字体的所有信息
static FT_Face g_tface;

static int g_iDefaultFontSize = 12;//初始字体大小

/*
设备初始化，得到face对象g_tface，并设置初始字体大小
输入参数:字体文件路径（如 "/usr/share/fonts/simhei.ttf"）
*/
static int freetype_fontinit(char *afinename)
{
    FT_library   library;//freetype库应用：定义freetype库实例句柄
    int error;
    //1、freetype库应用：初始化 freetype 库
    error = FT_Init_FreeType(&library);
    if(error)
    {
        printf("FT_Init_FreeType err\n");
        return -1;
    }
    
    //2、freetype库应用：加载字体文件，得到一个face
    //参数：library（库实例）、afinename（字体文件路径）、0（字体索引，多字体文件用）、
    //&g_tface（输出face对象）
    error = FT_New_Face(library,afinename,0,&g_tface);
    if(error)
    {
        printf("FT_New_Face err\n");
        return -1;
    }
    //设置字体大小
    // 参数：face对象、宽度（像素）、高度（0表示与宽度等比例）
    FT_Set_Pixel_Sizes(g_tface,g_iDefaultFontSize,0);
    return 0;
}

/*
修改字体大小（长宽相等）
参数：字体大小
*/
static int freetype_setfontsize(int ifontsize)
{
    FT_Set_Pixel_Sizes(g_tface,ifontsize,0);//freetype库应用：设置字体大小
    return 0;
}

/*
字符位图获取函数（字体渲染核心）
根据字符的 Unicode 编码（dwcode），从fontbitmap结构体中定义的基点开始生成该字符的位图数据，并填充剩余其他数据到fontbitmap结构体中
输入参数：字符的 Unicode 编码（要显示的字符串，要逐个输入）,上报数据的结构体
*/
static int freetype_getfontbitmap(unsigned int dwcode,p_fontbitmap pt_fontbitmap)
{
    int error;
    FT_Vector   pen; // freetype库应用：笔位置（字符绘制的基点，单位：1/64像素）
    FT_Glyph    glyph; // freetype库应用：字形对象（未实际使用，可能是预留）
    FT_GlphSlot slot = g_tface->glyph; // freetype库应用：字形槽：存储当前加载的字符信息

    // 1. 设置笔的初始位置（转换为FreeType的单位：1/64像素）
    pen.x = pt_fontbitmap->i_cur_originx *64;// x方向基点单位：1/64像素
    pen.y = pt_fontbitmap->i_cur_originy *64;// y方向基点单位：1/64像素

    // 2. 设置字形变换
    // 参数：face对象、变换矩阵（0表示无变换）、笔位置（决定字符绘制的起点）
    FT_Set_Transform(g_tface,0,&pen);

    // 3. 加载字符并渲染为位图
    //根据 Unicode 编码加载字符，并通过FT_LOAD_RENDER标志直接生成直接生成 8 位灰度位图（存储在slot->bitmap中）。
    // 参数：face对象、Unicode编码、渲染标志（FT_LOAD_RENDER表示直接生成八位灰度位图）
    error = FT_Load_Char(g_tface,dwcode,FT_LOAD_RENDER);
    if(error)
    {
        printf("FT_Load_Char err\n");
        return -1;
    }

    // 4. 填充 fontbitmap 结构体（将 FreeType 数据转换为上层通用格式）
    pt_fontbitmap->puc_buffer           = slot->bitmap.buffer;// 位图像素数据
    pt_fontbitmap-> t_region.i_x        = slot->bitmap_left;// 位图相对于基点的x偏移（左移为负）
    // 位图相对于基点的y偏移（FreeType的y轴向上，需转换为屏幕坐标系）
    pt_fontbitmap-> t_region.i_y        =pt_fontbitmap->i_next_originy - slot->bitmap_top;//bitmap_top:当前字符的位图上边缘相对于 “笔位置” 的 Y 轴偏移
    pt_fontbitmap-> t_region.i_width    =slot->bitmap.width; // 位图宽度
    pt_fontbitmap-> t_region.height     =slot->bitmap.rows;// 位图高度
    // 计算下一个字符的基点位置（前进距离：当前基点+字符宽度（单位转换为像素）
    pt_fontbitmap-> i_next_originx      =pt_fontbitmap->i_cur_originx + slot->advance.x /64;//advance.x:“笔位置” 绘制完当前字符后，需要移动的X 轴距离
    pt_fontbitmap-> i_next_originy      =pt_fontbitmap->i_next_originy;
    return 0;
}
/*
slot->bitmap 的类型是 FT_Bitmap，定义在 freetype.h 中，核心成员包括:
typedef struct  FT_Bitmap_
{
  int             rows;         // 位图高度（像素行数）
  int             width;        // 位图宽度（像素列数）
  int             pitch;        // 行间距（一行像素数据占用的字节数，可能为负）
  unsigned char*  buffer;       // 位图像素数据缓冲区（存储实际的像素值）
  short           num_grays;    // 灰度级数（0 表示单色位图，256 表示 8 位灰度图）
  char            pixel_mode;   // 像素模式（如 FT_PIXEL_MODE_MONO 单色，FT_PIXEL_MODE_GRAY 灰度）
  char            palette_mode; // 调色板模式（通常不用，为兼容保留）
  void*           palette;      // 调色板数据（通常不用）
} FT_Bitmap;
*/



/*
获得字符串的外框
存放在在pt_regioncar中
输入参数：待计算的字符串指针，存储字符串外框的区域指针
*/
static int freetype_getstring_regioncar(char *str , p_region_cartesian pt_regioncar)
{
    int i;
    int error;
    FT_BBOX bbox;// 整个字符串的边界框（合并所有字符的外框）
    FT_BBOX glyph_bbox;// 单个字符的边界框
    FT_Vector pen;// 笔位置（字符绘制的基点，单位：1/64 像素，FreeType 专用单位）
    FT_Glyph glyph;// 字形对象（用于提取单个字符的边界框）
    FT_GlyphSlot slot = g_tface->glyph; // freetype库应用：字形槽：存储当前加载的字符信息

    //初始化
    pen.x = 0;
    pen.y = 0;
    //初始化字符串边界框（先设为极大/极小值，后续逐步更新）
    bbox.xMin =  32767;  // 初始化为 int 类型的“较大值”
    bbox.yMin =  32767;
    bbox.xMax = -32768;  // 初始化为 int 类型的“较小值”
    bbox.yMax = -32768;

    //计算每一个字符的bounding box
    //先translate，再load char，就可以得到外框
    for(i = 0; i < strlen(str); i++)
    {
        //设置变换矩阵和笔位置
        FT_Set_Transform(g_tface, 0, &pen);

        //加载字符并渲染为位图
        error = FT_Load_Char(g_tface, str[i], FT_LOAD_RENDER);
        if (error)
        {
            printf("FT_Load_Char err\n");
            return -1;
        }

        //从字形槽中提取字形对象（用于获取边界框）
        error = FT_Get_Glyph(g_tface->glyph, &glyph);
        if (error)
        {
            printf("FT_Get_Glyph err\n");
            return -1;
        }

        //获取单个字符的边界框（FT_GLYPH_BBOX_UNSCALED：使用未缩放的原始坐标
        FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_UNSCALED, &glyph_bbox);

        //更新外框
        if (glyph_bbox.xMin < bbox.xMin) 
            bbox.xMin = glyph_bbox.xMin;
        if (glyph_bbox.yMin < bbox.yMin) 
            bbox.yMin = glyph_bbox.yMin;
        if (glyph_bbox.xMax > bbox.xMax) 
            bbox.xMax = glyph_bbox.xMax;
        if (glyph_bbox.yMax > bbox.yMax) 
            bbox.yMax = glyph_bbox.yMax;

       //前进到下一个字符位置
       pen.x += slot->advance.x;
       pen.y += slot->advance.y;
    }

    pt_regioncar->x = bbox.xMin;
    pt_regioncar->y = bbox.yMax;
    pt_regioncar->width = bbox.xMax - bbox.xMin + 1;
    pt_regioncar->height = bbox.yMax - bbox.yMin + 1;

    return 0;
}



//注册FreeType结构体1.1
void freetyperegister(void)
{
    registerfont(&g_t_freetypeopr);
}

//配置输入设备结构体
static  fontopr g_t_freetypeopr=
{
    .name ="freetype", 
    .fontinit = freetype_fontinit,//初始化函数
    .setfontsize  = freetype_setfontsize,//设置字体大小
    .getfontbitmap = freetype_getfontbitmap,//获取字符位图
    .getstring_regioncar = freetype_getstring_regioncar,//获取字符串外框
};