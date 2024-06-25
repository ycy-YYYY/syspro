#include "list.h"

// 添加新节点到链表头
void list_add(ListNode *new, ListNode *head) {
  ListNode *next = head->next;
  new->next = next;
  new->prev = head;
  head->next = new;
  next->prev = new;
}

// 判断链表是否为空
int list_empty(ListNode *head) { return head->next == head; }

// 从链表中删除节点
void list_del(ListNode *entry) {
  ListNode *prev = entry->prev;
  ListNode *next = entry->next;
  next->prev = prev;
  prev->next = next;
  entry->next = entry;
  entry->prev = entry;
}
