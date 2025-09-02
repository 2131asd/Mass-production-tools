/*
当前文件是字体系统的核心管理中间层，所有函数的设计都围绕 “解耦、统一、可扩展” 三个目标，具体价值体现在：
1. 解耦上层与底层实现
上层业务（如 UI 文本显示）只需调用 setfontsize、getfontbitmap 等统一接口，无需关心底层是 FreeType 还是其他字体库；
若需替换字体引擎（如从 FreeType 切换到 ASCII 点阵），只需修改 selectandinitfont 的参数（模块名称），上层代码无需改动。
2. 支持多字体模块动态管理
通过 g_pt_fonts 链表存储所有注册的模块，支持同时注册多个字体引擎（如 FreeType 用于中文字体，ASCII 点阵用于英文字体）；
新增字体模块时，只需实现 fontopr 接口并调用 registerfont，无需修改当前文件，符合 “开闭原则”（对扩展开放，对修改关闭）。
3. 简化上层开发流程
将 “模块注册”“模块选择”“初始化”“操作调用” 拆分为独立函数，上层可按流程逐步调用，无需手动管理链表和引擎状态；
封装重复逻辑（如遍历链表查找模块），避免上层代码冗余。
七、文件所在层级
结合项目架构（参考显示 / 输入系统分层），当前文件属于字体管理核心调度层，位于 “字体模块实现层” 与 “上层业务层” 之间，层级关系如下：
向下：对接具体字体模块（如 FreeType），通过 registerfont 接收模块注册，通过 g_pt_defaultfontopr 调用模块接口；
向上：为上层提供标准化的字体操作入口，屏蔽底层差异，是字体系统的 “中枢神经”
*/


#include <string.h>

#include <font_manager.h>
#include <common.h>

//通过链表结构存储所有已注册的字体操作接口（如 FreeType 的实现）
static p_fontopr g_pt_fonts = NULL;
//记录当前激活的字体引擎，所有字体操作（如设置大小、获取位图）都通过它完成。
static p_fontopr g_pt_defaultfontopr = NULL;

//注册FreeType结构体1
void font_system_register(void)
{
    extern void freetyperegister();
    freetyperegister();  
}

//注册FreeType结构体1.2
void registerfont(p_fontopr  pt_fontopr)
{
    pt_fontopr->pt_next = g_pt_fonts;
    g_pt_fonts = pt_fontopr;
}



/*
字库选择,并初始化字体引擎
输入参数：要选择的字体引擎名称（如 "freetype"），字体文件路径（如 "./simhei.ttf"）
*/
int selectandinitfont(char *a_fontoprname ,char *a_fontfilename)
{
    p_fontopr pt_tmp = g_pt_fonts;
    int error;
    while(pt_tmp)
    {
        if(strcmp(pt_tmp->name , a_fontoprname) == 0)
            break;
        pt_tmp = pt_tmp->pt_next;
    }
    if(!pt_tmp)
        return -1;
    g_pt_defaultfontopr =pt_tmp;

    error = pt_tmp->fontinit(a_fontfilename);   
    return error;
} 

/*
设置字体大小
输入参数：字体大小
*/
int setfontsize(int i_fontsize)
{
    return  g_pt_defaultfontopr->setfontsize(i_fontsize);
}

/*
得到字符位图
输入参数:字符的 Unicode 编码（如 0x4E2D 表示 “中”），位图数据结构体指针
*/
int getfontbitmap(unsigned int dwcode ,p_fontbitmap pt_fontbitmap)
{
    return g_pt_defaultfontopr->getfontbitmap(dwcode,pt_fontbitmap);
}

/*
获得字符串的外框
存放在在pt_regioncar中
输入参数：待计算的字符串指针，存储字符串外框的区域指针（笛卡尔坐标系显示区域）
*/
int getstring_regioncar(char *str , p_region_cartesian pt_regioncar)
{
    return g_pt_defaultfontopr->getstring_regioncar(str, pt_regioncar);
}

