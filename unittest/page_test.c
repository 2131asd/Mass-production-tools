#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <disp_manager.h>
#include <stdlib.h>

#include <page_manager.h>


int main(int argc,char **argv)
{
    page_register();
    page("main")->run(NULL); // 调用 main 页面动作的执行函数
    return 0;
}
