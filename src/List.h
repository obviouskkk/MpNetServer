#ifndef __LIST_H__
#define __LIST_H__
#include "stdio.h"

typedef struct list_node
{
	struct list_node* prev;
	struct list_node* next;
}list_node;

typedef list_node list_t;
//使用宏，避免类型检查
#define LIST_INIT(head) list_node head = { &(head), &(head) }

//初始化链表头节点
#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

static void list_push(list_node* new_node, list_node* prev, list_node* next)
{
  next->prev = new_node;
  new_node->next = next;
  new_node->prev = prev;
  prev->next = new_node;
}

//头插
static void list_push_front(list_node* new_node, list_node* head)
{
	list_push(new_node, head->prev, head);
}

//尾插
static void list_push_tail(list_node* new_node, list_node* head)
{
	list_push(new_node, head->prev, head);
}

//删除一段链表
static void list_del(list_node* prev, list_node* next)
{
	next->prev = prev;
	prev->next = next;
}

//删除一个节点
static void list_del_node(list_node* node)
{
	list_del (node->prev, node->next);
	node->next = NULL;
	node->prev = NULL;
}

static inline void list_del_init (list_t *entry)
{
	list_del (entry->prev, entry->next);
	INIT_LIST_HEAD (entry);
}

//判断链表是否为空
static inline int list_empty(list_node* head)
{
	return head->next == head;
}

#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_for_each_entry(pos, head, member) \
		for (pos = list_entry((head)->next, typeof(*pos), member);	\
		     &pos->member != (head); 					\
		     pos = list_entry(pos->member.next, typeof(*pos), member) )

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)


#endif
