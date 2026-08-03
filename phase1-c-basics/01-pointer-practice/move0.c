#include <stdio.h>
// 力扣 283. 移动零// 力扣 283. 移动零
// 双指针思路：slow 指向下一个非零元素的落点，fast 扫描整个数组。
// 每次 fast 发现非零元素，就与 slow 位置交换，slow 前进。
// 为什么用双指针：单指针需要额外数组，双指针原地完成。
void moveZeroes(int *nums, int numsSize)
{
    int slow = 0; // slow: 下一个非零元素的位置

    for (int fast = 0; fast < numsSize; fast++) {
        if (nums[fast] != 0) {
            // 把非零元素换到 slow 处，slow 处的旧值，被换到 fast 处
            int temp = nums[slow];
            nums[slow] = nums[fast];
            nums[fast] = temp;
            slow++;
        }
    }
}

/* 验证
int main(void)
{
    int nums[] = {0, 1, 0, 3, 12};
    int size = sizeof(nums) / sizeof(nums[0]);

    moveZeroes(nums, size);

    for (int i = 0; i < size; i++) {
        printf("%d ", nums[i]);
    }
    printf("\n");
    return 0;
}
*/