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

// 表示开区间/闭区间范围的结构
typedef struct {

    // 最小值和最大值
    double min, max;

    // 指示最小值和最大值是否*不*包含在范围之内
    // 值为 1 表示不包含，值为 0 表示包含
    int minex, maxex;
} zrangespec;


zskiplist *zslCreate(void);

zskiplistNode *zslCreateNode(int level, void *obj, double score);

zskiplistNode *zslInsert(zskiplist *zsl, double score, void *obj);

zskiplistNode *zslFirstInRange(zskiplist *zsl, zrangespec *range);

void zslFree(zskiplist *zsl);

double zzlGetScore(unsigned char *sptr);

void zzlNext(unsigned char *zl, unsigned char **eptr, unsigned char **sptr);

void zzlPrev(unsigned char *zl, unsigned char **eptr, unsigned char **sptr);