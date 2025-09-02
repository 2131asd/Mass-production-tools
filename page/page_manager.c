/*
页面管理核心文件
当前文件是 页面系统的 “中枢神经”，所有函数的设计都围绕 “动态管理多页面、解耦页面实现与业务逻辑” 展开，具体价值体现在 3 个方面：
1. 链表管理：支持多页面动态扩展
通过 g_pt_pages 链表存储所有页面，无需预先定义固定大小的数组，支持灵活添加 / 删除页面（如从 1 个页面扩展到 10 个页面，无需修改管理逻辑）；
头插法注册效率高（O (1)），适合嵌入式系统 “资源有限、追求简单高效” 的需求。
2. 批量注册：简化初始化流程
pages_register() 作为 “一站式注册入口”，上层只需调用这一个函数，即可完成所有预设页面的注册（如主页面、设置页面），无需逐个调用 page_system_register()，减少代码冗余；
新增页面时，只需在该函数中添加一行注册调用，符合 “扩展友好” 的设计原则。
3. 按名查找：解耦业务与页面结构
上层业务无需关心页面在链表中的位置（如主页面是第一个还是第二个节点），只需通过 “页面名称”（如 "mainpage"）即可获取页面结构体，降低业务层与页面管理层的耦合；
示例：若后续调整页面注册顺序（主页面从第一个变为第二个），业务层的 page("mainpage") 调用无需修改，仍能正确找到页面。
五、文件所在层级
结合嵌入式 UI 系统的分层架构，当前文件属于 页面管理层，位于 “具体页面实现层” 与 “业务层” 之间，是连接 “页面组件” 与 “业务逻辑” 的核心纽带，层级关系如下
向下：对接具体页面的实现（如 mainpage.c），通过 page_system_register 接收页面注册，统一管理页面生命周期；
向上：为业务层提供 “注册”“查找” 页面的标准化接口，让业务层无需关心页面的底层存储结构（链表），只需聚焦 “如何使用页面”（如初始化、绘制），大幅提升开发效率。
*/

#include <string.h>

#include <common.h>
#include <page_manager.h>

// 页面链表头指针：存储所有已注册的页面，初始为 NULL（链表为空）
static p_page_action g_pt_pages = NULL;

/*
注册多个页面
*/
void page_system_register(void)
{
    mainpage_register();
}

/*
将页面结构体注册到链表中
*/
void page_register(p_page_action pt_page_action)
{
    pt_page_action->pt_next = g_pt_pages;
    g_pt_pages = pt_page_action;
}



/*
按名称获得某一个页面
输入参数：名字（char *）
返回值： 页面的结构体指针 p_page_action g_pt_pages
*/
p_page_action page(char *name)
{
    p_page_action pt_tmp = g_pt_pages;

    while (pt_tmp != NULL) {
        if (strcmp(name , pt_tmp->name) == 0) 
        {
            return pt_tmp;  
        }
         pt_tmp = pt_tmp->pt_next;
    }
    return NULL;
}
