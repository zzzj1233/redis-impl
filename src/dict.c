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

    entry->v.val = d->type->keyDup(d->privdata, val);

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


int _dictDelete(dict *d, const void *key, int needFree) {
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
        if (needFree) {
            d->type->keyDestructor(d->privdata, previous->key);
            d->type->valDestructor(d->privdata, previous->v.val);
        }
        free(previous);
        return DICT_OK;
    }

    dictEntry *current = previous->next;

    while (current != NULL) {

        if (d->type->keyCompare(d->privdata, current->key, key)) {
            previous->next = current->next;
            d->type->keyDestructor(d->privdata, current->key);
            if (needFree) {
                d->type->keyDestructor(d->privdata, current->key);
                d->type->valDestructor(d->privdata, current->v.val);
            }
            free(current);
            return DICT_OK;
        }

        previous = current;
        current = current->next;
    }

    return 0;
}

int dictDelete(dict *d, const void *key) {
    return _dictDelete(d, key, 1);
}

int dictDeleteNoFree(dict *d, const void *key) {
    return _dictDelete(d, key, 0);
}

void _dictClearEntry(dict *d, dictEntry *entry) {
    if (entry != NULL) {
        d->type->keyDestructor(d->privdata, entry->key);
        d->type->valDestructor(d->privdata, entry->v.val);
        if (entry->next != NULL) {
            _dictClearEntry(d, entry->next);
        }
        free(entry);
    }
}

void _dictClear(dict *d) {
    if (d == NULL) {
        return;
    }
    for (int i = 0; i <= 1; ++i) {
        dictht ht = d->ht[i];
        if (ht.used > 0) {
            for (int j = 0; j < ht.size; ++j) {
                dictEntry *entry = ht.table[j];
                _dictClearEntry(d, entry);
            }
            free(ht.table);
            initDictHt(&ht);
        }
    }
}

void dictRelease(dict *d) {
    _dictClear(d);
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

int dictReplace(dict *d, void *key, void *val) {
    dictEntry *entry = dictFind(d, key);

    if (entry == NULL) {
        dictAdd(d, key, val);
        return 1;
    }

    d->type->valDestructor(d->privdata, entry->v.val);

    entry->v.val = d->type->valDup(d->privdata, val);

    return 0;
}

// 存在则返回,不存在则新增一个没有value的entry
dictEntry *dictReplaceRaw(dict *d, void *key) {
    dictEntry *entry = dictFind(d, key);

    return entry != NULL ? entry : dictAddRaw(d, key);
}

void dictEntryPureRepr(dictEntry *entry) {
    if (entry == NULL) {
        return;
    } else {
        printf("k %s , v = %s , ", entry->key, entry->v.val);
    }
}

void dictEntryRepr(dictEntry *entry) {
    if (entry == NULL) {
        return;
    } else {
        printf("k %s , v = %s , ", entry->key, entry->v.val);
        if (entry->next != NULL) {
            dictEntryRepr(entry->next);
        }
    }
}

void dictRepr(dict *d) {
    printf("dict : rehashIndex = %d \n", d->rehashidx);
    printf("ht0 : size = %lu, used = %lu : \n\t", d->ht[0].size, d->ht[0].used);
    for (int i = 0; i < d->ht[0].size; ++i) {
        printf(" %d : ", i);
        dictEntryRepr(d->ht[0].table[i]);
    }
    printf("\n================================================================\n");
    printf("ht1 : size = %lu, used = %lu \n\t", d->ht[1].size, d->ht[1].used);
    for (int i = 0; i < d->ht[1].size; ++i) {
        printf(" %d : ", i);
        dictEntryRepr(d->ht[1].table[i]);
    }
    printf("\n");
}

dict *initDict() {
    dict *d = dictCreate(&sdsDictType, NULL);

    dictAdd(d, sdsnew("name"), sdsnew("zzzj"));
    dictAdd(d, sdsnew("age"), sdsnew("23"));
    dictAdd(d, sdsnew("gender"), sdsnew("男"));
    dictAdd(d, sdsnew("hobby"), sdsnew("暂无"));
    return d;
}

dictIterator *dictGetIterator(dict *d) {
    dictIterator *iter = malloc(sizeof(*iter));
    iter->d = d;
    iter->table = 0;
    iter->index = -1;
    iter->entry = NULL;
    iter->nextEntry = NULL;
    iter->fingerprint = 0;
    iter->safe = 0;
    return iter;
}

dictIterator *dictGetSafeIterator(dict *d) {
    dictIterator *iter = dictGetIterator(d);
    iter->safe = 1;
    return iter;
}

dictEntry *dictNext(dictIterator *iter) {

    while (1) {
        if (iter->entry == NULL) {
            // 第一次执行
            if (iter->index == -1 && iter->table == 0) {
                // 安全迭代器记录持有数量
                if (iter->safe) {
                    iter->d->iterators++;
                } else {

                }
                // 非安全迭代器记录指纹
            }

            iter->index++;

            // 迭代完毕
            if (iter->index >= iter->d->ht[iter->table].size) {
                // 第一个table迭代完了,并且正在rehash,那么去第二个table迭代
                if (dictIsRehashing(iter->d) && iter->table == 0) {
                    iter->table = 1;
                    iter->index = -1;
                    // 迭代完了
                } else {
                    return NULL;
                }
            } else {
                // 获取table[index]处的entry,可能为空,然后再走一次循环
                iter->entry = iter->d->ht[iter->table].table[iter->index];
            }
        } else {
            // 可能为空,然后再走一次循环
            iter->entry = iter->nextEntry;
        }

        if (iter->entry != NULL) {
            iter->nextEntry = iter->entry->next;
            return iter->entry;
        }
    }
}

void dictReleaseIterator(dictIterator *iter) {
    if (iter->index != -1 || iter->table != 0) {
        if (iter->safe) {
            iter->d->iterators--;
        } else {
            // 检查指纹是否一致
        }
    }
    // 无需释放iter->d和iter-entry & iter->nextEntry
    // 所以直接是否iter即可
    free(iter);
}

dictEntry *dictGetRandomKey(dict *d) {

}

void testReplace() {
    dict *d = initDict();
    dictReplace(d, sdsnew("name"), sdsnew("dl"));
    dictRepr(d);
}

void testDelete() {
    dict *d = initDict();
    dictDelete(d, sdsnew("name"));
    dictRepr(d);
}

void testIter() {
    dict *d = initDict();
    dictIterator *iter = dictGetIterator(d);
    dictEntry *entry = NULL;
    dictRepr(d);
    while ((entry = dictNext(iter)) != NULL) {
        dictEntryPureRepr(entry);
    }
    dictReleaseIterator(iter);
}

int main() {
    // testReplace();
    // testDelete();
    testIter();
}