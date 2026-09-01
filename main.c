#include "bptree.h"
#include <stdio.h>

void menu()
{
    printf("\n==================== 持久化B+树操作菜单 ====================\n");
    printf("1. 插入数据\n");
    printf("2. 单点查找\n");
    printf("3. 区间范围查找 [start, end]\n");
    printf("4. 打印树中全部数据\n");
    printf("0. 退出程序（自动保存到磁盘）\n");
    printf("=============================================================\n");
    printf("请输入功能序号：");
}

int main()
{
    // 打开/创建 B+树文件（数据存在 bptree.dat 中，程序重启后仍在）
    BPTree *tree = bptree_open("bptree.dat");
    if (!tree) {
        fprintf(stderr, "打开B+树文件失败\n");
        return 1;
    }

    int op;
    while (1) {
        menu();
        // 检查 scanf 返回值，防止非法输入导致死循环
        if (scanf("%d", &op) != 1) {
            printf("输入无效，请输入数字\n");
            while (getchar() != '\n');  // 清空输入缓冲区
            continue;
        }

        switch (op) {
            case 1: {
                int val;
                printf("请输入要插入的整数：");
                scanf("%d", &val);
                bptree_insert(tree, val);
                printf("插入完成\n");
                break;
            }
            case 2: {
                int val;
                printf("请输入要查找的整数：");
                scanf("%d", &val);
                if (bptree_search(tree, val))
                    printf("查找成功：%d 存在于树中\n", val);
                else
                    printf("查找失败：%d 不存在\n", val);
                break;
            }
            case 3: {
                int l, r;
                printf("请输入区间起点 start：");
                scanf("%d", &l);
                printf("请输入区间终点 end：");
                scanf("%d", &r);
                bptree_range_search(tree, l, r);
                break;
            }
            case 4:
                bptree_show(tree);
                break;
            case 0:
                bptree_close(tree);  // 内部 msync 写盘 + munmap + close
                printf("数据已保存到磁盘，程序退出\n");
                return 0;
            default:
                printf("输入序号无效，请重新选择\n");
        }
    }
}
