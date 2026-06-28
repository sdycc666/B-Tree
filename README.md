# 数据引擎：b+树的设计与实现方案
创建了一颗4阶B+树索引结构

## 1. 业务场景
关系型数据库：表的主键索引、二级索引，支持等值查询、范围查询。
文件系统：目录项的快速定位和文件元数据索引（如 ext4、XFS）。
内存数据库：有序数据结构的底层存储，如 LevelDB 的 MemTable。
操作系统页缓存管理：通过 B+树管理内存中的缓存页。

按时间戳等关键字范围查找数据
支持高效查找数据
天然支持区间查询与顺序扫描

## 2. 核心数据结构
- b+树
- 叶子节点（使用双向链表存储）
- 非叶子节点（内部节点，关键字作为双向链表的索引结构）
- 全局变量：
    - MAX：4（叶子节点最大存储数，非叶子节点最大存储关键字树MAX-1）
    - INT_MAX：用作哨兵值，标记节点中未存储的数组

## 3. 核心算法原理
该算法实现的是 B+树的自底向上插入，采用递归插入的方式

## 4. 核心流程设计
插入主函数： void insert_recursive(BPtree *B, void *node, int data, int depth)
- 若根节点为空 → 创建根叶子节点，插入数据，返回
- 若根节点为内部节点且已满 → 分裂根，提高树高，更新 root
- 调用 insert_recursive(root, data, depth=0)

## 5. 主要函数
- void init(BPtree *B);                 初始化b+树
- void insert(BPtree *B, int data);     节点数据插入函数
- int search(BPtree *B, int data);      单点查找函数
- void range_search(BPtree *B, int start, int end); 范围查找函数
- void show(BPtree *B);           遍历
- void destroy(BPtree *B);        销毁树

```c
// 创建非叶子节点
static NNode *CreateNNode()
{
    NNode *nnode = safe_malloc(sizeof(NNode));
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
    LNode *lnode = safe_malloc(sizeof(LNode));
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
     if (leaf->datanum >= MAX)
        return;
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

c```

## 6. 难点
- 所能存储的关键字数量不同导致叶子节点与非叶子节点的分裂差异
- 节点分裂与父节点索引更新
- 插入、分裂、删除时必须正确维护前后指针，否则范围查询和顺序遍历会中断

## 7. 不足
- 仅支持单点插入
- 阶数固定，无法满足多样要求
- 采用递归插入，数据量过大会导致栈溢出
