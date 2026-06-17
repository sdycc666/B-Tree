#include "bptree.h"
#include <stdio.h>

// 创建非叶子节点
static NNode *CreateNNode()
{
    NNode *nnode = malloc(sizeof(NNode));
    nnode->keynum = 0;
    for (int i = 0; i <= MAX - 1; i++)
        nnode->child[i] = NULL;
    for (int i = 0; i < MAX - 1; i++)
        nnode->key[i] = MY_INT_MAX;
    return nnode;
}

// 创建叶子节点
static LNode *CreateLNode()
{
    LNode *lnode = malloc(sizeof(LNode));
    lnode->datanum = 0;
    lnode->next = NULL;
    lnode->prev = NULL;
    for (int i = 0; i < MAX; i++)
        lnode->data[i] = MY_INT_MAX;
    return lnode;
}

// 升序插入数据到叶子节点
static void insertlnode(LNode *leaf, int data)
{
    int i = leaf->datanum - 1;
    while (i >= 0 && leaf->data[i] >= data)
    {
        leaf->data[i + 1] = leaf->data[i];
        i--;
    }
    leaf->data[i + 1] = data;
    leaf->datanum++;
}

// 分裂满的叶子节点
static LNode *splitleaf(LNode *leaf, int *upkey)
{
    LNode *newnode = CreateLNode();
    int mid = leaf->datanum / 2;

    int j = 0;
    for (int i = mid; i < leaf->datanum; i++)
    {
        newnode->data[j] = leaf->data[i];
        newnode->datanum++;
        leaf->data[i] = MY_INT_MAX;
        j++;
    }
    leaf->datanum = mid;

    *upkey = newnode->data[0];

    // 维护双向链表
    newnode->next = leaf->next;
    newnode->prev = leaf;
    if (leaf->next != NULL)
    {
        leaf->next->prev = newnode;
    }
    leaf->next = newnode;

    return newnode;
}

// 升序插入索引和子节点到非叶子节点
static void insertnnode(NNode *nnode, int key, void *rightchild)
{
    int i = nnode->keynum - 1;
    while (i >= 0 && nnode->key[i] > key)
    {
        nnode->key[i + 1] = nnode->key[i];
        nnode->child[i + 2] = nnode->child[i + 1];
        i--;
    }
    nnode->key[i + 1] = key;
    nnode->child[i + 2] = rightchild;
    nnode->keynum++;
}

// 分裂满的非叶子节点
static NNode *splitnnode(NNode *nnode, int *upkey)
{
    NNode *newnode = CreateNNode();
    int mid = nnode->keynum / 2;
    *upkey = nnode->key[mid];

    int j = 0;
    for (int i = mid + 1; i < nnode->keynum; i++)
    {
        newnode->key[j] = nnode->key[i];
        newnode->keynum++;
        nnode->key[i] = MY_INT_MAX;
        j++;
    }

    j = 0;
    for (int i = mid + 1; i <= nnode->keynum; i++)
    {
        newnode->child[j] = nnode->child[i];
        j++;
    }

    nnode->keynum = mid;
    return newnode;
}

// 递归插入
static void insert_recursive(BPtree *B, void *node, int data, int depth)
{
    // 深度等于高度，到叶子节点
    if (depth == B->height)
    {
        LNode *leaf = (LNode*)node;
        // 检查重复
        for (int i = 0; i < leaf->datanum; i++)
        {
            if (leaf->data[i] == data)
                return;
        }
        insertlnode(leaf, data);
        return;
    }

    // 非叶子节点
    NNode *nnode = (NNode*)node;
    // 找到子节点
    int i = 0;
    while (i < nnode->keynum && data >= nnode->key[i])
        i++;
    void *child = nnode->child[i];

    // 检查子节点是否满了
    int child_is_full = 0;
    if (depth + 1 == B->height)
    {
        child_is_full = ((LNode*)child)->datanum == MAX;
    }
    else
    {
        child_is_full = ((NNode*)child)->keynum == MAX - 1;
    }

    // 子节点满了先分裂
    int child_upkey = 0;
    void *child_newnode = NULL;
    if (child_is_full)
    {
        if (depth + 1 == B->height)
        {
            child_newnode = splitleaf((LNode*)child, &child_upkey);
        }
        else
        {
            child_newnode = splitnnode((NNode*)child, &child_upkey);
        }
        insertnnode(nnode, child_upkey, child_newnode);
        // 重新决定插入的子节点
        if (data > child_upkey)
        {
            i++;
            child = child_newnode;
        }
    }

    // 递归插入
    insert_recursive(B, child, data, depth + 1);
}

// 销毁节点
static void destroy_node(BPtree *B, void *node, int depth)
{
    if (node == NULL)
        return;
    if (depth < B->height)
    {
        NNode *nnode = (NNode*)node;
        for (int i = 0; i <= nnode->keynum; i++)
        {
            destroy_node(B, nnode->child[i], depth + 1);
        }
    }
    free(node);
}


void init(BPtree *B)
{
    B->root = NULL;
    B->leaf_head = NULL;
    B->leaf_tail = NULL;
    B->height = 0;
}

void insert(BPtree *B, int data)
{
    // 空树第一次插入
    if (B->root == NULL)
    {
        LNode *leaf = CreateLNode();
        insertlnode(leaf, data);
        B->root = leaf;
        B->leaf_head = leaf;
        B->leaf_tail = leaf;
        B->height = 1;
        return;
    }

    // 检查根是否满了
    int root_is_full = 0;
    if (B->height == 1)
    {
        root_is_full = ((LNode*)B->root)->datanum == MAX;
    }
    else
    {
        root_is_full = ((NNode*)B->root)->keynum == MAX - 1;
    }

    // 根满了分裂根
    if (root_is_full)
    {
        int upkey;
        void *newnode;
        if (B->height == 1)
        {
            newnode = splitleaf((LNode*)B->root, &upkey);
            if (((LNode*)newnode)->next == NULL)
                B->leaf_tail = (LNode*)newnode;
        }
        else
        {
            newnode = splitnnode((NNode*)B->root, &upkey);
        }
        // 创建新根
        NNode *new_root = CreateNNode();
        new_root->key[0] = upkey;
        new_root->child[0] = B->root;
        new_root->child[1] = newnode;
        new_root->keynum = 1;
        B->root = new_root;
        B->height++;
    }

    // 递归插入
    insert_recursive(B, B->root, data, 1);
}

int search(BPtree *B, int data)
{
    if (B->root == NULL)
        return 0;

    void *node = B->root;
    int depth = 1;
    while (depth < B->height)
    {
        NNode *nnode = (NNode*)node;
        int i = 0;
        while (i < nnode->keynum && data >= nnode->key[i])
            i++;
        node = nnode->child[i];
        depth++;
    }

    LNode *leaf = (LNode*)node;
    for (int i = 0; i < leaf->datanum; i++)
    {
        if (leaf->data[i] == data)
            return 1;
    }
    return 0;
}

void range_search(BPtree *B, int start, int end)
{
    if (B->root == NULL || start > end)
    {
        printf("范围搜索结果：空\n");
        return;
    }

    // 定位起点
    void *node = B->root;
    int depth = 1;
    while (depth < B->height)
    {
        NNode *nnode = (NNode*)node;
        int i = 0;
        while (i < nnode->keynum && start >= nnode->key[i])
            i++;
        node = nnode->child[i];
        depth++;
    }

    // 遍历叶子链表
    LNode *leaf = (LNode*)node;
    printf("范围搜索[%d, %d]的结果：", start, end);
    int count = 0;
    while (leaf != NULL)
    {
        for (int i = 0; i < leaf->datanum; i++)
        {
            int data = leaf->data[i];
            if (data > end)
            {
                goto end_search;
            }
            if (data >= start)
            {
                printf("%d ", data);
                count++;
            }
        }
        leaf = leaf->next;
    }
end_search:
    if (count == 0)
    {
        printf("无匹配数据");
    }
    printf("\n");
}

void show(BPtree *B)
{
    if (B->leaf_head == NULL)
    {
        printf("空树\n");
        return;
    }
    LNode *leaf = B->leaf_head;
    printf("B+树所有数据：");
    while (leaf != NULL)
    {
        for (int i = 0; i < leaf->datanum; i++)
        {
            printf("%d ", leaf->data[i]);
        }
        leaf = leaf->next;
    }
    printf("\n");
}

void destroy(BPtree *B)
{
    destroy_node(B, B->root, 1);
    B->root = NULL;
    B->leaf_head = NULL;
    B->leaf_tail = NULL;
    B->height = 0;
}
