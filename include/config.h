#ifndef CONFIG_H
#define CONFIG_H

#include <common.h>

#define ITEMCFG_MAX_NUM 30  //配置项最大数量
#define CFG_FILE "/ect/test_gui/gui.conf" //配置文件路径


typedef struct itemcfg
{
    int index;//配置项索引
    char name[100];//配置项名称
    int b_canbetouched;//是否可以被触摸
    char command[100];//关联的命令
}itemcfg,*p_itemcfg;

int get_itemcfg_count(void);
p_itemcfg  get_itemcfg_byindex(int index);
p_itemcfg  get_itemcfg_byname(int name);
int parse_configfile(void);

#endif