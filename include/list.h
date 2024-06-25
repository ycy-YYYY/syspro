#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// 定义链表节点结构
typedef struct ListNode {
  struct ListNode *prev;
  struct ListNode *next;
} ListNode;

// 初始化链表头
#define INIT_LIST_HEAD(ptr)                                                    \
  do {                                                                         \
    (ptr)->next = (ptr);                                                       \
    (ptr)->prev = (ptr);                                                       \
  } while (0)

// 初始化链表节点
#define INIT_LIST_NODE(node)                                                   \
  do {                                                                         \
    (node)->prev = (node);                                                     \
    (node)->next = (node);                                                     \
  } while (0)

// 添加新节点到链表头
void list_add(ListNode *new, ListNode *head);

// 从链表中删除节点
void list_del(ListNode *entry);

// 判断链表是否为空
int list_empty(ListNode *head);

// 获取用户数据结构的指针
#define list_entry(ptr, type, member)                                          \
  ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

// 遍历链表
#define list_for_each(pos, head)                                               \
  for (pos = (head)->next; pos != (head); pos = pos->next)

#ifdef __cplusplus
}
#endif
