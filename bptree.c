#include "bptree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>



// 获取文件头指针（文件头就在 mmap 基址偏移0处）
static Header *get_header(BPTree *tree)
{
    return (Header *)tree->base;
}

// 根据偏移量获取节点指针：base + offset 直接还原
// 这就是 mmap 的核心优势——磁盘文件像内存一样直接访问
static Node *get_node(BPTree *tree, off_t offset)
{
    return (Node *)((char *)tree->base + offset);
}

// 分配一个新节点，返回它在文件中的偏移量
// 类似 malloc，但分配的是文件空间而非内存
static off_t alloc_node(BPTree *tree)
{
    Header *hdr = get_header(tree);
    off_t offset = hdr->next_free;
    hdr->next_free += sizeof(Node);

    // 初始化节点（mmap 映射的内存可能有脏数据，必须清零）
    Node *node = get_node(tree, offset);
    memset(node, 0, sizeof(Node));
    node->is_leaf = 1;  // 新建节点默认是叶子
    node->count = 0;
    return offset;
}

// 节点内二分查找：返回第一个 key >= target 的下标
// 用于叶子节点的插入定位和精确查找
static int binary_search(Node *node, int target)
{
    int left = 0, right = node->count - 1;
    while (left <= right) {
        int mid = (left + right) / 2;
        if (node->key[mid] < target)
            left = mid + 1;
        else
            right = mid - 1;
    }
    return left;
}


static void split_child(BPTree *tree, off_t parent_off, int child_index)
{
    Node *parent = get_node(tree, parent_off);
    off_t child_off = parent->child[child_index];
    Node *child = get_node(tree, child_off);

    // 分配新节点（分裂出的右半部分）
    off_t new_off = alloc_node(tree);
    Node *new_node = get_node(tree, new_off);
    new_node->is_leaf = child->is_leaf;

    int mid = child->count / 2;  // 分裂点

    if (child->is_leaf) {
        // ---------- 叶子节点分裂 ----------
        // 后半部分 [mid, count) 移到新节点
        int j = 0;
        for (int i = mid; i < child->count; i++) {
            new_node->key[j] = child->key[i];
            j++;
        }
        new_node->count = child->count - mid;
        child->count = mid;

        // 维护叶子双向链表：child[0]=next，child[1]=prev，偏移量0表示NULL
        new_node->child[0] = child->child[0];  // new->next = old->next
        new_node->child[1] = child_off;          // new->prev = old
        if (child->child[0] != 0) {
            Node *next = get_node(tree, child->child[0]);
            next->child[1] = new_off;            // next->prev = new
        }
        child->child[0] = new_off;                // old->next = new

        // B+树叶子分裂：提升新节点的第一个 key 到父节点
        int up_key = new_node->key[0];

        // 把 up_key 和新节点指针插入父节点的 child_index 位置
        for (int i = parent->count; i > child_index; i--) {
            parent->key[i] = parent->key[i - 1];
            parent->child[i + 1] = parent->child[i];
        }
        parent->key[child_index] = up_key;
        parent->child[child_index + 1] = new_off;
        parent->count++;
    } else {
        // ---------- 非叶子节点分裂 ----------
        // mid 位置的 key 提升到父节点（不保留在任何子节点）
        int up_key = child->key[mid];

        // mid+1 之后的 key 移到新节点
        int j = 0;
        for (int i = mid + 1; i < child->count; i++) {
            new_node->key[j] = child->key[i];
            j++;
        }
        new_node->count = child->count - mid - 1;

        // mid+1 之后的 child 指针移到新节点
        j = 0;
        for (int i = mid + 1; i <= child->count; i++) {
            new_node->child[j] = child->child[i];
            j++;
        }

        child->count = mid;

        // 把 up_key 和新节点指针插入父节点
        for (int i = parent->count; i > child_index; i--) {
            parent->key[i] = parent->key[i - 1];
            parent->child[i + 1] = parent->child[i];
        }
        parent->key[child_index] = up_key;
        parent->child[child_index + 1] = new_off;
        parent->count++;
    }
}

static void insert_non_full(BPTree *tree, off_t node_off, int key)
{
    Node *node = get_node(tree, node_off);

    if (node->is_leaf) {
        // ---------- 叶子节点：直接插入 ----------
        // 二分查找定位插入位置
        int pos = binary_search(node, key);
        // 重复值忽略
        if (pos < node->count && node->key[pos] == key)
            return;
        // 元素后移，空出位置
        for (int i = node->count; i > pos; i--)
            node->key[i] = node->key[i - 1];
        node->key[pos] = key;
        node->count++;
    } else {
        // ---------- 非叶子节点：找到要走的子节点 ----------
        // B+树非叶子的 key[i] 是分隔键：child[i+1] 子树中所有值 >= key[i]
        // 所以找第一个 key[i] > target 的位置，走 child[i]
        int i = 0;
        while (i < node->count && node->key[i] <= key)
            i++;
        int pos = i;

        off_t child_off = node->child[pos];
        Node *child = get_node(tree, child_off);

        // 子节点满了，先分裂（自顶向下分裂，保证递归下去子节点一定非满）
        if (child->count == ORDER) {
            split_child(tree, node_off, pos);
            // 分裂后父节点多了一个 key，重新判断走左边还是右边
            if (key > node->key[pos])
                pos++;
            child_off = node->child[pos];
        }

        // 递归插入到子节点
        insert_non_full(tree, child_off, key);
    }
}


int bptree_insert(BPTree *tree, int key)
{
    Header *hdr = get_header(tree);

    // 空树：创建第一个叶子节点作为根
    if (hdr->root == 0) {
        off_t root_off = alloc_node(tree);
        hdr->root = root_off;
        hdr->height = 1;
        Node *root = get_node(tree, root_off);
        root->is_leaf = 1;
        root->key[0] = key;
        root->count = 1;
        return 0;
    }

    Node *root = get_node(tree, hdr->root);

    // 根节点满了：创建新根，分裂旧根，树高+1
    if (root->count == ORDER) {
        off_t new_root_off = alloc_node(tree);
        Node *new_root = get_node(tree, new_root_off);
        new_root->is_leaf = 0;
        new_root->count = 0;
        new_root->child[0] = hdr->root;  // 新根的第一个孩子指向旧根
        hdr->root = new_root_off;
        hdr->height++;

        split_child(tree, new_root_off, 0);       // 分裂旧根
        insert_non_full(tree, new_root_off, key);  // 插入新值
    } else {
        insert_non_full(tree, hdr->root, key);
    }

    return 0;
}


int bptree_search(BPTree *tree, int key)
{
    Header *hdr = get_header(tree);
    if (hdr->root == 0)
        return 0;

    off_t node_off = hdr->root;
    while (1) {
        Node *node = get_node(tree, node_off);
        if (node->is_leaf) {
            // 到叶子了，二分查找精确匹配
            int pos = binary_search(node, key);
            return (pos < node->count && node->key[pos] == key);
        }
        // 非叶子：沿索引向下走
        int i = 0;
        while (i < node->count && node->key[i] <= key)
            i++;
        node_off = node->child[i];
    }
}


void bptree_range_search(BPTree *tree, int start, int end)
{
    Header *hdr = get_header(tree);
    if (hdr->root == 0 || start > end) {
        printf("范围搜索结果：空\n");
        return;
    }

    // 第一步：从根向下定位到 start 所在的叶子节点
    off_t node_off = hdr->root;
    while (1) {
        Node *node = get_node(tree, node_off);
        if (node->is_leaf)
            break;
        int i = 0;
        while (i < node->count && node->key[i] <= start)
            i++;
        node_off = node->child[i];
    }

    // 第二步：沿叶子双向链表顺序扫描，遇到 > end 就提前终止
    printf("范围搜索[%d, %d]：", start, end);
    int count = 0;
    while (node_off != 0) {
        Node *node = get_node(tree, node_off);
        for (int i = 0; i < node->count; i++) {
            if (node->key[i] > end)
                goto done;  // 超过上界，直接跳出所有循环
            if (node->key[i] >= start) {
                printf("%d ", node->key[i]);
                count++;
            }
        }
        node_off = node->child[0];  // 走到下一个叶子（next）
    }
done:
    if (count == 0)
        printf("无匹配数据");
    printf("\n");
}


void bptree_show(BPTree *tree)
{
    Header *hdr = get_header(tree);
    if (hdr->root == 0) {
        printf("空树\n");
        return;
    }

    // 找到最左边的叶子（一直走 child[0]）
    off_t node_off = hdr->root;
    while (1) {
        Node *node = get_node(tree, node_off);
        if (node->is_leaf)
            break;
        node_off = node->child[0];
    }

    // 沿链表顺序打印
    printf("B+树所有数据：");
    while (node_off != 0) {
        Node *node = get_node(tree, node_off);
        for (int i = 0; i < node->count; i++)
            printf("%d ", node->key[i]);
        node_off = node->child[0];
    }
    printf("\n");
}


BPTree *bptree_open(const char *path)
{
    BPTree *tree = malloc(sizeof(BPTree));
    if (!tree) return NULL;

    // 打开文件（不存在则创建）
    int fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        perror("open");
        free(tree);
        return NULL;
    }

    // 获取文件大小
    struct stat st;
    fstat(fd, &st);

    // 新文件：预分配空间（ftruncate 扩展文件大小）
    if (st.st_size == 0) {
        if (ftruncate(fd, INIT_FILE_SIZE) < 0) {
            perror("ftruncate");
            close(fd);
            free(tree);
            return NULL;
        }
    }

    size_t file_size = (st.st_size == 0) ? INIT_FILE_SIZE : st.st_size;


    void *base = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) {
        perror("mmap");
        close(fd);
        free(tree);
        return NULL;
    }

    tree->fd = fd;
    tree->base = base;
    tree->file_size = file_size;

    // 初始化或校验文件头
    Header *hdr = get_header(tree);
    if (st.st_size == 0) {
        // 新文件：写入文件头
        hdr->magic = MAGIC;
        hdr->page_size = PAGE_SIZE;
        hdr->root = 0;
        hdr->height = 0;
        hdr->next_free = PAGE_SIZE;  // 第一页留给文件头，节点从第2页开始
    } else if (hdr->magic != MAGIC) {
        // 已有文件但魔数不对：不是我们的B+树文件
        fprintf(stderr, "文件格式错误：魔数不匹配\n");
        munmap(base, file_size);
        close(fd);
        free(tree);
        return NULL;
    }

    return tree;
}


void bptree_close(BPTree *tree)
{
    if (!tree) return;
    msync(tree->base, tree->file_size, MS_SYNC);  // 强制把脏页写回磁盘
    munmap(tree->base, tree->file_size);            // 解除内存映射
    close(tree->fd);                                  // 关闭文件
    free(tree);                                       // 释放句柄
}
