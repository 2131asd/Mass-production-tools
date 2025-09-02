/*
该文件的核心目标是 将 “按钮” 封装为可复用的 UI 组件，解决上层业务（如页面、弹窗）中 “重复编写按钮绘制 / 交互逻辑” 的问题，具体设计价值体现在 4 个方面：
1. 组件化封装：降低 UI 开发成本
将按钮的 “绘制”“状态管理”“交互反馈” 封装为独立函数，上层无需关心 “如何画按钮”“如何处理按下效果”，只需调用 init_button 即可创建按钮；
示例：若需创建 10 个按钮，只需调用 10 次 init_button，无需重复编写 10 次 “画红色背景 + 居中文字” 的代码。
2. 默认行为兜底：简化基础使用
提供 default_on_draw 和 default_on_pressed 作为默认实现，上层在无需自定义按钮样式 / 行为时，可直接使用默认逻辑（如红色底色、按下变绿）；
新手开发者无需理解底层显示逻辑，即可快速创建可用的按钮。
3. 支持自定义扩展：满足多样化需求
允许上层传入自定义的 on_draw 和 on_pressed 函数，实现个性化按钮（如圆形按钮、按下后播放动画、长按触发特殊逻辑）；
示例：自定义 my_on_draw 函数绘制蓝色渐变底色，调用 init_button(btn, "取消", &region, my_on_draw, NULL) 即可创建蓝色按钮，不影响其他按钮。
4. 解耦 UI 与底层：确保跨硬件兼容性
按钮的绘制和刷新完全依赖显示层的标准接口（draw_region、flushdisplayregion），不直接操作硬件（如帧缓冲地址）；
若后续更换显示设备（如从 LCD 切换到 OLED），只需确保显示层接口兼容，按钮组件无需修改，实现 “UI 与硬件解耦”。
五、文件所在层级
结合项目架构（参考显示 / 输入 / 字体层），该文件属于 UI 组件层，位于 “显示 / 字体层” 之上、“业务层” 之下，层级关系如下：
向下：调用基础能力层的接口（显示绘制、字体设置、输入处理），实现 UI 组件的视觉和交互效果；
向上：为业务层提供 “即插即用” 的按钮组件，上层业务只需关注 “按钮的位置、文本、点击后的业务逻辑”，无需关心 UI 实现细节，大幅提升开发效率。
*/

#include <ui.h>
#include <disp_manager.h>
#include <font_manager.h>



/*
默认的按钮绘制函数
红色底色文字居中
输入参数：按钮的结构体指针，缓冲区指针
*/

static  int default_on_draw(struct button *pt_button,pdispbuff pt_dispbuff)//绘制按钮的函数
{
    //绘制底色（红色）
    draw_region(&pt_button->t_region, BUTTON_DEFAULT_COLOR);
    //居中显示文字
    setfontsize(pt_button->font_size);
    drawtext_inregioncentral(pt_button->name,&pt_button->t_region,BUTTON_TEXT_COLOR);
    //刷新到硬件上
    flushdisplayregion(&pt_button->t_region, pt_dispbuff);
    return 0; //假设默认实现返回 0 表示成功

}

/*
默认的按钮按下函数
输入参数：按钮的结构体指针，缓冲区指针，上报数据指针
*/
static int default_on_pressed(struct button *pt_button,pdispbuff pt_dispbuff,p_inputevent pt_inputevent) 
{
    unsigned int dwcolor = BUTTON_DEFAULT_COLOR; // 按钮初始的颜色 

    pt_button->status = !pt_button->status; // 切换按钮状态
    if(pt_button->status)
        dwcolor = BUTTON_PRESSED_COLOR; // 按钮被按下时颜色变为绿色

    // 绘制底色
    draw_region(&pt_button->t_region, dwcolor);
    //居中显示文字
    setfontsize(pt_button->font_size);
    drawtext_inregioncentral(pt_button->name,&pt_button->t_region,BUTTON_TEXT_COLOR);
    //刷新到硬件上
    flushdisplayregion(&pt_button->t_region, pt_dispbuff);
    return 0; //假设默认实现返回 0 表示成功
}

/*
按钮初始化函数
初始化配置按钮的结构体接口参数，将名字，绘制区域，默认的按键形态，默认的按下后执行的操作四个参数赋值给结构体
输入参数：按钮的结构体指针，名字，绘制区域结构体指针，绘制按钮函数，按钮被按下的函数
*/
void init_button(p_button pt_button , char *name , p_region pt_region , on_draw_func on_draw , on_pressed_func on_pressed) 
{
    pt_button->name             = name;
    pt_button->status           = 0;
    if(pt_region)
        pt_button->t_region     = *pt_region;
    pt_button->on_draw          = on_draw ? on_draw : default_on_draw; // 使用默认绘制函数
    pt_button->on_pressed       = on_pressed ? on_pressed : default_on_pressed; // 使用默认按下函数
}