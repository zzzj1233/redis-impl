#include <stddef.h>

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

#define ZIP_INT_16B (0xc0 | 0<<4)
#define ZIP_INT_32B (0xc0 | 1<<4)
#define ZIP_INT_64B (0xc0 | 2<<4)
#define ZIP_INT_24B (0xc0 | 3<<4)
#define ZIP_INT_8B 0xfe

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
