//通用.H用来包含多个多个文件都应用的结构体

#ifndef __COMMON_H
#define __COMMON_H

#ifndef  NULL
#define NULL (void *) 0
#endif

/*
显示区域结构体(LCD坐标系)，用指针传递参数
*/
typedef struct region{
    int x;
    int y;
    int width;
    int height;
} region,*p_region;

/*
显示区域结构体（笛卡尔坐标系），用指针传递参数
 */
typedef struct region_cartesian{
    int x;
    int y;
    int width;
    int height;
} region_cartesian,*p_region_cartesian;

#endif