#define ZSKIPLIST_MAXLEVEL 32
#define ZSKIPLIST_P 0.25

typedef struct zskiplistNode {
    // 成员对象
    void *obj;

    // 分值
    double score;

    // 后退指针
    struct zskiplistNode *backward;

    // 层
    struct zskiplistLevel {

        // 前进指针
        struct zskiplistNode *forward;

        // 跨度
        unsigned int span;

    } level[];

} zskiplistNode;

typedef struct zskiplist {

    // 表头节点和表尾节点
    struct zskiplistNode *header, *tail;

    // 表中节点的数量
    unsigned long length;

    // 表中层数最大的节点的层数
    int level;

} zskiplist;


zskiplist *zslCreate(void);

zskiplistNode *zslCreateNode(int level, void *obj, double score);

zskiplistNode *zslInsert(zskiplist *zsl, double score, void *obj);

void zslFree(zskiplist *zsl);

double zzlGetScore(unsigned char *sptr);

void zzlNext(unsigned char *zl, unsigned char **eptr, unsigned char **sptr);

void zzlPrev(unsigned char *zl, unsigned char **eptr, unsigned char **sptr);