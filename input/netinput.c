/*
该文件属于输入驱动抽象层,该文件的核心目标是将 UDP 网络数据封装为系统统一的输入事件，解耦上层业务与底层网络细节，具体价值体现在 3 点：

屏蔽硬件 / 协议差异：
上层无需关心 “UDP 协议如何工作”“socket 如何创建”，只需调用 get_inputevent 即可获取网络数据，降低开发复杂度；
统一输入框架：
通过 inputdevice 结构体与其他输入设备（触摸屏、按键）对齐，上层业务（如 UI 点击、命令执行）可 “无差别处理所有输入”，避免针对不同输入源写重复代码；
可扩展与可管理：
新增输入源（如串口输入）时，只需按 inputdevice 接口实现 “初始化 - 读取 - 清理” 函数并注册，无需修改输入管理器核心逻辑；同时通过 “注册 - 选择” 机制，可动态启用 / 禁用网络输入。
五、文件所在层级
结合项目架构（参考输入 / 显示系统分层），该文件属于输入驱动抽象层，位于：

下层：对网络硬件（网卡）和系统内核（socket 系统调用），实现 UDP 数据的接收与基础处理；
上层：为输入管理器（input_manager.c）提供标准 inputdevice 接口，支撑上层业务（如根据网络输入更新 UI、执行控制命令）。

*/

#include <sys/types.h>// 系统基础数据类型（如 socket 相关的 int、size_t）
#include <sys/socket.h>// socket 编程核心头文件（socket、bind、recvfrom 等函数）
#include <netinet/in.h>// IPv4 地址结构体（sockaddr_in）和字节序转换（htons 等）
#include <arpa/inet.h>// IP 地址转换函数（虽未显式用，但为 socket 编程标准依赖）

#include <unistd.h> // 系统调用（close 等）

#include  <stdio.h> // 标准输入输出（printf 调试信息）
#include <string.h>// 字符串操作（memset、strncpy 等）
#include <input_manager.h>

/*
socket
bind
sendto/recvfrom
*/ 
#define SERVER_POPRT 8888  // UDP 服务器端口号（8888，注意宏名笔误，应为 SERVER_PORT）
static int g_i_socketserver; // UDP 服务器套接字文件描述符（唯一标识服务器 socket）

/*
网络输入数据初始化(UDP) （输入设备初始化流程2.1）
*/
static int net_deviceinit(void)
{
    struct sockaddr_in t_socketserver_addr; // IPv4 专用地址结构体，存储服务器 （IP+端口）
    int i_ret;

    // 1. 创建 UDP 套接字（SOCK_DGRAM 表示 UDP 协议，0 表示默认协议族）
    g_i_socketserver = socket(AF_INET,SOCK_DGRAM,0);
    if(-1 == g_i_socketserver)    
    {   
        printf("socket error!\n");
        return -1;
    }

    // 2. 配置服务器地址结构体（sockaddr_in）
    t_socketserver_addr.sin_family       =AF_INET; // 地址族：IPv4  
    t_socketserver_addr.sin_port         =htons(SERVER_POPRT);// 端口号：转换为网络字节序（大端）,htons 是“主机字节序转网络字节序”，避免不同 CPU 字节序差异
    t_socketserver_addr.sin_addr.s_addr  =INADDR_ANY;// 绑定到所有网卡（0.0.0.0），可接收任意网卡的数据
    //确保 sockaddr_in 结构体的填充字段无垃圾数据，保证系统调用的兼容性和行为可预测性
    memset(t_socketserver_addr.sin_zero,0,8);

    // 3. 将套接字与地址绑定（让 socket 监听指定端口和网卡）
    //将创建的套接字 g_i_socketserver 与配置好的地址（t_socketserver_addr）绑定，使套接字能接收发往该地址的数据。
    i_ret = bind(g_i_socketserver,(const struct sockaddr *)&t_socketserver_addr,sizeof(struct sockaddr));
    if(-1 == i_ret)
    {
        printf("bind err!\n");
    }
    return 0;
}





//网络输入事件读取函数  (输入设备初始化 3.1)
static int net_getinputevent(p_inputevent pt_inputevent)
{
    struct sockaddr_in t_socketclient_addr;// IPv4 专用地址结构体，存储客户端地址（IP+端口）
    int i_recvlen;//接收的数据长度
    char arecvbuf[1000];//接收数据的缓冲区（最大 999 字节有效数据 + 1 字节终止符）
    unsigned int i_addrlen = sizeof(struct sockaddr);//客户端地址结构体长度
    //阻塞等待 UDP 数据报，接收后填充 arecvbuf，并记录客户端地址
    i_recvlen = recvform(g_i_socketserver,arecvbuf,999,0,(struct sockaddr *)&t_socketclient_addr,&i_addrlen);
    if(i_recvlen >0)
    {
        arecvbuf[i_recvlen] = '\0';// 给接收数据加字符串终止符，方便后续处理
        pt_inputevent->i_type   = INPUT_TYPE_NET;//设置事件类型为网络输入
        gettimeofday(&pt_inputevent->tTime,NULL); // 记录事件发生的时间（实际是“接收数据的时间”）
        strncpy(pt_inputevent->str,arecvbuf,1000); // 保存原始接收数据到 inputevent 的 str 成员（最多 1000 字节）
        pt_inputevent->str[999] = '\0'; // 强制终止，防止数组溢出 
        return 0;
    }
    else
        return -1;
}

/*
网络输入设备清理
*/
static int net_deviceexit(void)
{
    close(g_i_socketserver);
    return 0;
}

//网络输入设备接口封装（inputdevice 结构体）
static inputdevice g_t_netinput_dev ={
    .name = "net",                         // 设备名称："net"
    .deviceinit         = net_deviceinit,      //绑定“设备初始化”函数     
    .get_inputevent     = net_getinputevent,   //绑定“读取事件”函数
    .deviceexit         = net_deviceexit,      // 绑定“设备退出”函数 
};

/*
注册网络输入结构体  （输入初始化流程1.1）
*/
void netinput_register(void)
{
    register_inputdecive(&g_t_netinput_dev);
}



