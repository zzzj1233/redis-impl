#ifndef __ADLIST_H__
#define __ADLIST_H__


/*
 * 双端链表节点
 */
typedef struct listNode {

    // 前置节点
    struct listNode *prev;

    // 后置节点
    struct listNode *next;

    // 节点的值
    void *value;

} listNode;

/*
 * 双端链表迭代器
 */
typedef struct listIter {

    // 当前迭代到的节点
    listNode *next;

    // 迭代的方向
    int direction;

} listIter;

/*
 * 双端链表结构
 */
typedef struct list {

    // 表尾节点
    listNode *tail;

    // 表头节点
    listNode *head;

    // 节点值复制函数
    void *(*dup)(void *ptr);

    // 节点值释放函数
    void (*free)(void *ptr);

    // 节点值对比函数
    int (*match)(void *ptr, void *key);

    // 链表所包含的节点数量
    unsigned long len;

} list;

/* Prototypes */
list *listCreate(void);

void listRelease(list *list);

list *listAddNodeHead(list *list, void *value);

list *listAddNodeTail(list *list, void *value);

list *listInsertNode(list *list, listNode *old_node, void *value, int after);

void listDelNode(list *list, listNode *node);

listIter *listGetIterator(list *list, int direction);

listNode *listNext(listIter *iter);

void listReleaseIterator(listIter *iter);

list *listDup(list *orig);

listNode *listSearchKey(list *list, void *key);

listNode *listIndex(list *list, long index);

void listRewind(list *list, listIter *li);

void listRewindTail(list *list, listIter *li);

void listRotate(list *list);

/* Directions for iterators 
 *
 * 迭代器进行迭代的方向
 */
// 从表头向表尾进行迭代
#define AL_START_HEAD 0
// 从表尾到表头进行迭代
#define AL_START_TAIL 1

#endif /* __ADLIST_H__ */
