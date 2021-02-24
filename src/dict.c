#include "dict.h"
#include "stdlib.h"
#include "stdio.h"
#include "sds.h"

void initDictHt(dictht *ht) {
    ht->size = DICT_HT_INITIAL_SIZE;
    ht->sizemask = ht->size - 1;
    ht->used = 0;
    ht->table = NULL;
}

dict *dictCreate(dictType *type, void *privDataPtr) {
    dict *dic = malloc(sizeof(dict));
    dic->type = type;
    // 非-1表示正在rehash
    dic->rehashidx = -1;
    dic->iterators = 0;
    dic->privdata = privDataPtr;
    initDictHt(dic->ht);
    initDictHt(dic->ht + 1);
    return dic;
}

int dictAdd(dict *d, void *key, void *val) {
    unsigned int hash = d->type->hashFunction(key);

    struct dictht *ht;

    if (d->rehashidx == -1) {
        ht = d->ht;
    } else {
        ht = d->ht + 1;
    }

    int index = hash & ht->sizemask;

    ht->used++;

    if (ht->table == NULL) {
        ht->table = malloc(ht->size * sizeof(struct dictEntry *));
        memset(ht->table, 0x00, ht->size * sizeof(struct dictEntry *));
        ht->table[index] = malloc(sizeof(struct dictEntry));
        ht->table[index]->key = d->type->keyDup(d->privdata, key);
        ht->table[index]->v.val = d->type->valDup(d->privdata, val);
        ht->table[index]->next = NULL;
        return DICT_OK;
    }

    dictEntry *entry = ht->table[index];

    if (entry == NULL) {
        ht->table[index] = malloc(sizeof(struct dictEntry));
        ht->table[index]->key = d->type->keyDup(d->privdata, key);
        ht->table[index]->v.val = d->type->valDup(d->privdata, val);
        ht->table[index]->next = NULL;
        return DICT_OK;
    }

    while (entry->next != NULL) {
        if (d->type->keyCompare(d->privdata, entry->key, key)) {
            // 更新value
            entry->v.val = val;
            ht->used--;
            return DICT_OK;
        }
        entry = entry->next;
    }

    // 最后一个
    if (d->type->keyCompare(d->privdata, entry->key, key)) {
        // 更新value
        entry->v.val = val;
        ht->used--;
        return DICT_OK;
    }

    // 没有key相同的元素,采用头插法
    dictEntry *head = ht->table[index];

    ht->table[index] = malloc(sizeof(struct dictEntry));
    ht->table[index]->key = d->type->keyDup(d->privdata, key);
    ht->table[index]->v.val = d->type->valDup(d->privdata, val);
    ht->table[index]->next = head;

    return DICT_OK;
}

dictEntry *htFind(dict *d, struct dictht *ht, const void *key) {
    if (ht != NULL && ht->table != NULL) {
        int idx = d->type->hashFunction(key) & ht->sizemask;

        if (ht->table[idx] != NULL) {

            dictEntry *entry = ht->table[idx];

            while (entry != NULL) {
                if (d->type->keyCompare(d->privdata, entry->key, key)) {
                    return entry;
                }
                entry = entry->next;
            }
        }
    }

    return NULL;
}

dictEntry *dictFind(dict *d, const void *key) {
    // 如果正在rehash,从先0里面找,再到1里面找

    _Bool isRehashing = d->rehashidx != -1;

    dictEntry *ret = htFind(d, d->ht, key);

    if (ret != NULL) {
        return ret;
    }

    if (!isRehashing) {
        return NULL;
    }

    ret = htFind(d, d->ht + 1, key);

    return ret;
}

int main() {

    dict *dic = dictCreate(&sdsDictType, NULL);

    dictAdd(dic, sdsnew("name"), sdsnew("zzzj"));
    dictAdd(dic, sdsnew("age"), sdsnew("22"));
    dictAdd(dic, sdsnew("gender"), sdsnew("男"));

    printf("name = %s \n", dictFind(dic, sdsnew("name"))->v.val);
    printf("age = %s \n", dictFind(dic, sdsnew("age"))->v.val);
    printf("gender = %s \n", dictFind(dic, sdsnew("gender"))->v.val);
    printf("hahaha = %d \n", dictFind(dic, sdsnew("hahaha")) == NULL);
}