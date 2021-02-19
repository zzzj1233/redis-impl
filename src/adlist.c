#include "adlist.h"
#include "string.h"
#include "stdlib.h"
#include "sds.h"
#include <stdio.h>

list *listCreate(void) {
    list *l = malloc(sizeof(l));
    l->len = 0;
    l->free = NULL;
    l->dup = NULL;
    l->match = NULL;
    l->head = NULL;
    l->tail = NULL;
    return l;
}

list *createSdsList(void) {
    list *l = listCreate();
    l->dup = (void *(*)(void *)) sdsnew;
    l->match = (int (*)(void *, void *)) sdscmp;
    l->free = (void (*)(void *)) sdsfree;
    return l;
}

list *listAddNodeTail(list *list, void *value) {
    if (list->head == NULL) {
        list->head = malloc(sizeof(listNode));
        list->head->next = NULL;
        list->head->prev = NULL;
        list->head->value = value;
        list->tail = list->head;
        return list;
    }

    listNode *next = list->head;

    while (next->next != NULL) {
        if (list->match(next->value, value)) {
            return list;
        }
        next = next->next;
    }

    // 检查最后一个元素
    if (list->match(next->value, value)) {
        return list;
    }

    listNode *last = malloc(sizeof(listNode));
    last->next = NULL;
    last->prev = next;
    last->value = value;

    next->next = last;
    list->tail = last;

    return list;
}

list *listAddNodeHead(list *list, void *value) {
    if (list->head == NULL) {
        list->head = malloc(sizeof(listNode));
        list->head->next = NULL;
        list->head->prev = NULL;
        list->head->value = value;
        list->tail = list->head;
        return list;
    }

    listNode *next = list->head;

    // while已经检查了最后一个元素
    while (next != NULL) {
        if (list->match(next->value, value)) {
            return list;
        }
        next = next->next;
    }

    listNode *newHead = malloc(sizeof(listNode));
    newHead->prev = NULL;
    newHead->next = list->head;
    newHead->value = value;

    list->head->prev = newHead;
    list->head = newHead;

    return list;
}

struct listNode *createNode(void *value) {
    listNode *newHead = malloc(sizeof(listNode));
    newHead->prev = NULL;
    newHead->next = NULL;
    newHead->value = value;
    return newHead;
}

// after 1 = 向后插入 , else = 向前插入
list *listInsertNode(list *list, listNode *old_node, void *value, int after) {
    if (list->match(old_node->value, value)) {
        return list;
    }

    if (list->head == NULL) {
        return listAddNodeHead(list, value);
    }

    return NULL;
}

struct listNode *_listDelNode(list *list, listNode *node, listNode *cur) {
    if (cur == NULL) {
        return NULL;
    }

    if (cur == node) {
        cur->next = node->next;
        if (cur->next == NULL) {
            list->tail = cur;
        } else {
            cur->next->prev = cur;
        }
        list->free(node->value);
        free(node);
        node = NULL;
    } else {
        cur->next = _listDelNode(list, node, cur->next);
    }

    return cur;
}

void listDelNode(list *list, listNode *node) {
    list->head = _listDelNode(list, node, list->head);
}

listNode *listSearchKey(list *list, void *key) {
    if (list->head == NULL) {
        return NULL;
    }

    listNode *next = list->head;

    while (next != NULL) {
        if (list->match(next->value, key)) {
            return next;
        }
        next = next->next;
    }

    return NULL;
}

listNode *listIndex(list *list, long index) {
    if (list->head == NULL) {
        return NULL;
    }

    listNode *next = list->head;

    long idx = 0;
    while (next != NULL) {
        if (index == idx) {
            return next;
        }
        next = next->next;
        idx++;
    }

    return NULL;
}

void listRelease(list *list) {

}


int main() {
    list *l = createSdsList();

    listAddNodeTail(l, "zzzj");
    listAddNodeTail(l, "1233");
    listAddNodeTail(l, "dl");
    listAddNodeTail(l, "abc");

    printf("%s\n", l->head->value);
    printf("%s\n", l->tail->value);

    listDelNode(l, listSearchKey(l, "1233"));

    printf("%s\n", l->tail->value);

}