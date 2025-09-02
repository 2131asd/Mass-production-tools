//这段代码是一个 GUI 应用中的主页面管理模块，
//负责主页面的按钮生成、输入事件处理（触摸 / 网络事件）、页面绘制与运行等核心功能

#include <stdio.h>
#include <math.h>
#include <config.h>
#include <string.h>
#include <stdlib.h>

#include <page_manager.h>
//#include <disp_manager.h>
//#include <font_manager.h>
//#include <input_manager.h>
#include <ui.h>


#define X_GAP 5 //按钮之间间隔
#define Y_GAP 5 //按钮之间间隔

static button g_t_buttons[ITEMCFG_MAX_NUM];// 存储主页面所有按钮的数组
static int g_t_buttoncnt;//记录实际按钮数量


/*
生成按钮  
该函数根据配置文件中的配置项（itemcfg）逐个计算按钮显示区域，把按钮名字，显示区域
以及后面编写的按下后的执行函数一同赋值给按钮结构体，进行初始化所有按钮
*/
static int generate_buttons(void)
{
    int width ,height;
    int n_per_line;//每行显示按钮个数
    int row,rows;//行数
    int col;//列数
    int n;//配置项数量
    pdispbuff p_dispbuff;//缓冲区结构体
    int xres,yres;//行和列的分辨率
    int start_x,start_y;//四周空余大小,也是初始xy值
    int pre_start_x,pre_start_y;//下一个按钮 的xy值
    p_button p_button;
    int i = 0;
    int i_fontsize;//合适的字体大小

    //算出单个按钮的width和height
    g_t_buttoncnt  = n = get_itemcfg_count(); // 获取配置项数量（即按钮总数）
    p_dispbuff = getdisplaybuffer(); // 获取显示缓冲区（来自disp_manager）
    xres = p_dispbuff->ixres;// 屏幕宽度（x方向分辨率）
    yres = p_dispbuff->iyres;// 屏幕高度（y方向分辨率）
    width = sqrt(1.0/0.618*xres*yres/n);// 初始宽度：根据黄金比例（1/0.618）和屏幕面积计算 
    n_per_line = xres / width + 1;// 每行按钮数量（初始估算）
    width = xres / n_per_line; // 修正宽度：确保每行按钮能填满屏幕宽度
    height = width * 0.618; // 高度：按黄金比例（0.618）计算（宽*0.618）

    //居中显示：计算
    start_x = (xres - width * n_per_line) / 2;// x方向起点：使所有按钮在屏幕水平居中
    rows = n / n_per_line; // 总行数（初始估算）
    if(rows * n_per_line < n)// 修正行数：若有余数则加1行
        rows++;
    start_y = (yres - height * rows) / 2; // y方向起点：使所有按钮在屏幕垂直居中

    //计算每个按钮的显示区域region
    for(row = 0; (row < rows) && (i < n); row++)
    {
        pre_start_x = start_x - width;// 行内第一个按钮的x起点前值（用于计算下一个按钮）
        pre_start_y = start_y + row * height; // 当前行的y起点（每行y坐标递增height）
        for(col = 0; (col < n_per_line) && (i < n); col++)
        {
            p_button = &g_t_buttons[i];//把每个按钮的首地址赋值，后续在此地址存储按钮参数
            p_button->t_region.x = pre_start_x + width;
            p_button->t_region.y = pre_start_y;
            p_button->t_region.width = width -  X_GAP;// 宽度：减去水平间隔
            p_button->t_region.height = height - Y_GAP;
            pre_start_x = p_button->t_region.x;// 更新下一个按钮的x起点前值
            
            //初始化按钮
            init_button(p_button , get_itemcfg_byindex(i)->name,NULL,NULL,mainpage_on_pressed);
            i++;
        }
    }
    //为了防止生成按钮后有其他的函数修改字体大小，所以把字体大小也放入按钮结构体
    i_fontsize = getfontsize_forallbutton();

    //绘制按钮
    for(int i = 0; i < n; i++)
    {
        g_t_buttons[i].font_size = i_fontsize;
        g_t_buttons[i].on_draw(&g_t_buttons[i], p_dispbuff);
    }
    return 0;
}


/*
按钮按下执行的函数
切换按钮状态，更新颜色
输入参数：按钮的结构体指针，缓冲区指针，上报数据指针
*/
int mainpage_on_pressed(struct button *pt_button , pdispbuff pt_dispbuff , p_inputevent pt_inputevent) 
{
    unsigned int dwcolor = BUTTON_DEFAULT_COLOR; // 按钮初始的颜色 
    char name[100];
    char status[100];
    char *strbutton; // 按钮上显示的文字（可能是名称或状态值）
    char *command_status[3] = {"err", "ok", "percent"};
    int command_status_index = 0;
    char command[1000];
    p_itemcfg pt_itemcfg;

    strbutton = pt_button->name; // 默认显示按钮名称
    //对于触摸屏事件
    if(pt_inputevent->i_type == INPUT_TYPE_TOUCH)
    {
        //分辨能否被点击,（b_canbetouched为 0 则禁止）
        if(get_itemcfg_byname(pt_button->name)->b_canbetouched == 0)
        {
            return -1; //不能被点击
        }

        //按钮按下执行的功能：切换按钮状态，更新颜色
        pt_button->status = !pt_button->status;
        if(pt_button->status)
        {
            dwcolor = BUTTON_PRESSED_COLOR;
            command_status_index = 1;
        }
    }
    //对于网络类事件
    else if(pt_inputevent->i_type == INPUT_TYPE_NET)
    {
        //根据传进来的字符串修改颜色
        //从网络事件字符串中解析出“名称”和“状态”（格式如"button1 ok"）
        sscanf(pt_inputevent->str,"%s %s",name,status);
        if(strcmp(status,"ok") == 0)
        {
            dwcolor = BUTTON_PRESSED_COLOR;
            command_status_index = 1;
        }
        else if(strcmp(status,"cancel") == 0)
        {
            dwcolor = BUTTON_DEFAULT_COLOR;
            command_status_index = 0;
        }
        else if (status[0] >= '0' && status[0] <= '9')
        {
            dwcolor = BUTTON_PERCENT_COLOR;
            strbutton = status;
            command_status_index = 2;
        }
        else
        {
            return -1; //未知状态
        }
    }
    else 
    {
        return -1;
    }

    // 绘制底色
    draw_region(&pt_button->t_region, dwcolor);
    //居中显示文字
    drawtext_inregioncentral(strbutton,&pt_button->t_region,BUTTON_TEXT_COLOR);
    //刷新到硬件上
    flushdisplayregion(&pt_button->t_region, pt_dispbuff);

    //执行command
    pt_itemcfg = get_itemcfg_byname(pt_button->name);
    if(pt_itemcfg->command[0] != '\0')
    {
        printf(command , "%s %s",pt_itemcfg->command,command_status[command_status_index]);
        //执行命令
        system(command);// 调用系统命令（如启动程序、发送指令）
    }

    return 0;
}

/*
根据最长的名字算出适合的字体大小
输出参数：字体大小 
*/
static int getfontsize_forallbutton(void)
{
    int i;
    int max_len = -1;
    int max_index;
    int len;
    region_cartesian t_regioncar;
    float k,kx,ky;

    //第一步：找出name最长的button
    for(i = 0; i < g_t_buttoncnt; i++)
    {
        len = strlen(g_t_buttons[i].name);
        if(len > max_len)
        {
            max_len = len;
            max_index = i;
        }
    }

    //第二步：以foont_size= 100,算出他的外框
    setfontsize(100);
    getstring_regioncar(g_t_buttons[max_index].name,&t_regioncar);

    //第三步：把文字的外框缩放为button的外框
    kx = g_t_buttons[max_index].t_region.width / t_regioncar.width; // x方向缩放比例
    ky = g_t_buttons[max_index].t_region.height / t_regioncar.height; // y方向缩放比例
    if(kx < ky)
    {
        k = kx; // 取较小的缩放比例，保证文字不会超出按钮区域
    }
    else
    {
        k = ky;
    }

    //第四步：根据缩放比例计算出合适的字体大小，只取0.8为了美观
    return (int)(100 * k * 0.8);
}



/*
根据名字找到按钮
输入参数：名字（char*）
输出参数：对应的按钮结构体地址
*/
static p_button get_button_by_name(char *name)
{
    int i;
    for(i = 0; i < g_t_buttoncnt; i++)
    {
        if(strcmp(name, g_t_buttons[i].name) == 0)
        {
            return &g_t_buttons[i];
        }
    }
    return NULL; //没有找到
}

/*
判断触电位于区域内
输入参数：触点xy坐标，区域指针
*/
static int isTouchPointInRegion(int x, int y, p_region pt_region)
{
    if(x < pt_region->x || x > pt_region->x + pt_region->width ||
       y < pt_region->y || y > pt_region->y + pt_region->height)
    {
        return 0; //不在区域内
    }
    return 1;//在区域内
}

/*
根据输入事件找到按钮
输入参数：输入事件指针
输出参数：对应的按钮结构体地址
*/
static p_button get_button_by_inputevent(p_inputevent pt_inputevent)
{   
    int i;
    char name[100];
    if(pt_inputevent->i_type == INPUT_TYPE_TOUCH)
    {
        for(i = 0; i < g_t_buttoncnt; i++)
        {
            if(isTouchPointInRegion(pt_inputevent->i_x,pt_inputevent->i_y,&g_t_buttons[i].t_region))
            {
                return &g_t_buttons[i];
            }
        }

    }
    else if(pt_inputevent->i_type == INPUT_TYPE_NET)
    {
       sscanf(pt_inputevent->str,"%s",name);
       return get_button_by_name(name);
    }
    else
    {
        return NULL;    
    }
    return NULL; 
}
    

/*
页面执行函数，主页面的入口函数，负责初始化、事件循环和按钮交互
*/
static void mainpage_run(void *p_params)
{
    int error;
    inputevent t_inputevent;
    p_button pt_button;
    pdispbuff pt_disbuff = getdispbuff();

    //初始化步骤：
    //1、调用parse_configfile解析配置文件（获取按钮名称、是否可触摸等信息）。
    //2、调用generate_buttons生成按钮并绘制初始界面。
    error = parse_configfile();
    if (error)
        return ;
    generate_buttons();

    while(1)
    {
        //读取输入事件
        error = get_inputevent(&t_inputevent);
        if(error)
        {
            continue;
        }
        //根据输入事件找到按钮
        pt_button = get_button_by_inputevent(&t_inputevent);
        if(!pt_button)
        {
            continue;
        }
        //调用按钮的on_pressed函数
        pt_button->on_pressed(pt_button, pt_disbuff, &t_inputevent);
    }
}

/*
注册页面结构体
*/
void mainpage_register(void)
{
    page_register(&g_t_mainpage);
}

/*
页面结构体接口
内部参数：名字，执行函数
*/
static page_action g_t_mainpage = {
    .name = "main",
    .run = mainpage_run,
};

