
extern void thread_pool_init(int num);

extern void thread_pool_add_task(void* (*task)(void *arg), void *arg);
extern void thread_pool_destroy();