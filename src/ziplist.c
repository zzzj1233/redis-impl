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

int zipTryEncoding(unsigned char *s, unsigned int slen, long long *value, unsigned char *encoding) {
    long long numberValue = 0;

    if (string2ll(s, slen, &numberValue)) {
        if (numberValue >= 0 && numberValue <= 12) {
            *encoding = ZIP_INT_IMM_MIN + numberValue;
        } else if (numberValue >= INT8_MIN && numberValue <= INT8_MAX) {
            encoding = (unsigned char *) ZIP_INT_8B;
        } else if (numberValue >= INT16_MIN && numberValue <= INT16_MAX) {
            encoding = (unsigned char *) ZIP_INT_16B;
        } else if (numberValue >= INT32_MIN && numberValue <= INT32_MAX) {
            encoding = (unsigned char *) ZIP_INT_32B;
        } else {
            encoding = (unsigned char *) ZIP_INT_64B;
        }
        *value = numberValue;
        return 1;
    }

    return 0;
}

// 对字节数组进行编码,返回encoding的长度
// encoding将被追加到content的后面
uint8_t zipEncoding(unsigned char *zl, unsigned int slen) {
    char buf[5];

    uint8_t encodingLen = 0;

    if (slen <= 63) {
        encodingLen = 1;
        if (!zl) {
            return encodingLen;
        }
        buf[0] = 0b00 | slen;
    } else if (slen <= 16383) {
        encodingLen = 2;
        if (!zl) {
            return encodingLen;
        }
        buf[0] = 0b01000000 | (slen >> 8);
        buf[1] = slen << 8;
        // else if (slen <= 4294967295)
    } else {
        encodingLen = 5;
        if (!zl) {
            return encodingLen;
        }
        buf[0] = 0b10000000;
        buf[1] = slen << 24;
        buf[2] = slen << 16;
        buf[3] = slen << 8;
        buf[4] = slen;
    }

    memcpy(zl, buf, encodingLen);

    return encodingLen;
}

unsigned char *ziplistPush(unsigned char *zl, unsigned char *s, unsigned int slen, int where) {
    // 1. 尝试把s变成数字
    long long numberValue = 0;
    unsigned char encoding = 0;
    uint8_t encodingLen = 0;

    if (zipTryEncoding(s, slen, &numberValue, &encoding)) {
        encodingLen = 1;
    } else {
        zipEncoding(zl, slen);
    }
}


int main() {
    unsigned char *zl = ziplistNew();

}