/*
 * @file    main.c
 * @brief   指针基础练习：swap、数组退化、函数指针
 */
#include <stdio.h>
// ── 1. 指针交换两数 ────── //
void swap(int *a, int *b)
{
    int temp = *a;    // 用 * 取 a 指向的值
    *a = *b;
    *b = temp;
}
// 2. 数组名退化为指针 ─────*/
void print_array(int *arr, int len)
{
    // arr 传入时已退化为指针，sizeof(arr) 不再等于数组总大小
    for (int i = 0; i < len; i++)
    {
        printf("arr[%d] = %d\n", i, arr[i]);
    }
}
/* ── 3. 函数指针 ──────── */
int add(int x, int y) { return x + y; }
/*————————————————————————————*/


int main(void)
{
    /* 测试 1：swap */
    int x = 10, y = 20;
    printf("before: x=%d, y=%d\n", x, y);
    swap(&x, &y);                             // & 取地址传给指针
    printf("after:  x=%d, y=%d\n", x, y);
    /* 测试 2：数组退化 */
    int nums[] = {1, 2, 3, 4, 5};
    int len = sizeof(nums) / sizeof(nums[0]); // 这里 sizeof 还能用，因为 nums 还是数组
    print_array(nums, len);                   // 传入后 nums 退化为 int*
    /* 测试 3：函数指针 */
    int (*func_ptr)(int, int) = &add;         // 声明函数指针，指向 add
    printf("add(3, 5) via func_ptr = %d\n", func_ptr(3, 5));
    return 0;
}