#include "intset.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

intset *intsetNew(void) {
    intset *is = calloc(1, sizeof(*is));
    is->encoding = INTSET_ENC_INT16;
    return is;
}

uint64_t _getIdxVal(intset *is, int idx) {
    if (is->encoding == INTSET_ENC_INT16) {
        return ((uint16_t *) is->contents)[idx];
    } else if (is->encoding == INTSET_ENC_INT32) {
        return ((uint32_t *) is->contents)[idx];
    } else {
        return ((uint64_t *) is->contents)[idx];
    }
}

uint8_t intsetSearch(intset *is, int64_t value, int *pos) {

    if (is->length == 0) {
        return 0;
    }

    uint8_t valueEncoding = _intsetValueEncoding(value);

    if (valueEncoding != is->encoding) {
        return 0;
    }

    // intset有序
    if (value > _getIdxVal(is, is->length - 1)) {
        *pos = is->length;
        return 0;
    }

    if (value < _getIdxVal(is, 0)) {
        *pos = 0;
        return 0;
    }

    // 使用二分搜索,获取value的Index
    int max = is->length - 1, min = 0, mid;

    while (max >= min) {
        mid = (max + min) / 2;
        if (value > _getIdxVal(is, mid)) {
            min = mid + 1;
        } else if (value < _getIdxVal(is, mid)) {
            max = mid - 1;
        } else {
            *pos = mid;
            return 1;
        }
    }

    *pos = mid;
    return 0;
}

uint8_t intsetFind(intset *is, int64_t value) {
    int pos;

    if (intsetSearch(is, value, &pos)) {
        return 1;
    }

    return 0;
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

void intsetSetValue(intset *is, int64_t value, int idx) {
    if (is->encoding == INTSET_ENC_INT16) {
        ((uint16_t *) is->contents)[idx] = value;
    } else if (is->encoding == INTSET_ENC_INT32) {
        ((uint32_t *) is->contents)[idx] = value;
    } else {
        ((uint64_t *) is->contents)[idx] = value;
    }
}

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

intset *intsetAdd(intset *is, int64_t value, uint8_t *success) {
    // 获取value的实际Encoding
    uint8_t valueEncoding = _intsetValueEncoding(value);

    // 升级
    if (valueEncoding > is->encoding) {
        // upgrade
        is->encoding = valueEncoding;
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


int main() {

    intset *is = intsetNew();

    int success;
    intsetAdd(is, 10, &success);
    intsetAdd(is, 5, &success);
    intsetAdd(is, 15, &success);
    intsetAdd(is, 20, &success);
    intsetAdd(is, 17, &success);
    intsetAdd(is, 16, &success);
    intsetAdd(is, 19, &success);

    printIsContent(is);

    return 0;
}
