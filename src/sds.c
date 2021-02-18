#include "sds.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

sds sdsnewlen(const void *init, size_t initlen) {

    struct sdshdr *sh;

    int size = sizeof(*sh) + initlen + 1;

    sh = malloc(size);

    memset(sh, 0x00, size);

    memcpy(sh->buf, init, initlen);

    sh->len = initlen;

    sh->buf[sh->len + 1] = '\0';

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
        int curLen = sizeof(struct sdshdr) + sh->len;
        int newLen = 2 * (sh->len + len - sh->free) + 1;
        sh = realloc(sh, newLen);
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
        sh->len += +len;
        sh->free -= len;
        sh->buf[len] = '\0';
    } else {
        int curLen = sh->len;
        int newLen = sh->len + len - sh->free + 1;

        sh = realloc(sh, newLen);

        sh->len = newLen;

        for (int i = 0; i < len; ++i) {
            sh->buf[curLen + i] = *(((char *) t) + i);
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

}

int main() {
    sds s;
    s = sdsnew("AA...AA.a.aa.aHelloWorld     :::");

    s = sdstrim(s, "Aa. ");

    printf("%s", s);
}
