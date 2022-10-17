//
// Created by bear on 9/24/2022.
//
#pragma once
#include "types.h"
#include "defs.h"

#define LIST_HEAD(name) \
        list_head_t name = LIST_HEAD_INIT(name)

#define  LIST_HEAD_INIT(name) \
    {&(name), &(name)}

static inline void __list_add(list_head_t *entry, list_head_t *prev, list_head_t *next)
{
	prev->next = entry;
	entry->prev = prev;
	entry->next = next;
	next->prev = entry;
}

static inline void list_init(list_head_t *head)
{
	head->next = head;
	head->prev = head;
}

static inline void list_add(list_head_t *entry, list_head_t *head)
{
	__list_add(entry, head, head->next);
}

static inline void list_add_tail(list_head_t *entry, list_head_t *head)
{
	__list_add(entry, head->prev, head);
}

static inline void __list_del(list_head_t *prev, list_head_t *next)
{
	prev->next = next;
	next->prev = prev;
}

static inline void list_del(list_head_t *entry)
{
	__list_del(entry->prev, entry->next);
	entry->prev = (list_head_t *)LIST_POISON1;
	entry->next = (list_head_t *)LIST_POISON2;
}

static inline void list_replace(list_head_t *old, list_head_t *neo)
{
	neo->next = old->next;
	neo->next->prev = neo;

	neo->prev = old->prev;
	neo->prev->next = neo;
}

static inline void list_move(list_head_t *list, list_head_t *head)
{
	list_del(list);
	list_add(list, head);
}

static inline void list_move_tail(list_head_t *list, list_head_t *head)
{
	list_del(list);
	list_add_tail(list, head);
}

static inline int list_is_last(list_head_t *list, list_head_t *head)
{
	return list->next == head;
}

static inline int list_empty(list_head_t *head)
{
	return head == head->next;
}

static inline void list_rotate_left(list_head_t *head)
{
	list_head_t *first;

	if (!list_empty(head))
	{
		first = head->next;
		list_move_tail(first, head);
	}
}

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
    list_entry((ptr)->prev, type, member)

#define list_first_entry_or_null(ptr, type, member) ({ \
    struct list_head *head__ = (ptr); \
    struct list_head *pos__ = READ_ONCE(head__->next); \
    pos__ != head__ ? list_entry(pos__, type, member) : NULL; \
})

#define list_next_entry(pos, member) \
    list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_prev_entry(pos, member) \
    list_entry((pos)->member.prev, typeof(*(pos)), member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_prev(pos, head) \
    for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

#define list_for_each_prev_safe(pos, n, head) \
    for (pos = (head)->prev, n = pos->prev; \
         pos != (head); \
         pos = n, n = pos->prev)



