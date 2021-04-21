#include "skplist.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "windows.h"

zskiplistNode *zslCreateNode(int level, void *obj, double score) {
    zskiplistNode *node = malloc(sizeof(struct zskiplistNode) + level * sizeof(struct zskiplistLevel));
    node->backward = NULL;
    node->obj = obj;
    node->score = score;
    return node;
}

void zslRepr(zskiplist *zsl) {
    for (int i = 0; i < zsl->level; ++i) {
        zskiplistNode *node = zsl->header;

        while (node != NULL) {
            if (node->level[i].forward != NULL) {
                printf("score =  %.0f , span = %d | ", node->level[i].forward->score, node->level[i].span);
            }
            node = node->level[i].forward;
        }

        printf("\n");
    }
}

void nodeRepr(struct zskiplistNode *node) {
    if (node == NULL) {
        printf("node is null \n");
    } else {
        printf("score =  %.0f  , value = %d \n", node->score, *((int *) node->obj));
    }
}

int zslRandomLevel(void) {
    int level = 1;

    struct timespec timestamp;
    clock_gettime(CLOCK_REALTIME, &timestamp);

    srand((unsigned) timestamp.tv_nsec);

    while ((rand() & 0xFFFF) < (ZSKIPLIST_P * 0xFFFF)) {
        level += 1;
    }

    return (level < ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
}

zskiplist *zslCreate(void) {
    zskiplist *zsl = malloc(sizeof(*zsl));
    zsl->tail = NULL;
    zsl->length = 0;
    zsl->level = 1;

    zsl->header = zslCreateNode(ZSKIPLIST_MAXLEVEL, NULL, 0);

    for (int i = 0; i < ZSKIPLIST_MAXLEVEL; ++i) {
        zsl->header->level[i].forward = NULL;
        zsl->header->level[i].span = 0;
    }

    return zsl;
}


zskiplistNode *zslInsert(zskiplist *zsl, double score, void *obj) {
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL];
    unsigned int rank[ZSKIPLIST_MAXLEVEL] = {0};

    // 先看是否已经存在相同的分值,存在则更新obj

    // 从上往下找,可以有几率减少查找的次数
    for (int i = zsl->level - 1; i >= 0; --i) {
        zskiplistNode *node = zsl->header;

        while (node != NULL && node->level[i].forward) {
            // todo 如果score相同并且obj的内容相同,那么这次插入失败 , 目前仅判断score是否相同
            if (node->level[i].forward->score == score) {
                return NULL;
            } else if (node->level[i].forward->score < score) {
                rank[i] += node->level[i].span;
                node = node->level[i].forward;
                // node->level[i].forward->score > score
            } else {
                break;
            }
        }
        update[i] = node;
    }

    int lev = zslRandomLevel();

    if (lev > zsl->level) {
        for (int i = zsl->level; i < lev; ++i) {
            update[i] = zsl->header;
            rank[i] = 0;
        }
        zsl->level = lev;
    }

    zskiplistNode *newNode = zslCreateNode(lev, obj, score);

    zsl->length++;

    for (int i = 0; i < lev; ++i) {
        zskiplistNode *forward = update[i]->level[i].forward;
        update[i]->level[i].forward = newNode;
        update[i]->level[i].span = zsl->length - rank[i];
        newNode->level[i].forward = forward;
    }

    // update[0]就是第一层
    newNode->backward = update[0] == zsl->header ? NULL : update[0];

    if (newNode->level[0].forward) {
        newNode->level[0].forward->backward = newNode;
    } else {
        // 没有下一个元素了,当前元素就是第一层的最后一个元素
        zsl->tail = newNode;
    }

    return newNode;
}

// 如果给定的分值范围包含在跳跃表的分值范围之内,返回1,否则返回0
int zslIsInRange(zskiplist *zsl, zrangespec *range) {
    if (range->min > range->max
        || (range->min == range->max && (range->maxex || range->minex))) {
        return 0;
    }

    if (zsl->header->level[0].forward == NULL) {
        return 0;
    }

    zskiplistNode *head = zsl->header->level[0].forward;

    // zsl列表最小的元素的score大于range.max
    if (head->score >= range->max) {
        // 如果max包含,那么返回判断是否是 == , 否则返回0
        return !range->maxex ? head->score == range->max : 0;
    }

    // zsl列表最大的元素的score小于range.min
    if (zsl->tail->score <= range->min) {
        return !range->minex ? zsl->tail->score == range->min : 0;
    }

    return 1;
}

// 返回 zsl 中第一个分值符合 range 中指定范围的节点
// 如果 zsl 中没有符合范围的节点,返回 NULL
zskiplistNode *zslFirstInRange(zskiplist *zsl, zrangespec *range) {

    // 1. 超出范围,直接返回
    if (!zslIsInRange(zsl, range)) {
        return NULL;
    }

    // 2. 遍历skpList
    for (int i = zsl->level - 1; i >= 0; --i) {
        zskiplistNode *node = zsl->header->level[i].forward;

        while (node) {
            // 找到第一个比range.min大的元素
            if (node->score >= range->min) {
                if (node->score == range->min) {
                    // 包含最小值
                    if (!range->minex) {
                        return node;
                    }
                    // >
                } else {
                    return node;
                }
            }
            node = node->level[i].forward;
        }
    }

    // redisLog
    exit(1);
}

int main() {
    zskiplist *zsl = zslCreate();
    int num1 = 1, num2 = 2, num3 = 3, num4 = 4;
    zslInsert(zsl, 4, &num4);
    Sleep(50);
    zslInsert(zsl, 3, &num3);
    Sleep(50);
    zslInsert(zsl, 2, &num2);
    Sleep(50);
    zslInsert(zsl, 1, &num1);

    zslRepr(zsl);

    zrangespec range = {
            1, 2, 0, 0
    };

    zskiplistNode *node = zslFirstInRange(zsl, &range);
    nodeRepr(node);
}