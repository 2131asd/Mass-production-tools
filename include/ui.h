#ifndef __ui_h
#define __ui_h
#include <common.h>
#include <disp_manager.h>
#include <input_manager.h>

#define BUTTON_DEFAULT_COLOR 0XFF0000  //默认按钮颜色红色
#define BUTTON_PRESSED_COLOR 0x00FF00 //默认按钮按下后颜色绿色
#define BUTTON_PERCENT_COLOR 0x0000FF //默认按钮按下后颜色绿色
#define BUTTON_TEXT_COLOR 0x000000 //默认按钮文字颜色黑色

struct button; //前置声明

typedef int (*on_draw_func)(struct button *pt_button,pdispbuff pt_dispbuff); //绘制按钮的函数
typedef int (*on_pressed_func)(struct button *pt_button,pdispbuff pt_dispbuff,p_inputevent pt_inputevent); //按钮被按下的函数

//按钮的结构体接口
typedef struct button
{
    char *name;
    int font_size;//字体大小
    region t_region; //按钮所在区域
    on_draw_func on_draw; //绘制按钮的函数
    on_pressed_func on_pressed; //按钮被按下的函数
    int status;

}button,*p_button;

/*上面的功能等同于下面
//按钮的结构体接口
typedef struct button
{
    char *name;
    region t_region; //按钮所在区域
    int (*on_draw_func)(struct button *pt_button,pdispbuff pt_dispbuff); //绘制按钮的函数
    int (*on_pressed_func)(struct button *pt_button,pdispbuff pt_dispbuff,p_inputevent pt_inputevent); //按钮被按下的函数

}button,*p_button;
*/

void init_button(p_button pt_button, char *name, p_region pt_region, on_draw_func on_draw, on_pressed_func on_pressed);

#endif
