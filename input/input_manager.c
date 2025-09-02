/*
该文件是输入系统的核心管理中间层，目标是实现 “多输入源并行处理”“线程安全的事件传递”“上层业务无感知对接”，具体设计思路体现在 4 个方面：
1. 多设备管理：链表 + 统一接口
用 g_inputdevs 链表管理所有输入设备，通过 inputdevice 结构体标准化接口（deviceinit/get_inputevent），新增设备（如按键输入）只需实现接口并注册，无需修改核心逻辑；
遍历链表初始化设备并创建线程，实现多设备并行处理，避免单线程阻塞。
2. 线程同步：互斥锁 + 条件变量
互斥锁（g_tMutex）保护 “缓冲区读写”“设备链表操作” 等临界区，防止多线程（如 2 个生产者线程 + 1 个消费者线程）数据竞争；
条件变量（g_tConVar）实现 “生产者 - 消费者” 同步：生产者写数据后唤醒消费者，消费者无数据时等待，避免 CPU 空转。
3. 事件暂存：环形缓冲区
用环形缓冲区暂存事件，解耦 “事件读取”（生产者线程）与 “事件处理”（上层业务）：
避免生产者线程因 “消费者处理慢” 导致事件丢失；
避免消费者线程因 “生产者无数据” 导致阻塞；
环形结构实现缓冲区复用，比线性缓冲区更高效。
4. 上层接口统一：get_inputevent
上层业务只需调用 get_inputevent 即可读取所有输入设备的事件，无需区分触摸、网络等输入源，实现 “输入源无关性”；
封装同步与缓冲区细节，降低上层开发复杂度。
*/

#include <pthread.h>    // 线程库头文件（线程创建、互斥锁、条件变量）
#include <stdio.h>      // 标准输入输出（调试打印）
#include <unistd.h>     // 系统调用（无显式用，为线程相关依赖兜底）
#include <semaphore.h>  // 信号量头文件（虽未显式用，为同步机制标准依赖）
#include <string.h>     // 字符串操作（无显式用，为输入事件结构体拷贝兜底）

#include <input_manager.h>   // 输入系统头文件（定义 inputdevice/inputevent 结构体、函数声明）


//初始化一个互斥锁,保护“环形缓冲区读写”“设备链表操作”等临界区，防止多线程数据竞争
static pthread_mutex_t g_tMutex  = PTHREAD_MUTEX_INITIALIZER;
//初始化条件变量（在互斥锁中临时释放锁）实现“读事件线程”与“取事件线程”的同步（无数据时等待，有数据时唤醒）
static pthread_cond_t  g_tConVar = PTHREAD_COND_INITIALIZER;
//初始化输入设备链表的头指针为空，准备后续挂载设备
static p_inputdevice g_inputdevs = NULL;

//start of 实现环形buffer
#define BUFFER_LEN 20 //环形缓冲区最大容量（最多存 20 个输入事件）
static int gi_read = 0;// 缓冲区读指针（指向待读取的事件位置）
static int gi_write = 0;// 缓冲区写指针（指向待写入的事件位置）
static inputevent g_atinputevent[BUFFER_LEN];// 环形缓冲区数组（存储 inputevent 结构体）

/*
输入系统注册（输入初始化流程1）
也就是把两个输入设备结构体都注册到链表中  
*/
void input_system_register(void)
{
    extern void touchscreen_register(void); // 声明外部函数
    touchscreen_register(); //调用，触发触摸屏注册

    extern void netinput_register(void);// 声明外部函数
    netinput_register(); //调用，触发网络输入注册
}

/*
链表头传递输入设备结构体  （输入初始化流程1.2） 
*/
//两设备都初始化执行完毕后。g_inputdevs为网络输入结构体。触摸屏结构体的pt_next为NULL。网络输入结构体的pt_next为触摸屏结构体。
void register_inputdevice(p_inputdevice pt_inputdev)
{
    pt_inputdev->pt_next = g_inputdevs; // 新设备的 next 指向当前链表头
    g_inputdevs = pt_inputdev;          // 链表头更新为新设备（头插法）
}




/*
输入设备初始化（输入设备初始化流程2）
遍历输入设备链表。从链表中取出设备，设备初始化然后创建一个线程。 
pt_tmp->deviceinit时调用的是inputdevice类型结构体的设备接口输入
也就是触摸屏结构体的.deviceinit     = touchscreen_deviceinit,
和网络输入结构体的.deviceinit     = net_deviceinit,
*/
void input_deviceinit(void)
{
    int ret;
    pthread_t tid;// 线程 ID（存储创建的线程标识）
    //对于每一个输入设备都先初始化设备，然后创建线程
    p_inputdevice pt_tmp = g_inputdevs;
    while(pt_tmp)
    {
        //// 1. 初始化当前设备（调用设备自带的初始化函数）
        ret = pt_tmp->deviceinit();
        //创建线程
        if(!ret)
        {
            ret = pthread_create(&tid,NULL,input_recv_thread_func,pt_tmp);
        }
        pt_tmp = pt_tmp->pt_next;
    }
}


/*
读取设备事件并写入缓冲区   （输入设备初始化流程3）
不断调用getinputevent获取事件数据，读取数据函数后将事件数据写入缓冲区保存数据，唤醒等待数据的线程 
输入参数：任意数据

pt_tmp作为输入参数其实也就是把设备的链表地址也就是把挂载的设备的inputdevice类型结构体的设备接口输入
所以调用t_inputdev->get_inputevent时调用的是输入参数设备相应的接口get_inputevent所实现的函数
也就是触摸屏结构体的.get_inputevent  = touchscreen_getinputevent
和网络输入结构体的.get_inputevent  = net_getinputevent
*/
static void *input_recv_thread_func(void *data)
{
    p_inputdevice t_inputdev = (p_inputdevice)data; // 转换参数：获取当前设备结构体
    inputevent t_event;// 临时存储读取到的输入事件结构体参数
    int ret;
    while(1)
    {
        //读取数据（阻塞式，无事件时等待）
        ret = t_inputdev->get_inputevent(&t_event);
        if(!ret)
        {
            //保存数据，
            pthread_mutex_lock(&g_tMutex);// 加锁：保护环形缓冲区（临界区操作，防止多线程同时写）
            put_inputevent_tobuffer(&t_event);// 将事件写入缓冲区
            pthread_cond_signal(&g_tConVar);// // 发送信号：唤醒等待事件的上层线程
            pthread_mutex_unlock(&g_tMutex);// 解锁：释放临界区
        }
    }
    return NULL;
}

/*
向环形缓冲区写如输入参数中的数据 （输入设备初始化流程3.2）
输入参数：上报数据的结构体指针
*/
static void put_inputevent_tobuffer(p_inputevent pt_inputevent)
{
    if(!is_inputbuffer_full()) // 缓冲区未满时才写入（避免溢出）
    {
        g_atinputevent[gi_write] = *pt_inputevent;// 拷贝事件到缓冲区当前写位置
        gi_write =(gi_write + 1)% BUFFER_LEN; // 写指针后移，环形循环（超界时重置为 0）
    }
}

/*
上层业务读事件接口  （输入设备初始化流程4）
调用函数从环形缓冲区中读取数据，放入输入参数中
输入参数：上报的数据结构体指针
*/
int get_inputevent(p_inputevent pt_inputevent)
{
    inputevent t_event;
    int ret;
    //无数据则休眠，有数据再写入
    pthread_mutex_lock(&g_tMutex);
    if(get_inputevent_frombuffer(&t_event))
    {
        *pt_inputevent = t_event;
        pthread_mutex_unlock(&g_tMutex);
        return 0;
    }
    else
    {
        //休眠等待，调用条件变量等待（自动释放锁，避免死锁）
        pthread_cond_wait(&g_tConVar,&g_tMutex);
        //唤醒后，再次尝试读取（防止“虚假唤醒”）
        if(get_inputevent_frombuffer(&t_event))
        {
            *pt_inputevent = t_event;
            ret = 0;
        }
        else
        {
            ret = -1;
        }
        pthread_mutex_unlock(&g_tMutex);
    }
    return ret;
}


/*
取出环形缓冲区的数据存放在输入参数中  （输入设备初始化流程4.1）
输入参数：上报的数据结构体指针
*/
static int get_inputevent_frombuffer(p_inputevent pt_inputevent)
{
    if(!is_inputbuffer_empty())
    {
        *pt_inputevent = g_atinputevent[gi_read];
        gi_read =(gi_read + 1)% BUFFER_LEN;
        return 1;
    }
    else
    {
        return 0;
    }  
}


//环形缓冲区为空的判断函数，返回0因为只有两个都为0时才会相等是
static int is_inputbuffer_empty(void)
{
    return(gi_read == gi_write);
}
//环形缓冲区为满的判断函数，返回满时的长度（不一定为BUFFER_LEN）
static int is_inputbuffer_full(void)
{
    return(gi_read == (gi_write + 1) % BUFFER_LEN);
}















