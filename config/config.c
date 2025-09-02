/*
1. 解耦 “配置解析” 与 “业务逻辑”
业务层（如 UI 按钮创建、功能绑定）无需关心 “配置文件怎么读、怎么解析”，只需调用查询接口获取数据；
若配置文件格式变化（如新增字段），只需修改 parse_configfile，不影响上层业务。
2. 支持灵活的配置查询
按索引遍历：适合批量处理（如自动创建 N 个按钮，按配置顺序排列）；
按名称查询：适合精准定位（如根据用户输入的按钮名执行命令）。
3. 健壮性设计
防溢出：通过 ITEMCFG_MAX_NUM 限制配置项数量，避免数组越界；
容错处理：跳过注释行、格式错误行，保证解析过程不崩溃；
资源释放：fclose 关闭文件，避免内存泄漏。
4. 可扩展性
若新增配置字段（如 color），只需修改 itemcfg 结构体和 sscanf 格式，查询接口无需改动；
支持多类型配置项（如按钮、文本框），只需在配置文件中扩展，解析逻辑可复用。
五、层级定位：配置管理层的核心
在嵌入式系统架构中，该文件属于 配置管理层，处于 “底层文件系统” 与 “上层业务逻辑” 之间：
向下：封装文件 IO 细节，处理配置文件的读取、解析、容错；
向上：通过简单的查询接口，将配置数据转化为业务可直接使用的结构体，降低上层开发复杂度。
*/

#include <stdio.h>
#include <string.h>

#include <config.h>

static itemcfg g_t_itemcfgs[ITEMCFG_MAX_NUM];//存储配置项的数组
static int g_i_itemcfg_count =0;//记录已解析的配置项数量（动态维护，反映实际配置项个数）也就是要显示多少个功能按钮

/*
解析配置文件：从配置文件 CFG_FILE 中读取内容，按规则解析为 itemcfg 结构体
并存储到数组itemcfg g_t_itemcfgs[ITEMCFG_MAX_NUM]中
配置文件每一行有三个参数：名字，是否可触摸（0，1），命令
*/
int parse_configfile(void)
{
    FILE  *fp;// 文件指针，用于操作配置文件
    char buf[100]; // 存储读取的一行内容（最多99字符+结束符）
    char  *p = buf;// 指针，用于处理行首空白字符

    //步骤一：打开文件
    fp = fopen(CFG_FILE,"r");// 以只读方式打开配置文件（路径由CFG_FILE定义）
    if(!fp)
    {
        printf("can not open file %s\n",CFG_FILE);
        return -1;
    }

    //步骤二：读出每一行
    while(fgets(buf,100,fp)) // 循环读取文件，每次读一行（最多99字符）
    {
        //防止溢出，手动设置字符串结束符
        buf[99] = '\0';

        //吃掉开头的的空格或TAB
        p = buf;//指针指向buf起始地址，也就是指针指向当前行的起始位置
        while(*p == ' ' || *p == '\t')
            p++;

        //忽略注释:若行首（清理后）是'#'，则跳过当前行
        if(*p == '#')
            continue;//检测到是注释则跳过这一次循环直接执行下一次循环

        // 新增：检查是否超过最大配置项数量
        if (g_i_itemcfg_count >= ITEMCFG_MAX_NUM)
        {
            printf("配置项数量超过最大值 %d 忽略多余内容\n", ITEMCFG_MAX_NUM);
            break;  // 跳出循环，不再解析
        }

        //处理有效行：解析内容并存储到数组
        //  1. 清空当前配置项的command字段也就是关联的命令（避免残留旧数据）
        g_t_itemcfgs[g_i_itemcfg_count].command[0] = '\0';
        //  2. 为当前配置项设置index（用当前计数作为序号）
        g_t_itemcfgs[g_i_itemcfg_count].index = g_i_itemcfg_count;
        //  3. 按格式解析行内容到结构体字段
        //sscanf 是 C 语言标准库（stdio.h）中的一个字符串格式化输入函数，
        //核心功能是从指定字符串中按自定义格式提取数据
        //%99s限制字符串长度最大为99
        int ret = sscanf(p,"%99s %d %99s",
            g_t_itemcfgs[g_i_itemcfg_count].name,
            &g_t_itemcfgs[g_i_itemcfg_count].b_canbetouched,
            g_t_itemcfgs[g_i_itemcfg_count].command
        );
        
        // 若解析失败（未成功提取3个字段），跳过当前行
        if (ret != 3)
        {
            printf("配置行格式错误：%s， 已忽略\n", p);
            continue;  // 不递增计数，直接处理下一行
        }

        // 4. 配置项计数+1（准备存储下一个配置项）
        g_i_itemcfg_count++;
    }

    // 新增：关闭文件
    fclose(fp);  // 释放文件资源
    return 0;
}

/*
获得配置项数量
*/
int get_itemcfg_count(void)
{
    return g_i_itemcfg_count;
}

/*
传入index得到对应配置项的地址
输入参数：配置项索引（int）
*/
p_itemcfg  get_itemcfg_byindex(int index)
{
    if(index < g_i_itemcfg_count) // 若索引在有效范围内（0 ≤ index < 总数量）
        return &g_t_itemcfgs[index]; // 返回对应配置项的地址（指针）
    else
        return  NULL;
}

/*
传入名称得到对应配置项的地址
输入参数：配置项名称（const char *）
*/
p_itemcfg  get_itemcfg_byname(const char *name)
{
    int i;
    for(i = 0; i < g_i_itemcfg_count; i++)
    {
        if(strcmp(name,g_t_itemcfgs[i].name) == 0)
            return &g_t_itemcfgs[i];
    } 
    return NULL;
}

