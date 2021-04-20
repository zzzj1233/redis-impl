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
                printf(" %.0f ", node->level[i].forward->score);
            }
            node = node->level[i].forward;
        }

        printf("\n");
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
    unsigned int rank[ZSKIPLIST_MAXLEVEL];

    // 先看是否已经存在相同的分值,存在则更新obj

    // 从上往下找,可以有几率减少查找的次数
    for (int i = zsl->level - 1; i >= 0; --i) {
        zskiplistNode *node = zsl->header;

        rank[i] = (i == zsl->level - 1) ? 0 : rank[i + 1];

        while (node != NULL && node->level[i].forward) {
            // todo 如果score相同并且obj的内容相同,那么这次插入失败 , 目前仅判断score是否相同
            if (node->level[i].forward->score == score) {
                return NULL;
            } else if (node->level[i].forward->score > score) {
                node = node->level[i].forward;
                // node->level[i].forward->score < score
            } else {
                rank[i] += node->level[i].span;
                break;
            }
        }
        update[i] = node;
    }

    int lev = zslRandomLevel();

    printf("lev = %d , score = %.0f \n", lev, score);

    if (lev > zsl->level) {
        for (int i = zsl->level; i < lev; ++i) {
            update[i] = zsl->header;
            rank[i] = 0;
        }
        zsl->level = lev;
    }

    zskiplistNode *newNode = zslCreateNode(lev, obj, score);

    for (int i = 0; i < lev; ++i) {
        zskiplistNode *forward = update[i]->level[i].forward;
        update[i]->level[i].forward = newNode;
        newNode->level[i].forward = forward;
    }

    zsl->level++;

    return newNode;
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
}