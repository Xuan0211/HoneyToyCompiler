#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#if (defined _MSC_VER) && (defined _WIN64) || (defined __LP64__)
#define int int64_t
#endif


// VM
char *data;         // 数据段
int *code,          // 代码段
    *stack;         // 运行栈
int *pc, *sp, *bp, ax, cycle; // 寄存器
int poolsize;       // 各个段分配内存大小


int main(int argc, char** argv)
{
	int tmp;
	int fd;
	printf("hello word");
	return 0;
}