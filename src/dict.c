#include "dict.h"
#include "stdlib.h"
#include "stdio.h"

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
    dictEntry *entry = dictAddRaw(d, key);

    char ret = entry->v.val == NULL ? DICT_OK : DICT_ERR;

    entry->v.val = val;

    // 成功新增了元素 & dict没有在rehash &
    if (ret && d->rehashidx == -1 && (((double) d->ht[0].used) / ((double) d->ht[0].size)) >= LOAD_FACTOR) {
        dictExpand(d, d->ht[0].size * 2);
    }

    return ret;
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
    if (dictIsRehashing(d)) {
        dictRehash(d, 1);
    }
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


void *dictFetchValue(dict *d, const void *key) {
    dictEntry *entry = dictFind(d, key);
    if (entry != NULL) {
        return entry->v.val;
    }
    return NULL;
}

dictEntry *dictAddRaw(dict *d, void *key) {
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
        ht->table[index]->next = NULL;
        return ht->table[index];
    }

    dictEntry *entry = ht->table[index];

    if (entry == NULL) {
        ht->table[index] = malloc(sizeof(struct dictEntry));
        ht->table[index]->key = d->type->keyDup(d->privdata, key);
        ht->table[index]->next = NULL;
        return ht->table[index];
    }

    while (entry->next != NULL) {
        if (d->type->keyCompare(d->privdata, entry->key, key)) {
            // 更新value
            ht->used--;
            return entry;
        }
        entry = entry->next;
    }

    // 最后一个
    if (d->type->keyCompare(d->privdata, entry->key, key)) {
        // 更新value
        ht->used--;
        return entry;
    }

    // 没有key相同的元素,采用头插法
    dictEntry *head = ht->table[index];

    ht->table[index] = malloc(sizeof(struct dictEntry));
    ht->table[index]->key = d->type->keyDup(d->privdata, key);
    ht->table[index]->next = head;

    return ht->table[index];
}

_Bool _dictContains(dict *d, struct dictht *ht, unsigned int hash, const void *key) {
    int index = hash & ht->sizemask;

    if (ht->table == NULL || ht->table[index] == NULL) {
        return 0;
    }

    dictEntry *entry = ht->table[index];

    while (entry != NULL) {
        if (d->type->keyCompare(d->privdata, entry->key, key)) {
            return 1;
        }
        entry = entry->next;
    }

    return 0;
}

_Bool dictContains(dict *d, const void *key) {
    struct dictht *ht;

    if (dictIsRehashing(d)) {
        dictRehash(d, 1);
    }

    // 如果正在rehash,从ht[0]没有匹配后,还需要从ht[1]进行匹配

    unsigned int hash = d->type->hashFunction(key);

    if (_dictContains(d, &d->ht[0], hash, key)) {
        return 1;
    }

    return _dictContains(d, &d->ht[1], hash, key);
}

int dictDelete(dict *d, const void *key) {
    if (dictIsRehashing(d)) {
        dictRehash(d, 1);
    }

    dictht ht = d->rehashidx == -1 ? d->ht[0] : d->ht[1];

    if (ht.table == NULL) {
        return DICT_ERR;
    }

    unsigned int hash = d->type->hashFunction(key);
    int index = hash & ht.sizemask;

    if (ht.table[index] == NULL) {
        return DICT_ERR;
    }

    dictEntry *previous = ht.table[index];

    // head的特殊处理
    if (d->type->keyCompare(d->privdata, previous->key, key)) {
        d->type->keyDestructor(d->privdata, previous->key);
        ht.table[index] = previous->next;
        free(previous);
        return DICT_OK;
    }
    dictEntry *current = previous->next;

    while (current != NULL) {

        if (d->type->keyCompare(d->privdata, current->key, key)) {
            previous->next = current->next;
            d->type->keyDestructor(d->privdata, current->key);
            free(current);
            return DICT_OK;
        }

        previous = current;
        current = current->next;
    }

    return 0;
}

int dictRehash(dict *d, int n) {
    // 没在rehash
    if (d->rehashidx == -1) {
        return DICT_ERR;
    }

    // rehash结束了
    if (d->ht[0].used == 0) {
        free(d->ht[0].table);
        d->ht[0] = d->ht[1];
        initDictHt(d->ht + 1);
        d->rehashidx = -1;
        return DICT_OK;
    }


    while (n--) {

        // 跳过空的index
        while (!d->ht[0].table[d->rehashidx]) {
            d->rehashidx++;
        }

        // rehash
        dictEntry *entry = d->ht[0].table[d->rehashidx];

        while (entry != NULL) {
            int index = d->type->hashFunction(entry->key) & d->ht[1].sizemask;
            dictEntry *next = entry->next;
            d->ht[0].table[d->rehashidx] = next;

            entry->next = d->ht[1].table[index];
            d->ht[1].table[index] = entry;

            d->ht[0].used--;
            d->ht[1].used++;
            entry = next;
        }

        d->ht[0].table[d->rehashidx] = NULL;
        d->rehashidx++;
    }

    return DICT_OK;
}

int dictExpand(dict *d, unsigned long size) {
    d->ht[1].size = size;
    d->ht[1].sizemask = size - 1;
    d->ht[1].table = calloc(size, sizeof(dictEntry *));
    d->ht[1].used = 0;
    d->rehashidx = 0;
    return DICT_OK;
}

int main() {

    dict *dic = dictCreate(&sdsDictType, NULL);

    dictAdd(dic, sdsnew("name"), sdsnew("zzzj"));
    dictAdd(dic, sdsnew("age"), sdsnew("22"));
    dictAdd(dic, sdsnew("gender"), sdsnew("男"));

    printf("name = %d \n", dictContains(dic, sdsnew("name")));
    printf("age = %d \n", dictContains(dic, sdsnew("age")));
    printf("gender = %d \n", dictContains(dic, sdsnew("gender")));
    printf("haha = %d \n", dictContains(dic, sdsnew("haha")));

    dictDelete(dic, sdsnew("age"));

    printf("age = %d \n", dictContains(dic, sdsnew("age")));
}