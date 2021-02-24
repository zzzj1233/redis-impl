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
    list->len++;
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
    list->len++;
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

    // 如果没有节点
    if (list->head == NULL) {
        return listAddNodeHead(list, value);
    }

    listNode *newNode = createNode(value);

    if (after) {
        if (old_node->next == NULL) {
            newNode->prev = old_node;
            old_node->next = newNode;
            list->tail = newNode;
        } else {
            listNode *next = old_node->next;
            old_node->next = newNode;
            newNode->prev = old_node;
            newNode->next = next;
        }
    } else {
        if (old_node->prev == NULL) {
            newNode->next = old_node;
            old_node->prev = newNode;
            list->head = newNode;
        } else {
            listNode *previous = old_node->prev;
            previous->next = newNode;
            newNode->prev = previous;
            newNode->next = old_node;
            old_node->prev = newNode;
        }
    }

    list->len++;
    return list;
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
        list->len--;
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

listIter *listGetIterator(list *list, int direction) {
    listIter *iter = malloc(sizeof(listIter));

    iter->direction = direction;

    // 从头开始遍历
    if (direction == AL_START_HEAD) {
        iter->next = list->head;

        // 从尾开始遍历
    } else {
        iter->next = list->tail;
    }

    return iter;
}

void listReleaseIterator(listIter *iter) {
    free(iter);
}

listNode *listNext(listIter *iter) {
    listNode *node = iter->next;

    if (node != NULL) {
        if (iter->direction == AL_START_HEAD) {
            iter->next = node->next;
        } else {
            iter->next = node->prev;
        }
    }

    return node;
}

list *listDup(list *orig) {
    // 1. 计算出申请内存的大小

    list *cpy = malloc(sizeof(struct list));
    cpy->len = orig->len;
    cpy->dup = orig->dup;
    cpy->free = orig->free;
    cpy->match = orig->match;

    listNode **ptr = malloc(sizeof(*ptr) * orig->len);

    listIter *iter = listGetIterator(orig, AL_START_HEAD);

    listNode *next = NULL;
    long idx = 0;

    while ((next = listNext(iter)) != NULL) {
        ptr[idx] = malloc(sizeof(listNode));
        ptr[idx]->value = orig->dup(next->value);
        ptr[idx]->prev = NULL;
        ptr[idx]->next = NULL;
        idx++;
    }

    for (int i = 1; i < idx; ++i) {
        ptr[i]->prev = ptr[i - 1];
        ptr[i - 1]->next = ptr[i];
    }

    cpy->head = ptr[0];
    cpy->tail = ptr[idx - 1];

    return cpy;
}

void listRewind(list *list, listIter *li){
    li->next = list->head;
    li->direction = AL_START_HEAD;
}

void listRewindTail(list *list, listIter *li){
    li->next = list->tail;
    li->direction = AL_START_TAIL;
}

void listRelease(list *list) {
    struct listIter *iter = listGetIterator(list,AL_START_HEAD);

    listNode *next = NULL;

    while ((next = listNext(iter)) != NULL) {
        list->free(next->value);
        free(next);
    }

    listReleaseIterator(iter);
    free(list);
}

void listRotate(list *list){

}


int main() {
    list *l = createSdsList();
    listAddNodeTail(l, "zzzj");
    listAddNodeTail(l, "1233");
    listAddNodeTail(l, "dl");
    listAddNodeTail(l, "abc");
    listInsertNode(l, listSearchKey(l, "abc"), "hhh", 0);

    listIter *iter = listGetIterator(l, AL_START_HEAD);

    listNode *next = NULL;

    while ((next = listNext(iter)) != NULL) {
        printf("item = %s \n", next->value);
    }

    list *cpy = listDup(l);

    iter = listGetIterator(cpy, AL_START_HEAD);

    next = NULL;


    while ((next = listNext(iter)) != NULL) {
        printf("item = %s \n", next->value);
    }

}