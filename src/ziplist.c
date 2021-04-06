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
        buf[1] = (slen << 24) & 0xff;
        buf[2] = (slen << 16) & 0xff;
        buf[3] = (slen << 8) & 0xff;
        buf[4] = slen & 0xff;
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
        entry->encodingLen = 1;
        entry->contentLen = zipIntSize(encoding);
        // 字符串编码
    } else {
        entry->contentLen = zipStrSize(zl + len, &entry->encodingLen);
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
    struct zlentry *previousEntry = NULL;
    if (!isLastNode) {
        previousEntry = zipEntry(startPtr);
        previousLen = previousEntry->encodingLen;
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
    memcpy(startPtr, &previousSize, previousLen);
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
    struct zlentry *entry = zipEntry(p);

    // 是数字
    if ((entry->encoding >> 6) == 0b11) {
        *lval = zipLoadInt(entry->p, entry->encoding);
        // 字符串
    } else {
        if ((entry->encoding >> 6) == 0b00) {
            *slen = entry->encoding & 0b00111111;
        } else if ((entry->encoding >> 6) == 0b01) {
            *slen = ((entry->encoding & 0b00111111) << 8) | *((&entry->encoding) + 1);
        } else {

        }

        *sval = malloc(*slen);
        memcpy(*sval, p + entry->previousLen + entry->encodingLen, *slen);
    };
}

void printZl(unsigned char *zl) {
    for (int i = 0; i < ZIPLIST_BYTES(zl); ++i) {
        printf("%d ", *(zl + i));
    }
    printf("\n");
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

int main() {
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
    unsigned char *p = ziplistIndex(zl, 4);

    unsigned char *sVal = NULL;
    unsigned int slen = 0;
    long long lVal = 0;

    ziplistGet(p, &sVal, &slen, &lVal);
}