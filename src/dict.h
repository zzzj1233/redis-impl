#include <stdint.h>
#include <string.h>
#include "sds.h"

#ifndef __DICT_H
#define __DICT_H
#define LOAD_FACTOR 0.7
/*
 * 字典的操作状态
 */
// 操作成功
#define DICT_OK 0
// 操作失败（或出错）
#define DICT_ERR 1


/*
 * 哈希表节点
 */
typedef struct dictEntry {

    // 键
    void *key;

    // 值
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
    } v;

    // 指向下个哈希表节点，形成链表
    struct dictEntry *next;

} dictEntry;


/*
 * 字典类型特定函数
 */
typedef struct dictType {

    // 计算哈希值的函数
    unsigned int (*hashFunction)(const void *key);

    // 复制键的函数
    void *(*keyDup)(void *privdata, const void *key);

    // 复制值的函数
    void *(*valDup)(void *privdata, const void *obj);

    // 对比键的函数
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);

    // 销毁键的函数
    void (*keyDestructor)(void *privdata, void *key);

    // 销毁值的函数
    void (*valDestructor)(void *privdata, void *obj);

} dictType;


/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
/*
 * 哈希表
 *
 * 每个字典都使用两个哈希表，从而实现渐进式 rehash 。
 */
typedef struct dictht {

    // 哈希表数组
    dictEntry **table;

    // 哈希表大小
    unsigned long size;

    // 哈希表大小掩码，用于计算索引值
    // 总是等于 size - 1
    unsigned long sizemask;

    // 该哈希表已有节点的数量
    unsigned long used;

} dictht;

/*
 * 字典
 */
typedef struct dict {

    // 类型特定函数
    dictType *type;

    // 私有数据
    void *privdata;

    // 哈希表
    dictht ht[2];

    // rehash 索引
    // 当 rehash 不在进行时，值为 -1
    int rehashidx; /* rehashing not in progress if rehashidx == -1 */

    // 目前正在运行的安全迭代器的数量
    int iterators; /* number of iterators currently running */

} dict;

/*
 * 字典迭代器
 *
 * 如果 safe 属性的值为 1 ，那么在迭代进行的过程中，
 * 程序仍然可以执行 dictAdd 、 dictFind 和其他函数，对字典进行修改。
 *
 * 如果 safe 不为 1 ，那么程序只会调用 dictNext 对字典进行迭代，
 * 而不对字典进行修改。
 */
typedef struct dictIterator {

    // 被迭代的字典
    dict *d;

    // table ：正在被迭代的哈希表号码，值可以是 0 或 1 。
    // index ：迭代器当前所指向的哈希表索引位置。
    // safe ：标识这个迭代器是否安全
    int table, index, safe;

    // zlentry ：当前迭代到的节点的指针
    // nextEntry ：当前迭代节点的下一个节点
    //             因为在安全迭代器运作时， zlentry 所指向的节点可能会被修改，
    //             所以需要一个额外的指针来保存下一节点的位置，
    //             从而防止指针丢失
    dictEntry *entry, *nextEntry;

    long long fingerprint; /* unsafe iterator fingerprint for misuse detection */
} dictIterator;

typedef void (dictScanFunction)(void *privdata, const dictEntry *de);

/* This is the initial size of every hash table */
/*
 * 哈希表的初始大小
 */
#define DICT_HT_INITIAL_SIZE     4

/* API */
dict *dictCreate(dictType *type, void *privDataPtr);

int dictExpand(dict *d, unsigned long size);

int dictAdd(dict *d, void *key, void *val);

dictEntry *dictAddRaw(dict *d, void *key);

int dictReplace(dict *d, void *key, void *val);

dictEntry *dictReplaceRaw(dict *d, void *key);

int dictDelete(dict *d, const void *key);

int dictDeleteNoFree(dict *d, const void *key);

void dictRelease(dict *d);

dictEntry *dictFind(dict *d, const void *key);

void *dictFetchValue(dict *d, const void *key);

int dictResize(dict *d);

dictIterator *dictGetIterator(dict *d);

dictIterator *dictGetSafeIterator(dict *d);

dictEntry *dictNext(dictIterator *iter);

void dictReleaseIterator(dictIterator *iter);

dictEntry *dictGetRandomKey(dict *d);

int dictGetRandomKeys(dict *d, dictEntry **des, int count);

void dictPrintStats(dict *d);

unsigned int dictGenHashFunction(const void *key, int len);

unsigned int dictGenCaseHashFunction(const unsigned char *buf, int len);

void dictEmpty(dict *d, void(callback)(void *));

void dictEnableResize(void);

void dictDisableResize(void);

int dictRehash(dict *d, int n);

int dictRehashMilliseconds(dict *d, int ms);

void dictSetHashFunctionSeed(unsigned int initval);

unsigned int dictGetHashFunctionSeed(void);

unsigned long dictScan(dict *d, unsigned long v, dictScanFunction *fn, void *privdata);

static uint32_t dict_hash_function_seed = 5381;

unsigned int dictGenHashFunction(const void *key, int len) {
    uint32_t seed = dict_hash_function_seed;
    const uint32_t m = 0x5bd1e995;
    const int r = 24;

    uint32_t h = seed ^len;

    const unsigned char *data = (const unsigned char *) key;

    while (len >= 4) {
        uint32_t k = *(uint32_t *) data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch (len) {
        case 3:
            h ^= data[2] << 16;
        case 2:
            h ^= data[1] << 8;
        case 1:
            h ^= data[0];
            h *= m;
    };

    /* Do a few final mixes of the hash to ensure the last few
     * bytes are well-incorporated. */
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return (unsigned int) h;
}

unsigned int sdsHashFun(const void *key) {
    return dictGenHashFunction(key, sdslen(key));
}

void *sdsDup0(void *privdata, const void *key) {
    return sdsdup(key);
}

int sdsCmp0(void *privdata, const void *key, const void *value) {
    return sdscmp(key, value);
}

void sdsFree(void *privdata, void *val) {
    sdsfree(val);
}

_Bool dictIsRehashing(dict *d) {
    return d->rehashidx != -1;
}

struct dictType sdsDictType = {
        sdsHashFun,
        sdsDup0,
        sdsDup0,
        sdsCmp0,
        sdsFree,
        sdsFree
};

#endif /* __DICT_H */
