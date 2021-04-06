#include <stddef.h>
#include "endianconv.h"

#define ZIPLIST_HEAD 0
#define ZIPLIST_TAIL 1

#define ZIPLIST_BYTES(zl)       (*((uint32_t*)(zl)))
#define ZIPLIST_TAIL_OFFSET(zl) (*((uint32_t*)((zl)+sizeof(uint32_t))))
#define ZIPLIST_HEADER_SIZE     (sizeof(uint32_t)*2+sizeof(uint16_t))
#define ZIPLIST_LENGTH(zl)      (*((uint16_t*)((zl)+sizeof(uint32_t)*2)))
#define ZIP_END 255

#define ZIP_INT_IMM_MASK 0x0f
#define ZIP_INT_IMM_MIN 0xf1    /* 11110001 */
#define ZIP_INT_IMM_MAX 0xfd    /* 11111101 */
#define ZIP_INT_IMM_VAL(v) (v & ZIP_INT_IMM_MASK)

// 11000000
#define ZIP_INT_16B (0xc0 | 0<<4)
// 11010000
#define ZIP_INT_32B (0xc0 | 1<<4)
// 11100000
#define ZIP_INT_64B (0xc0 | 2<<4)
// 11110000
#define ZIP_INT_24B (0xc0 | 3<<4)
// 11111110
#define ZIP_INT_8B 0xfe

/*
 * 24 位整数的最大值和最小值
 */
#define INT24_MAX 0x7fffff
#define INT24_MIN (-INT24_MAX - 1)

typedef struct zlentry {
    unsigned int previousLen;
    // 1 | 5
    unsigned int previousSize;

    unsigned char encoding;

    unsigned int encodingLen;

    unsigned int contentLen;

    unsigned char *p;
};

unsigned char *ziplistNew(void);

unsigned char *ziplistPush(unsigned char *zl, unsigned char *s, unsigned int slen, int where);

unsigned char *ziplistIndex(unsigned char *zl, int index);

unsigned char *ziplistNext(unsigned char *zl, unsigned char *p);

unsigned char *ziplistPrev(unsigned char *zl, unsigned char *p);

unsigned int ziplistGet(unsigned char *p, unsigned char **sval, unsigned int *slen, long long *lval);

unsigned char *ziplistInsert(unsigned char *zl, unsigned char *p, unsigned char *s, unsigned int slen);

unsigned char *ziplistDelete(unsigned char *zl, unsigned char **p);

unsigned char *ziplistDeleteRange(unsigned char *zl, unsigned int index, unsigned int num);

unsigned int ziplistCompare(unsigned char *p, unsigned char *s, unsigned int slen);

unsigned char *ziplistFind(unsigned char *p, unsigned char *vstr, unsigned int vlen, unsigned int skip);

unsigned int ziplistLen(unsigned char *zl);

size_t ziplistBlobLen(unsigned char *zl);
