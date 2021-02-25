#include "intset.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

intset *intsetNew(void) {
    intset *is = calloc(1, sizeof(*is));
    is->encoding = INTSET_ENC_INT16;
    return is;
}

uint8_t intsetSearch(intset *is, int64_t value, int *pos) {
    uint8_t valueEncoding = _intsetValueEncoding(value);
    if (valueEncoding != is->encoding) {
        return 0;
    }

    if (value > is->contents[is->length - 1]) {
        return 0;
    }

    if (value < is->contents[0]) {
        return 0;
    }

    // 使用二分搜索,获取value的Index
    int max = is->length - 1, min = 0, mid = is->length / 2;

    while (max >= mid) {
        if (value > is->contents[mid]) {
            min = mid + 1;
            mid = (max + min) / 2;
        } else if (value < is->contents[mid]) {
            max = mid - 1;
            mid = (max + min) / 2;
        } else {
            *pos = mid;
            return 1;
        }
    }

    *pos = min;
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
        dest = ((uint16_t *) is->contents) + pos;
        src = ((uint16_t *) is->contents) + (pos + 1);
        bytes = is->length - pos * sizeof(uint16_t);
    } else if (is->encoding == INTSET_ENC_INT32) {
        dest = ((uint32_t *) is->contents) + pos;
        src = ((uint32_t *) is->contents) + (pos + 1);
        bytes = is->length - pos * sizeof(uint32_t);
    } else {
        dest = ((uint64_t *) is->contents) + pos;
        src = ((uint64_t *) is->contents) + (pos + 1);
        bytes = is->length - pos * sizeof(uint64_t);
    }
    memmove(dest, src, bytes);
}

intset *intsetAdd(intset *is, int64_t value, uint8_t *success) {
    // 获取value的实际Encoding
    uint8_t valueEncoding = _intsetValueEncoding(value);

    // 升级
    if (valueEncoding > is->encoding) {
        // upgrade
        is->encoding = valueEncoding;
    }

    int pos;

    // 如果值存在,那么添加失败
    if (intsetSearch(is, value, &pos)) {
        return is;
    }

    is = realloc(is, sizeof(struct intset) + (is->length + 1) * is->encoding);

    move(is, pos);

    is->length++;

    return is;
}


int main() {

    int value = 12;

    int arr[10] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 15};

    int max = 9, min = 0, mid = (max - min) / 2;

    while (max >= min) {
        if (value > arr[mid]) {
            min = mid + 1;
            mid = (max + min) / 2;
        } else if (value < arr[mid]) {
            max = mid - 1;
            mid = (max + min) / 2;
        } else {
            break;
        }
    }

    printf("max = %d , min = %d , mid = %d \n", max, min, mid);

    return 0;
}
