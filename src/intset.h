#ifndef __INTSET_H
#define __INTSET_H

#include <stdint.h>

/*
 * intset 的编码方式
 */
#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))

/* Return the required encoding for the provided value.
 *
 * 返回适用于传入值 v 的编码方式
 *
 * T = O(1)
 */
static uint8_t _intsetValueEncoding(int64_t v) {
    if (v < INT32_MIN || v > INT32_MAX)
        return INTSET_ENC_INT64;
    else if (v < INT16_MIN || v > INT16_MAX)
        return INTSET_ENC_INT32;
    else
        return INTSET_ENC_INT16;
}

typedef struct intset {

    // 编码方式
    uint32_t encoding;

    // 集合包含的元素数量
    uint32_t length;

    // 保存元素的数组
    int8_t contents[];

} intset;

intset *intsetNew(void);

intset *intsetAdd(intset *is, int64_t value, uint8_t *success);

intset *intsetRemove(intset *is, int64_t value, int *success);

uint8_t intsetFind(intset *is, int64_t value);

int64_t intsetRandom(intset *is);

uint8_t intsetGet(intset *is, uint32_t pos, int64_t *value);

uint32_t intsetFindIndex(intset *is, int64_t value);

uint32_t intsetLen(intset *is);

uint32_t intsetBlobLen(intset *is);

#endif // __INTSET_H
