#include "ziplist.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stdint.h>
#include "util.h"

unsigned char *ziplistNew(void) {
    unsigned int bytes = ZIPLIST_HEADER_SIZE + 1;
    unsigned char *zl = malloc(bytes);
    memset(zl, 0x00, bytes);
    ZIPLIST_BYTES(zl) = bytes;
    ZIPLIST_TAIL_OFFSET(zl) = ZIPLIST_HEADER_SIZE;
    ZIPLIST_LENGTH(zl) = 0;
    zl[bytes - 1] = ZIP_END;
    return zl;
}

int zipTryEncoding(unsigned char *s, unsigned int slen, long long *value, unsigned char **encoding) {
    long long numberValue = 0;

    *encoding = malloc(sizeof(unsigned char *));

    if (string2ll(s, slen, &numberValue)) {
        if (numberValue >= 0 && numberValue <= 12) {
            **encoding = ZIP_INT_IMM_MIN + numberValue;
        } else if (numberValue >= INT8_MIN && numberValue <= INT8_MAX) {
            **encoding = ZIP_INT_8B;
        } else if (numberValue >= INT16_MIN && numberValue <= INT16_MAX) {
            **encoding = ZIP_INT_16B;
        } else if (numberValue >= INT32_MIN && numberValue <= INT32_MAX) {
            **encoding = ZIP_INT_32B;
        } else {
            **encoding = ZIP_INT_64B;
        }
        *value = numberValue;
        return 1;
    }

    return 0;
}

// 对字节数组进行编码,返回encoding的长度
// encoding将被追加到content的后面
unsigned char *zipEncoding(unsigned char *zl, unsigned int slen, uint8_t *encodingLen) {
    unsigned char *buf;

    if (zl) {
        buf = malloc(5);
        memset(buf, 0x00, 5);
    }

    if (slen <= 63) {
        *encodingLen = 1;
        if (!zl) {
            return NULL;
        }
        buf[0] = 0b00 | slen;
    } else if (slen <= 16383) {
        *encodingLen = 2;
        if (!zl) {
            return NULL;
        }
        buf[0] = 0b01000000 | (slen >> 8);
        buf[1] = slen << 8;
        // else if (slen <= 4294967295)
    } else {
        *encodingLen = 5;
        if (!zl) {
            return NULL;
        }
        buf[0] = 0b10000000;
        buf[1] = slen << 24;
        buf[2] = slen << 16;
        buf[3] = slen << 8;
        buf[4] = slen;
    }

    return buf;
}

static unsigned int zipIntSize(unsigned char encoding) {
    if (encoding == ZIP_INT_8B) {
        return 1;
    }
    if (encoding == ZIP_INT_16B) {
        return 2;
    }
    if (encoding == ZIP_INT_24B) {
        return 3;
    }
    if (encoding == ZIP_INT_32B) {
        return 4;
    }
    if (encoding == ZIP_INT_64B) {
        return 8;
    }
    return 0;
}


// return size
static unsigned int decodePreviousLen(unsigned char *zl, unsigned int *previousSize) {
    if (zl[0] < 254) {
        *previousSize = zl[0];
        return 1;
    } else {
        memcpy(previousSize, zl, 5);
        return 5;
    }
}

static unsigned int zipStrSize(unsigned char *zl, unsigned int *encodingLen) {
    unsigned char encoding0 = zl[0];

    if ((encoding0 >> 6) == 0b00) {
        *encodingLen = 1;
        return encoding0;
    } else if ((encoding0 >> 6) == 0b01) {
        unsigned char encoding1 = zl[1];
        *encodingLen = 2;
        return ((encoding0 & 0b00111111) << 8) | encoding1;
        // (encoding0 >> 6) == 0b10
    } else {
        unsigned char encoding1 = zl[1];
        unsigned char encoding2 = zl[2];
        unsigned char encoding3 = zl[3];
        unsigned char encoding4 = zl[4];
        *encodingLen = 5;
        return (encoding1 << 24) | (encoding2 << 16) || (encoding3 << 8) || encoding4;
    }
}

struct zlentry *zipEntry(unsigned char *zl) {
    // 1. 读取previousLength
    struct zlentry *entry = malloc(sizeof(*entry));

    entry->previousLen = decodePreviousLen(zl, &entry->previousSize);

    unsigned int len = 0;

    len += entry->previousLen;

    // 2. 读取encoding

    // 数字编码
    if (zl[len] >> 6 == 0b11) {
        unsigned char encoding = zl[len];
        entry->lenSize = 1;
        entry->len = zipIntSize(encoding);
        // 字符串编码
    } else {
        entry->len = zipStrSize(zl + len, &entry->lenSize);
    }

    entry->encoding = zl[len];
    entry->p = zl;

    return entry;
}

unsigned char *ziplistPush(unsigned char *zl, unsigned char *s, unsigned int slen, int where) {
    // 1. 当前节点是否是最后一个节点?
    unsigned char *startPtr = zl + ZIPLIST_TAIL_OFFSET(zl);

    _Bool isLastNode = *startPtr == ZIP_END;

    // 2. 计算previousLen
    unsigned int previousLen = 0;
    unsigned int previousSize = 0;
    if (!isLastNode) {
        struct zlentry *previousEntry = zipEntry(startPtr);
        previousLen = previousEntry->len;
        previousSize = previousEntry->lenSize;
    } else {
        previousLen = 1;
        previousSize = 0;
    }

    // 3. encoding
    long long numberValue = 0;
    unsigned char *encoding = NULL;
    uint8_t encodingLen = 0;

    _Bool isNumber = 0;

    // 3.1 尝试转换为数字
    if (zipTryEncoding(s, slen, &numberValue, &encoding)) {
        encodingLen = 1;
        isNumber = 1;
        slen = zipIntSize(*encoding);
    } else {
        // 3.2 encodingLen作为输出参数传入
        encoding = zipEncoding(startPtr, slen, &encodingLen);
    }

    unsigned int bytes = previousLen + encodingLen + slen;

    // 4. realloc
    // ps : +1 = ZIP_EOF

    printf("%d \n", *(zl + ZIPLIST_HEADER_SIZE));

    zl = realloc(zl, ZIPLIST_BYTES(zl) + bytes + 1);

    startPtr = zl + ZIPLIST_TAIL_OFFSET(zl);

    // 5. 赋值
    memcpy(startPtr, &previousSize, previousLen);
    startPtr += previousLen;

    memcpy(startPtr, encoding, encodingLen);

    startPtr += encodingLen;

    if (isNumber) {
        memcpy(startPtr, &numberValue, slen);
    } else {
        memcpy(startPtr, s, slen);
    }

    startPtr += slen;

    // 6. 维护zl的属性
    ZIPLIST_BYTES(zl) = ZIPLIST_BYTES(zl) + bytes;
    ZIPLIST_TAIL_OFFSET(zl) = ZIPLIST_TAIL_OFFSET(zl);
    ZIPLIST_LENGTH(zl) = ZIPLIST_LENGTH(zl) + 1;
    *startPtr = ZIP_END;

    return zl;
}

unsigned char *ziplistIndex(unsigned char *zl, int index) {
    // todo 支持负数

    zl = zl + ZIPLIST_HEADER_SIZE;

    int idx = 0;
    while (*zl != ZIP_END) {
        struct zlentry *entry = zipEntry(zl);
        if (idx == index) {
            return entry->p;
        }
        zl += entry->lenSize;
    }

    return NULL;
}


int main() {
    unsigned char *zl = ziplistNew();
    zl = ziplistPush(zl, "zzzj", 4, 0);
    zl = ziplistPush(zl, "hahahah", 7, 0);
    zl = ziplistPush(zl, "1233", 4, 0);
    zl = ziplistPush(zl, "abc", 3, 0);

    printf("%d \n", ziplistIndex(zl, 0));
}