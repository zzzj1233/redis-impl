#include "intset.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

void printIsContent(intset *is) {
    if (is->encoding == INTSET_ENC_INT16) {
        for (int i = 0; i < is->length; ++i) {
            printf(" %d , ", ((uint16_t *) is->contents)[i]);
        }
    } else if (is->encoding == INTSET_ENC_INT32) {
        for (int i = 0; i < is->length; ++i) {
            printf(" %d , ", ((uint32_t *) is->contents)[i]);
        }
    } else {
        for (int i = 0; i < is->length; ++i) {
            printf(" %llu , ", ((uint64_t *) is->contents)[i]);
        }
    }
    printf("\n");
}

intset *intsetNew(void) {
    intset *is = calloc(1, sizeof(*is));
    is->encoding = INTSET_ENC_INT16;
    return is;
}

uint64_t intsetGetVal(intset *is, int idx, uint32_t encoding) {
    if (encoding == INTSET_ENC_INT16) {
        return ((uint16_t *) is->contents)[idx];
    } else if (encoding == INTSET_ENC_INT32) {
        return ((uint32_t *) is->contents)[idx];
    } else {
        return ((uint64_t *) is->contents)[idx];
    }
}

void intsetSetValue(intset *is, int64_t value, int idx) {
    if (is->encoding == INTSET_ENC_INT16) {
        ((uint16_t *) is->contents)[idx] = value;
    } else if (is->encoding == INTSET_ENC_INT32) {
        ((uint32_t *) is->contents)[idx] = value;
    } else {
        ((uint64_t *) is->contents)[idx] = value;
    }
}

static void intsetMoveTail(intset *is, uint32_t index) {
    // index + 1的元素全部向前挪
    void *dest, *src;
    unsigned int bytes;

    if (is->encoding == INTSET_ENC_INT16) {
        dest = ((uint16_t *) is->contents) + index;
        src = ((uint16_t *) is->contents) + index + 1;
        bytes = (is->length - index + 1) * sizeof(uint16_t);
    } else if (is->encoding == INTSET_ENC_INT32) {
        dest = ((uint32_t *) is->contents) + index;
        src = ((uint32_t *) is->contents) + index + 1;
        bytes = (is->length - index + 1) * sizeof(uint32_t);
    } else {
        dest = ((uint64_t *) is->contents) + index;
        src = ((uint64_t *) is->contents) + index + 1;
        bytes = (is->length - index + 1) * sizeof(uint64_t);
    }

    memmove(dest, src, bytes);
}

uint8_t intsetGet(intset *is, uint32_t pos, int64_t *value) {
    if (pos < 0 || pos >= is->length) {
        return 0;
    }
    *value = intsetGetVal(is, pos, is->encoding);
    return 1;
}


intset *upgradeAndSet(intset *is, uint32_t oldEncoding, uint64_t value) {

    // 如果新值小于0,那么一定是最小的数
    // 否则一定是最大的数
    _Bool insertHead = value < 0 ? 1 : 0;

    // 先扩充大小
    is = realloc(is, sizeof(struct intset) + (is->length + 1) * is->encoding);

    if (insertHead) {
        for (int i = is->length; i >= 0; --i) {
            uint64_t val = intsetGetVal(is, i - 1, oldEncoding);
            intsetSetValue(is, val, i);
        }
        intsetSetValue(is, value, 0);
    } else {
        for (int i = is->length - 1; i >= 0; --i) {
            uint64_t val = intsetGetVal(is, i, oldEncoding);
            intsetSetValue(is, val, i);
        }
        intsetSetValue(is, value, is->length);
    }

    is->length++;

    return is;
}

uint8_t intsetSearch(intset *is, int64_t value, int *pos) {

    if (is->length == 0) {
        return 0;
    }

    // intset有序
    if (value > intsetGetVal(is, is->length - 1, is->encoding)) {
        *pos = is->length;
        return 0;
    }

    if (value < intsetGetVal(is, 0, is->encoding)) {
        *pos = 0;
        return 0;
    }

    // 使用二分搜索,获取value的Index
    int max = is->length - 1, min = 0, mid;

    while (max >= min) {
        mid = (max + min) / 2;
        if (value > intsetGetVal(is, mid, is->encoding)) {
            min = mid + 1;
        } else if (value < intsetGetVal(is, mid, is->encoding)) {
            max = mid - 1;
        } else {
            *pos = mid;
            return 1;
        }
    }

    if (value < intsetGetVal(is, mid, is->encoding)) {
        *pos = mid;
    } else {
        *pos = mid + 1;
    }
    return 0;
}

uint32_t intsetLen(intset *is) {
    return is->length;
}

// 返回intset总占的字节大小
uint32_t intsetBlobLen(intset *is) {
    return sizeof(struct intset) + is->length * is->encoding;
}

uint8_t intsetFind(intset *is, int64_t value) {
    int pos;

    if (intsetSearch(is, value, &pos)) {
        return 1;
    }

    return 0;
}

int64_t intsetRandom(intset *is) {
    srand((unsigned) time(NULL));
    return intsetGetVal(is, rand() % is->length, is->encoding);
}

void move(intset *is, int pos) {
    void *dest, *src;
    unsigned int bytes;

    if (is->encoding == INTSET_ENC_INT16) {
        src = ((uint16_t *) is->contents) + pos;
        dest = ((uint16_t *) is->contents) + (pos + 1);
        bytes = (is->length - pos) * sizeof(uint16_t);
    } else if (is->encoding == INTSET_ENC_INT32) {
        src = ((uint32_t *) is->contents) + pos;
        dest = ((uint32_t *) is->contents) + (pos + 1);
        bytes = (is->length - pos) * sizeof(uint32_t);
    } else {
        src = ((uint64_t *) is->contents) + pos;
        dest = ((uint64_t *) is->contents) + (pos + 1);
        bytes = (is->length - pos) * sizeof(uint64_t);
    }

    memmove(dest, src, bytes);
}


intset *intsetAdd(intset *is, int64_t value, uint8_t *success) {
    // 获取value的实际Encoding
    uint8_t valueEncoding = _intsetValueEncoding(value);

    // 升级
    if (valueEncoding > is->encoding) {
        // upgradeAndSet
        uint32_t oldEncoding = is->encoding;
        is->encoding = valueEncoding;
        is = upgradeAndSet(is, oldEncoding, value);
        return is;
    }

    int pos = 0;

    // 如果值存在,那么添加失败
    // 如果值不存在,那么pos被赋值为应该存在的索引
    if (intsetSearch(is, value, &pos)) {
        return is;
    }

    is = realloc(is, sizeof(struct intset) + (is->length + 1) * is->encoding);

    // 新的元素在最后面
    if (pos < is->length) {

        // 把(pos-end)的元素全部往后挪
        move(is, pos);
    }

    intsetSetValue(is, value, pos);

    is->length++;

    return is;
}

uint32_t intsetFindIndex(intset *is, int64_t value) {
    uint32_t idx = -1;
    for (int i = 0; i < is->length; ++i) {
        if (intsetGetVal(is, i, is->encoding) == value) {
            idx = i;
        }
    }
    return idx;
}

intset *intsetRemove(intset *is, int64_t value, int *success) {

    uint32_t idx = intsetFindIndex(is, value);

    if (idx == -1) {
        if (success != NULL) {
            *success = 0;
        }

        return is;
    }

    // 如果删除的不是最后一个元素,则需挪动元素
    if (idx != is->length - 1) {
        // 通过挪动位置来删除元素,idx + 1的所有元素往后挪一个元素
        intsetMoveTail(is, idx);
    }

    // 缩小intset
    is->length--;
    is = realloc(is, sizeof(struct intset) + is->length * is->encoding);
    if (success != NULL) {
        *success = 1;
    }
    return is;
}

int main() {

    intset *is = intsetNew();

    int success;
    is = intsetAdd(is, 10, &success);
    is = intsetAdd(is, 5, &success);
    is = intsetAdd(is, 15, &success);
    is = intsetAdd(is, 20, &success);
    is = intsetAdd(is, 17, &success);
    is = intsetAdd(is, 16, &success);
    is = intsetAdd(is, 65536, &success);
    is = intsetAdd(is, 19, &success);
    is = intsetAdd(is, 9999999999999, &success);
    is = intsetAdd(is, 3, &success);
    is = intsetRemove(is, 17, &success);
    is = intsetRemove(is, 5, &success);
    is = intsetRemove(is, 13, &success);

    printIsContent(is);

    return 0;
}
