#include "sds.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"

sds sdsnewlen(const void *init, size_t initlen) {

    struct sdshdr *sh;

    int size = sizeof(*sh) + initlen + 1;

    sh = malloc(size);

    memset(sh, 0x00, size);

    memcpy(sh->buf, init, initlen);

    sh->len = initlen;

    sh->buf[sh->len] = '\0';

    return sh->buf;
}

sds sdsnew(const char *init) {
    return sdsnewlen(init, strlen(init));
}

sds sdsempty(void) {
    return sdsnewlen(NULL, 0);
}

sds sdsdup(const sds s) {
    return sdsnewlen(s, sdslen(s));
}

void sdsfree(sds s) {
    free(s - sizeof(struct sdshdr));
}

sds sdsgrowzero(sds s, size_t len) {
    struct sdshdr *sh = (struct sdshdr *) (s - sizeof(struct sdshdr));

    if (sh->free >= len) {
        for (int i = 0; i < len; ++i) {
            sh->buf[len + i] = 0;
        }
        sh->len += len;
        sh->buf[len] = '\0';
    } else {

        int curLen = sh->len;
        int newLen = sh->len + len;

        sh = realloc(sh, sizeof(*sh) + (newLen * 2) + 1);

        sh->len = newLen;
        sh->free = newLen;

        memset(sh + curLen, 0x00, newLen);

        sh->buf[sh->len] = '\0';
    }
    return sh->buf;
}

sds sdscatlen(sds s, const void *t, size_t len) {
    struct sdshdr *sh = (struct sdshdr *) (s - sizeof(struct sdshdr));

    if (sh->free >= len) {
        for (int i = 0; i < len; ++i) {
            sh->buf[sh->len + i] = *(((char *) t) + i);
        }
        sh->len += len;
        sh->free -= len;
        sh->buf[sh->len] = '\0';
    } else {
        int curLen = sh->len;
        int newLen = curLen + len;

        sh = realloc(sh, sizeof(*sh) + (newLen * 2) + 1);

        sh->len = newLen;
        sh->free = newLen;

        for (int i = 0; i < len; ++i) {
            sh->buf[curLen + i] = ((char *) t)[i];
        }

        sh->buf[sh->len] = '\0';
    }
    return sh->buf;
}

sds sdscat(sds s, const char *t) {
    return sdscatlen(s, t, strlen(t));
}

sds sdscatsds(sds s, const sds t) {
    return sdscatlen(s, &t, sdslen(t));
}

// 将字符串 t 的前 len 个字符复制到 sds s 当中，
sds sdscpylen(sds s, const char *t, size_t len) {
    struct sdshdr *sh = (struct sdshdr *) (s - sizeof(struct sdshdr));
    if (sh->len + sh->free > len) {
        for (int i = 0; i < len; ++i) {
            sh->buf[i] = *(((char *) t) + i);
        }
        int free;
        sh->len = len;
        sh->free = (free = len - sh->free) <= 0 ? 0 : free;
    } else {
        int newLen = 2 * (sh->len + len - sh->free) + 1;
        sh = realloc(sh, newLen);
        sh->free = newLen - len;
        sh->len = len;
        memcpy(sh->buf, t, len);
    }
    return sh->buf;
}

sds sdstrim(sds s, const char *cset) {
    struct sdshdr *sh = (struct sdshdr *) (s - sizeof(struct sdshdr));

    char *h = s, *f = s;
    int hl = 0, fl = sh->len;

    while (hl <= sh->len && strchr(cset, *(h + hl))) {
        hl++;
    }

    if (hl == sh->len) {
        sdsfree(s);
        return sdsempty();
    }

    while (fl >= 0 && strchr(cset, *(f + fl))) {
        fl--;
    }

    char *remain = sh->buf + hl;

    int remainLen = fl - hl + 1;
    char buf[remainLen + 1];

    memcpy(buf, remain, remainLen);

    sds ret = sdsnewlen(remain, remainLen);

    sdsfree(s);

    return ret;
}

// 按索引对截取 sds 字符串的其中一段,start 和 end 都是闭区间（包含在内）
// start和end支持负数
void sdsrange(sds s, int start, int end) {
    struct sdshdr *sh = (struct sdshdr *) (s - sizeof(struct sdshdr));

    int s0 = start, e0 = end;

    if (start < 0) {
        s0 = sh->len - (-start);
        if (s0 >= sh->len) {
            return;
        }
    }

    if (end < 0) {
        e0 = sh->len - (-end);
        if (e0 >= sh->len) {
            return;
        }
    }

    if (s0 > e0) {
        return;
    }

    int idx = 0;

    for (int i = s0; i <= e0; ++i) {
        sh->buf[idx] = sh->buf[i];
        idx++;
    }

    sh->free = sh->len - (e0 - s0) + 1;
    sh->len = (e0 - s0) + 1;
    sh->buf[sh->len] = '\0';
}

void sdsclear(sds s) {
    struct sdshdr *sh = (struct sdshdr *) (s - sizeof(struct sdshdr));
    sh->free += sh->len;
    sh->len = 0;
    sh->buf[0] = '\0';
}

int sdscmp(const sds s1, const sds s2) {
    struct sdshdr *sh1 = (struct sdshdr *) (s1 - sizeof(struct sdshdr));
    struct sdshdr *sh2 = (struct sdshdr *) (s2 - sizeof(struct sdshdr));

    if (sh1->len != sh2->len) {
        return 0;
    }

    return strcmp(s1, s2) == 0;
}

// 使用分隔符 sep 对 s 进行分割，返回一个 sds 字符串的数组。
// *count 会被设置为返回数组元素的数量。
sds *sdssplitlen(const char *s, int len, const char *sep, int seplen, int *count) {

    int lastIdx = 0;

    sds *items = malloc(0);

    int itemLen = 0;

    for (int i = 0; i < len - seplen; ++i) {

        if (strncmp(s + i, sep, seplen) == 0) {
            if (lastIdx < i) {
                itemLen++;
                sds item = sdsnewlen(s + lastIdx, i - lastIdx);
                items = realloc(items, itemLen * sizeof(sds));
                items[itemLen - 1] = item;
                i += seplen;
                lastIdx = i;
            }

        }

    }

    if (itemLen == 0) {
        *count = 1;
        items = realloc(items, sizeof(sds));
        items[0] = sdsnewlen(s, len);
        return items;
    }

    *count = itemLen;

    return items;
}

void sdstolower(sds s) {
    int idx = 0;
    while (s[idx] != '\0') {
        s[idx] = tolower(s[idx]);
        idx++;
    }
}

void sdstoupper(sds s) {
    int idx = 0;
    while (s[idx] != '\0') {
        s[idx] = toupper(s[idx]);
        idx++;
    }
}

int sdsll2str(char *s, long long value) {
    char *p, aux;
    unsigned long long v;
    size_t l;

    /* Generate the string representation, this method produces
     * an reversed string. */
    v = (value < 0) ? -value : value;
    p = s;
    do {
        *p++ = '0' + (v % 10);
        v /= 10;
    } while (v);
    if (value < 0) *p++ = '-';

    /* Compute length and add null term. */
    l = p - s;
    *p = '\0';

    /* Reverse the string. */
    p--;
    while (s < p) {
        aux = *s;
        *s = *p;
        *p = aux;
        s++;
        p--;
    }
    return l;
}

sds sdsfromlonglong(long long value) {
    char buf[21];
    int len = sdsll2str(buf, value);

    return sdsnewlen(buf, len);
}

// sdsmapchars("hello", "ho", "01", 2) -> "0ell1"
sds sdsmapchars(sds s, const char *from, const char *to, size_t setlen) {
    int idx = 0;
    while (s[idx] != '\0') {

        for (int i = 0; i < setlen; ++i) {
            if (s[idx] == from[setlen]) {
                s[idx] = to[setlen];
            }
        }

        idx++;
    }
    return s;
}

// 将argc个char*使用sep链接成sds
sds sdsjoin(char **argv, int argc, char *sep) {
    sds empty = sdsempty();

    for (int i = 0; i < argc; ++i) {
        empty = sdscat(empty, argv[i]);
        if (i != argc - 1) {
            empty = sdscat(empty, sep);
        }
    }

    return empty;
}

int main() {
    char *strs[4];
    strs[0] = "zzzj";
    strs[1] = "1233";
    strs[2] = "hello";
    strs[3] = "world";

    printf("%s", sdsjoin(strs, 4, "|"));
}
