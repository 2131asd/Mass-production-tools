#ifndef    __page_manager_h
#define    __page_manager_h

//页面结构体
typedef struct page_action {
    char *name;
    void (*run)(void *p_params); // 页面动作的执行函数
    struct page_action *pt_next;
} page_action, *p_page_action;


void page_register(p_page_action pt_page_action);
void page_system_register(void);
p_page_action page(char *name);


#endif // !__page_manager_h