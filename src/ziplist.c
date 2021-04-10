#include "stdio.h"
#include "ziplist.h"
#include "stdlib.h"
#include "string.h"
#include <stdint.h>
#include "util.h"

void zipSetPrevious(unsigned char *p, unsigned int previousSize, unsigned int previousLen) {
    if (previousLen == 1) {
        *p = previousSize;
    } else {
        memset(p + 1, 0x00, 4);
        *p = ZIP_BIGLEN;
        // assert previousLen == 5
        memcpy(p + 1, &previousSize, 4);
    }
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
        buf[0] = 0b00000000 | slen;
    } else if (slen <= 16383) {
        *encodingLen = 2;
        if (!zl) {
            return NULL;
        }
        buf[0] = 0b01000000 | ((slen >> 8) & 0x3f);
        buf[1] = slen & 0xff;
        // else if (slen <= 4294967295)
    } else {
        *encodingLen = 5;
        if (!zl) {
            return NULL;
        }
        buf[0] = 0b10000000;
        buf[1] = (slen >> 24) & 0xff;
        buf[2] = (slen >> 16) & 0xff;
        buf[3] = (slen >> 8) & 0xff;
        buf[4] = slen & 0xff;
    }

    return buf;
}

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
        } else if (numberValue >= INT24_MIN && numberValue <= INT24_MAX) {
            **encoding = ZIP_INT_24B;
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

static void zipCopyInt(unsigned char *ptr, long long value, unsigned int sLen) {
    uint8_t value8;
    uint16_t value16;
    uint32_t value32;
    uint64_t value64;

    if (sLen == 0) {
        return;
    } else if (sLen == 1) {
        value8 = value;
        memcpy(ptr, &value8, sizeof(value8));
    } else if (sLen == 2) {
        value16 = value;
        memcpy(ptr, &value16, sizeof(value16));
        memrev16ifbe(ptr)
    } else if (sLen == 3) {
        value32 = value << 8;
        // 如果是大端字节序则转换为小端字节序
        // 小端字节序 : 高位在后,低位在前

        // 1233(big)    : 010011010001
        // 1233(litter) : 110100010100 << 8 = 00000000110100010100
        memrev32ifbe(&value32)
        memcpy(ptr, ((uint8_t *) &value32) + 1, sizeof(uint32_t) - sizeof(uint16_t));
    } else if (sLen == 4) {
        value32 = value;
        memcpy(ptr, &value32, sizeof(value32));
        memrev32ifbe(ptr)
    } else if (sLen == 8) {
        value64 = value;
        memcpy(ptr, &value64, sizeof(value64));
        memrev64ifbe(ptr)
    }

}

static uint64_t zipLoadInt(unsigned char *ptr, unsigned char encoding) {
    uint8_t value8;
    uint16_t value16;
    uint32_t value32;
    uint64_t value64;

    if (encoding == ZIP_INT_8B) {
        memcpy(&value8, ptr, sizeof(uint8_t));
        return value8;
    }

    if (encoding == ZIP_INT_16B) {
        memcpy(&value16, ptr, sizeof(uint16_t));
        memrev16ifbe(&value16);
        return value16;
    }

    if (encoding == ZIP_INT_24B) {
        memcpy(((uint8_t *) &value32) + 1, ptr, sizeof(uint32_t) - sizeof(uint8_t));
        memrev32ifbe(&value32);
        return value32 >> 8;
    }

    if (encoding == ZIP_INT_32B) {
        memcpy(&value32, ptr, sizeof(uint32_t));
        memrev32ifbe(&value32);
        return value32;
    }

    if (encoding == ZIP_INT_64B) {
        memcpy(&value64, ptr, sizeof(uint64_t));
        memrev64ifbe(&value64);
        return value64;
    }

    // error
    exit(0);
}

// return size
static unsigned int decodePreviousLen(unsigned char *zl, unsigned int *previousSize) {
    if (zl[0] < ZIP_BIGLEN) {
        *previousSize = zl[0];
        return 1;
    } else {
        memcpy(previousSize, zl + 1, 4);
        return 5;
    }
}

static unsigned int zipStrSize(unsigned char *zl, unsigned int *encodingLen) {
    unsigned char encoding0 = zl[0];

    if ((encoding0 >> 6) == 0b00) {
        *encodingLen = 1;
        return encoding0;
    } else if ((encoding0 >> 6) == 0b01) {
        *encodingLen = 2;
        unsigned char encoding1 = zl[1];
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
        entry->encodingLen = 1;
        entry->contentLen = zipIntSize(encoding);
        // 字符串编码
    } else {
        entry->contentLen = zipStrSize(zl + len, &entry->encodingLen);
    }

    entry->encoding = zl[len];
    entry->fullEncoding = malloc(entry->encodingLen);
    memcpy(entry->fullEncoding, zl + len, entry->encodingLen);
    entry->p = zl;
    entry->totalSize = entry->previousLen + entry->encodingLen + entry->contentLen;

    return entry;
}

void printZl(unsigned char *zl) {
    unsigned int offset = ZIPLIST_HEADER_SIZE;

    int skip = -1;
    int skipLength = 0;

    for (int i = 0; i < ZIPLIST_BYTES(zl); ++i) {
        if (i == offset) {
            printf("] ");
            struct zlentry *entry = zipEntry(zl + offset);
            printf(" [ ");
            offset += entry->totalSize;
            if (entry->totalSize > 20) {
                // 跳过 entry->totalSize - 15个元素
                skip = i + 14;
                skipLength = entry->totalSize - 14;
            }
        } else if (i == skip) {
            i += skipLength - 1;
            skip = -1;
            printf("...");
        }
        printf("%d ", *(zl + i));
    }

    printf("\n");
}

unsigned char *ziplistPush(unsigned char *zl, unsigned char *s, unsigned int slen, int where) {
    // 1. 当前节点是否是最后一个节点?
    unsigned char *startPtr = zl + ZIPLIST_TAIL_OFFSET(zl);

    _Bool isLastNode = *startPtr == ZIP_END;

    // 2. 计算previousLen
    unsigned int previousLen = 0;
    unsigned int previousSize = 0;
    struct zlentry *previousEntry = NULL;
    if (!isLastNode) {
        previousEntry = zipEntry(startPtr);
        previousLen = previousEntry->totalSize < ZIP_BIGLEN ? 1 : 5;
        previousSize = previousEntry->previousLen + previousEntry->encodingLen + previousEntry->contentLen;
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

    zl = realloc(zl, ZIPLIST_BYTES(zl) + bytes + 1);

    startPtr = zl + ZIPLIST_TAIL_OFFSET(zl) + (isLastNode ? 0 : previousSize);

    // 5. 赋值
    zipSetPrevious(startPtr, previousSize, previousLen);
    startPtr += previousLen;

    memcpy(startPtr, encoding, encodingLen);

    startPtr += encodingLen;

    if (isNumber) {
        zipCopyInt(startPtr, numberValue, slen);
    } else {
        memcpy(startPtr, s, slen);
    }

    startPtr += slen;

    // 6. 维护zl的属性
    ZIPLIST_BYTES(zl) = ZIPLIST_BYTES(zl) + bytes;
    ZIPLIST_TAIL_OFFSET(zl) = ZIPLIST_TAIL_OFFSET(zl) + previousSize;
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
        idx++;
        zl += entry->contentLen + entry->previousLen + entry->encodingLen;
    }

    return NULL;
}

unsigned int ziplistGet(unsigned char *p, unsigned char **sval, unsigned int *slen, long long *lval) {
    if (p == NULL || p[0] == ZIP_END) {
        return 0;
    }

    struct zlentry *entry = zipEntry(p);

    // 是数字
    if ((entry->encoding >> 6) == 0b11) {
        *lval = zipLoadInt(entry->p, entry->encoding);
        // 字符串
    } else {
        if ((entry->encoding >> 6) == 0b00) {
            *slen = entry->encoding & 0b00111111;
        } else if ((entry->encoding >> 6) == 0b01) {
            *slen = ((entry->fullEncoding[0] & 0b00111111) << 8) | entry->fullEncoding[1];
        } else {
            *slen = entry->fullEncoding[1] << 24 | entry->fullEncoding[2] << 16 | entry->fullEncoding[3] << 8 | entry->fullEncoding[4];
        }
        *sval = malloc(*slen);
        memcpy(*sval, p + entry->previousLen + entry->encodingLen, *slen);
    };

    return 1;
}

unsigned char *readCurrentFile(unsigned long *slen) {
    FILE *fp = fopen(__FILE__, "rb");

    fseek(fp, 0L, SEEK_END);

    long len = ftell(fp);
    *slen = len;

    // 回到文件首地址
    rewind(fp);

    // +1 = \0
    unsigned char *content = malloc(len + 1);
    memset(content, 0x00, len + 1);

    fread(content, 1, len, fp);

    content[len] = '\0';

    fclose(fp);

    return content;
}

unsigned char *ziplistNext(unsigned char *zl, unsigned char *p) {
    if (zl == NULL || p == NULL || p[0] == ZIP_END) {
        return NULL;
    }

    struct zlentry *current = zipEntry(p);

    p += current->previousLen + current->encodingLen + current->contentLen;

    if (p[0] == ZIP_END) {
        return NULL;
    }

    return p;
}

unsigned char *ziplistPrev(unsigned char *zl, unsigned char *p) {
    if (zl == NULL || p == NULL) {
        return NULL;
    }
    struct zlentry *current = zipEntry(p);

    if (current->previousSize == 0) {
        return NULL;
    }

    return p - current->previousSize;
}

void entryRepr(unsigned char *zl, struct zlentry *entry) {
    printf("offset = %d , previousLen = %d , previousSize = %d , encodingLen = %d , contentLen = %d , totalSize = %d", entry->p - zl, entry->previousLen, entry->previousSize,
           entry->encodingLen, entry->contentLen, entry->totalSize);
}

void ziplistRepr(unsigned char *zl) {
    printf("zl, bytes = %d , length = %d , tailOffset = %d \n", ZIPLIST_BYTES(zl), ZIPLIST_LENGTH(zl), ZIPLIST_TAIL_OFFSET(zl));
    unsigned char *ptr = zl + ZIPLIST_HEADER_SIZE;
    for (int i = 0; i < ZIPLIST_LENGTH(zl); ++i) {
        struct zlentry *entry = zipEntry(ptr);
        printf("index = %d , ", i);
        entryRepr(zl, entry);
        printf("\n");
        ptr += entry->totalSize;
    }
}

unsigned char *cascadeUpdate(unsigned char *zl, unsigned char *p) {
    if (p == NULL || *p == ZIP_END) {
        return zl;
    }

    struct zlentry *entry = zipEntry(p);

    if (*(entry->p + entry->totalSize) == ZIP_END) {
        return zl;
    }

    unsigned char *nextP = entry->p + entry->totalSize;

    struct zlentry *nextEntry = zipEntry(nextP);

    uint8_t previousLen = entry->totalSize < ZIP_BIGLEN ? 1 : 5;

    if (nextEntry->previousLen == previousLen) {
        // 更新nextEntry的previousSize
        zipSetPrevious(nextEntry->p, entry->totalSize, previousLen);
        return zl;
    }

    unsigned int offset = nextEntry->p - zl;

    // 1. 缩小的情况不做处理,浪费3个字节也无妨
    if (previousLen == 1) {
        zipSetPrevious(nextEntry->p, entry->totalSize, 5);
        return zl;
    } else {
        ZIPLIST_BYTES(zl) = ZIPLIST_BYTES(zl) + 4;
        zl = realloc(zl, ZIPLIST_BYTES(zl));
    }
    // 23 0 0 0 16 0 0 0 2 0 [0 4 122 122 122 106] [6 4 98 105 104 97] 255

    // 2. 挪动元素
    // nextEntryPtr = nextEntry.p
    unsigned char *nextEntryPtr = zl + offset;

    nextEntry = zipEntry(nextEntryPtr);

    // 重新复制previousSize
    memmove(nextEntryPtr + 5, nextEntryPtr + 1, ZIPLIST_BYTES(zl) - (nextEntryPtr - zl));

    zipSetPrevious(nextEntryPtr, entry->totalSize, 5);

    nextEntry = zipEntry(nextEntryPtr);

    if (*(nextEntry->p + nextEntry->totalSize) == ZIP_END) {
        return zl;
    }

    // 每一个previousLen从1个字节升级到5个字节,就需要维护tailOffset
    ZIPLIST_TAIL_OFFSET(zl) = ZIPLIST_TAIL_OFFSET(zl) + 4;

    return cascadeUpdate(zl, nextEntryPtr);
}

unsigned char *ziplistInsert(unsigned char *zl, unsigned char *p, unsigned char *s, unsigned int slen) {
    if (p == NULL) {
        return ziplistPush(zl, s, slen, ZIPLIST_TAIL);
    }

    struct zlentry *current = zipEntry(p);

    unsigned char *nextPtr = ziplistNext(zl, p);

    if (nextPtr == NULL) {
        return ziplistPush(zl, s, slen, ZIPLIST_TAIL);
    }

    struct zlentry *next = zipEntry(nextPtr);

    if (next == NULL) {
        return ziplistPush(zl, s, slen, ZIPLIST_TAIL);
    }

    unsigned int previousSize = current->totalSize;
    unsigned int previousLen = previousSize < ZIP_BIGLEN ? 1 : 5;

    unsigned char *startPtr = p + previousSize;

    long long numberValue = 0;
    unsigned char *encoding = NULL;
    uint8_t encodingLen = 0;

    if (zipTryEncoding(s, slen, &numberValue, &encoding)) {
        encodingLen = 1;
        slen = zipIntSize(*encoding);
    } else {
        // 3.2 encodingLen作为输出参数传入
        encoding = zipEncoding(startPtr, slen, &encodingLen);
    }

    unsigned int bytes = previousLen + encodingLen + slen;

    // 1. 申请空间
    unsigned int offset = startPtr - zl;
    unsigned int newSize = ZIPLIST_BYTES(zl) + bytes;

    zl = realloc(zl, newSize);

    startPtr = zl + offset;

    memmove(startPtr + bytes, startPtr, ZIPLIST_BYTES(zl) - (startPtr - zl));

    // 2. 在当前元素的后面插入
    zipSetPrevious(startPtr, previousSize, previousLen);

    memcpy(startPtr + previousLen, encoding, encodingLen);

    if (numberValue) {
        memcpy(startPtr + encodingLen + previousLen, &numberValue, slen);
    } else {
        memcpy(startPtr + encodingLen + previousLen, &s, slen);
    }


    // 5. 更新tailOffset
    ZIPLIST_BYTES(zl) = newSize;
    ZIPLIST_LENGTH(zl) = ZIPLIST_LENGTH(zl) + 1;
    ZIPLIST_TAIL_OFFSET(zl) = ZIPLIST_TAIL_OFFSET(zl) + zipEntry(startPtr)->totalSize;

    // 4. 级联更新
    zl = cascadeUpdate(zl, startPtr);


    return zl;
}


void printEntry(struct zlentry *entry) {
    printf("previousLen = %d ,  previousSize = %d , encodingLen = %d , contentLen = %d , totalSize = %d \n", entry->previousLen, entry->previousSize, entry->encodingLen,
           entry->contentLen, entry->totalSize);
}

void printEntryPtr(unsigned char *p) {
    struct zlentry *entry = zipEntry(p);
    printEntry(entry);
    free(entry);
}

void test1() {
    // 1. push
    // 11 0 0 0 10 0 0 0 0 0 255
    unsigned char *zl = ziplistNew();
    // 17 0 0 0 10 0 0 0 1 0 [0 4 122 122 122 106] 255
    zl = ziplistPush(zl, "zzzj", 4, 0);
    // 26 0 0 0 16 0 0 0 2 0 [0 4 122 122 122 106] 6 7 104 97 104 97 104 97 104 255
    zl = ziplistPush(zl, "hahahah", 7, 0);
    // 30 0 0 0 25 0 0 0 3 0 [0 4 122 122 122 106] [6 7 104 97 104 97 104 97 104] [9 192 209 4] 255
    zl = ziplistPush(zl, "1233", 4, 0);
    // 35 0 0 0 29 0 0 0 4 0 [0 4 122 122 122 106] [6 7 104 97 104 97 104 97 104] [9 192 209 4] [4 208 197 225 240] 255
    zl = ziplistPush(zl, "123333", 6, 0);
    // 保存一个大字符串
    unsigned long fileStrLen = 0;
    unsigned char *fileContent = readCurrentFile(&fileStrLen);
    zl = ziplistPush(zl, fileContent, fileStrLen, 0);

    // 2. get
    unsigned char *p = ziplistIndex(zl, 2);

    unsigned char *sVal = NULL;
    unsigned int slen = 0;
    long long lVal = 0;

    printf("======================== next ======================== \n");

    // 3. next
    printEntryPtr(p);
    unsigned char *next = ziplistNext(zl, p);
    printEntryPtr(next);
    unsigned char *nextNext = ziplistNext(zl, next);
    printEntryPtr(nextNext);

    printf("======================== previous ======================== \n");

    // 4. previous
    unsigned char *previous = ziplistPrev(zl, nextNext);
    printEntryPtr(previous);
    unsigned char *previousPrev = ziplistPrev(zl, previous);
    printEntryPtr(previousPrev);
}


unsigned int ziplistLen(unsigned char *zl) {
    return ZIPLIST_LENGTH(zl);
}

size_t ziplistBlobLen(unsigned char *zl) {
    return ZIPLIST_BYTES(zl);
}

unsigned char *_zlRealloc(unsigned char *zl, unsigned int newSize) {
    zl = realloc(zl, newSize);
    zl[newSize - 1] = ZIP_END;
    ZIPLIST_BYTES(zl) = newSize;
    return zl;
}

unsigned char *_ziplistDelete(unsigned char *zl, unsigned char *p, unsigned int deleteNumber) {
    if (zl == NULL || p[0] == ZIP_END) {
        return NULL;
    }

    unsigned char *endP = p;

    // 真正应该删除的数量
    int deleteCount = 0;

    while (*endP != ZIP_END && deleteCount < deleteNumber) {
        struct zlentry *entry = zipEntry(endP);
        endP += entry->totalSize;
        deleteCount += 1;
    }

//    zl, bytes = 300 , length = 5 , tailOffset = 293
//    index = 0 , offset = 10 , previousLen = 1 , previousSize = 0 , encodingLen = 1 , contentLen = 4 , totalSize = 6
//    index = 1 , offset = 16 , previousLen = 1 , previousSize = 6 , encodingLen = 1 , contentLen = 4 , totalSize = 6
//    index = 2 , offset = 22 , previousLen = 1 , previousSize = 6 , encodingLen = 2 , contentLen = 256 , totalSize = 259
//    index = 3 , offset = 281 , previousLen = 5 , previousSize = 259 , encodingLen = 1 , contentLen = 6 , totalSize = 12
//    index = 4 , offset = 293 , previousLen = 1 , previousSize = 12 , encodingLen = 1 , contentLen = 4 , totalSize = 6

    if (!deleteCount) {
        return zl;
    }

    unsigned int newSize = ZIPLIST_BYTES(zl) - (endP - p);
    // 更新tailOffset
    ZIPLIST_TAIL_OFFSET(zl) = ZIPLIST_TAIL_OFFSET(zl) - (endP - p);

    // 后面没有元素了
    if (*endP == ZIP_END) {
        zl = _zlRealloc(zl, newSize);
    } else {
        // 后面还有元素,可能会触发级联更新

        // 为级联更新做准备
        struct zlentry *previousEntry = zipEntry(p - zipEntry(p)->previousSize);
        unsigned char *previous = previousEntry->p;
        unsigned int offset = previous - zl;

        // 1. 先挪动元素
        memmove(p, endP, ZIPLIST_BYTES(zl) - (endP - zl) - 1);

        // 2. realloc
        zl = _zlRealloc(zl, newSize);

        previous = zl + offset;

        // 3. 级联更新
        zl = cascadeUpdate(zl, previous);
    }

    ZIPLIST_LENGTH(zl) = ZIPLIST_LENGTH(zl) - deleteCount;
    return zl;
}

unsigned char *ziplistDelete(unsigned char *zl, unsigned char **p) {
    unsigned int offset = *p - zl;
    zl = _ziplistDelete(zl, *p, 1);
    *p = zl + offset;
    return zl;
}

unsigned char *ziplistDeleteRange(unsigned char *zl, unsigned int index, unsigned int num) {
    unsigned char *p = ziplistIndex(zl, index);
    if (!p) {
        return zl;
    }
    return _ziplistDelete(zl, p, num);
}

_Bool zipEntryIsStr(struct zlentry *entry) {
    return entry->encoding >> 6 != 0b11;
}

unsigned char *ziplistFind(unsigned char *p, unsigned char *vstr, unsigned int vlen, unsigned int skip) {
    if (p == NULL || p[0] == ZIP_END) {
        return NULL;
    }

    int skipCount = 0;
    while (p[0] != ZIP_END) {
        if (skipCount == 0) {
            struct zlentry *entry = zipEntry(p);

            if (zipEntryIsStr(entry)) {
                if (entry->contentLen == vlen && memcmp(entry->p + entry->previousLen + entry->encodingLen, vstr, vlen)) {
                    return entry->p;
                }
            } else {

            }

            skipCount = skip;
        } else {
            skipCount--;
        }
    }
}

unsigned int ziplistCompare(unsigned char *p, unsigned char *s, unsigned int slen) {
    if (p == NULL || p[0] == ZIP_END) {
        return 0;
    }
    struct zlentry *entry = zipEntry(p);

    if (zipEntryIsStr(entry)) {
        return entry->contentLen == slen && memcmp(entry->p + entry->previousLen + entry->encodingLen, s, slen) == 0;
    } else {
        long long numberValue = 0;
        unsigned char *encoding = NULL;
        if (zipTryEncoding(s, slen, &numberValue, &encoding)) {
            return zipLoadInt(entry->p + entry->previousLen + entry->encodingLen, *encoding) == numberValue;
        }
        return 0;
    }
}

void testInsert() {
    unsigned char *zl = ziplistNew();
    zl = ziplistPush(zl, "zzzj", 4, 0);
    zl = ziplistPush(zl, "biha", 4, 0);
    zl = ziplistPush(zl, "big data", 256, 0);
    zl = ziplistPush(zl, "heihei", 6, 0);
    unsigned char *bigData = ziplistIndex(zl, 2);
    ziplistRepr(zl);
    zl = ziplistInsert(zl, bigData, "1233", 4);
    ziplistRepr(zl);
}


void testDelete() {
    unsigned char *zl = ziplistNew();
    zl = ziplistPush(zl, "zzzj", 4, 0);
    zl = ziplistPush(zl, "biha", 4, 0);
    zl = ziplistPush(zl, "big data", 256, 0);
    zl = ziplistPush(zl, "heihei", 6, 0);
    zl = ziplistPush(zl, "last", 4, 0);
    unsigned char *p = ziplistIndex(zl, 1);
    ziplistRepr(zl);
    zl = ziplistDelete(zl, &p);
    ziplistRepr(zl);

    // entryRepr(zl, zipEntry(p));
}

void testDeleteRange() {
    unsigned char *zl = ziplistNew();
    zl = ziplistPush(zl, "zzzj", 4, 0);
    zl = ziplistPush(zl, "biha", 4, 0);
    zl = ziplistPush(zl, "big data", 256, 0);
    zl = ziplistPush(zl, "heihei", 6, 0);
    zl = ziplistPush(zl, "last", 4, 0);
    ziplistRepr(zl);
    zl = ziplistDeleteRange(zl, 1, 2);
    ziplistRepr(zl);
}

void testInsertCascadeUpdate() {
    unsigned char *zl = ziplistNew();
    zl = ziplistPush(zl, "zzzj", 4, 0);
    zl = ziplistPush(zl, "1233", 4, 0);
    // pLen = 1, encodingLen = 2, sLen = 250 , totalLen = 253
    zl = ziplistPush(zl, "big data", 250, 0);
    zl = ziplistPush(zl, "last", 4, 0);

    // 在bigData前插入一个大于254字节的元素，导致bigData的previousLen = 5，从而导致bigData的totalLen > 254 , 导致last的previousLen变成五
    unsigned char *ptr1233 = ziplistIndex(zl, 1);

    ziplistRepr(zl);

    zl = ziplistInsert(zl, ptr1233, "just for insert", 255);

    ziplistRepr(zl);
}

int main() {
    // test1();
    // testInsert();
    // testInsertCascadeUpdate();
    // testDelete();
    // testDeleteRange();
}