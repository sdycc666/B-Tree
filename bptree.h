#ifndef BPTREE_H
#define BPTREE_H

#include <stddef.h>
#include <sys/types.h>

#define ORDER 4                // B+树阶数：叶子最多存4个key，非叶子最多存3个key
#define PAGE_SIZE 4096         // 页大小（第一页存文件头）
#define MAGIC 0x42505452       // 文件魔数 "BPTR"
#define INIT_FILE_SIZE (16 * 1024 * 1024)  // 预分配16MB

typedef struct {
    int magic;
    int page_size;
    off_t root;
    int height;
    off_t next_free;
} Header;

typedef struct {
    int is_leaf;
    int count;
    int key[ORDER];
    off_t child[ORDER + 1];
} Node;


typedef struct {
    int fd;
    void *base;
    size_t file_size;
} BPTree;

// 打开/创建 B+树文件（内部做 mmap 映射）
BPTree *bptree_open(const char *path);
// 关闭：同步写盘 + 解除映射 + 释放资源
void bptree_close(BPTree *tree);
// 插入一个整数，重复值忽略
int bptree_insert(BPTree *tree, int key);
// 单点查找，存在返回1，不存在返回0
int bptree_search(BPTree *tree, int key);
// 区间查找 [start, end]，打印结果
void bptree_range_search(BPTree *tree, int start, int end);
// 沿叶子链表打印全部数据
void bptree_show(BPTree *tree);

#endif
