#include "bptree.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{
    BPtree bptree;
    init(&bptree);

    // 测试插入数据
    int test_data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    int n = sizeof(test_data) / sizeof(test_data[0]);
    printf("插入数据：");
    for (int i = 0; i < n; i++)
    {
        printf("%d ", test_data[i]);
        insert(&bptree, test_data[i]);
    }
    printf("\n");

    // 遍历所有数据
    show(&bptree);

    // 测试单点查找
    int search_key[] = {2, 7, 22};
    for (int i = 0; i < 3; i++)
    {
        if (search(&bptree, search_key[i]))
            printf("查找 %d：存在\n", search_key[i]);
        else
            printf("查找 %d：不存在\n", search_key[i]);
    }

    // 测试范围搜索
    range_search(&bptree, 3, 7);
    range_search(&bptree, 8, 15);
    range_search(&bptree, 21, 30);

    // 销毁树
    destroy(&bptree);
    return 0;
}
