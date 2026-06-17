#ifndef BPTREE_H
#define BPTREE_H

#include <stdlib.h>

#define MAX 4
#define MY_INT_MAX 0x7FFFFFFF // 自定义int最大值，把未使用的空位都填充成这个最大值，用来标记

// 非叶子节点
typedef struct NNode
{
    int key[MAX - 1];
    int keynum;
    void *child[MAX];
} NNode;

// 叶子节点
typedef struct LNode
{
    int data[MAX];
    int datanum;
    struct LNode *next;
    struct LNode *prev;
} LNode;

// 树结构
typedef struct BPtree
{
    void *root;
    LNode *leaf_head;
    LNode *leaf_tail;
    int height;
} BPtree;

void menu();
void init(BPtree *B);
void insert(BPtree *B, int data);
int search(BPtree *B, int data);
void range_search(BPtree *B, int start, int end);
void show(BPtree *B);
void destroy(BPtree *B);

#endif // BPTREE_H