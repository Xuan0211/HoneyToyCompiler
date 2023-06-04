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

#define POOL_SIZE 256 * 1024 //256KB

// debug
int debug;// 调试模式
int *last_code;// 上一次打印至的code段指针


// VM
char *data;         // 数据段
int *code,          // 代码段
    *stack;         // 运行栈
int *pc, *sp, *bp, ax, cycle; // 寄存器
int poolsize;       // 各个段分配内存大小

// 定义一个枚举类型，需要使用 enum 关键字，后面跟着枚举类型的名称，以及用大括号 {} 括起来的一组枚举常量。
// 每个枚举常量可以用一个标识符来表示，也可以为它们指定一个整数值，如果没有指定，那么默认从 0 开始递增。
enum Instruction
{
	/*-----------  save & load  -----------*/

	// load immediate to ax 加载立即数
	IMM = 100, 
	// load address to ax X86经典
	LEA, 
	// load char to ax
	LC, 
	// load int to ax
	LI, 
	// save char to address in stack top, then pop
	SC, 
	// save int to address in stack top, then pop
	SI, 
	// push ax to stack top
	PUSH,

	/*-----------  运算  -----------*/

	ADD, SUB, MUL, DIV, MOD, 
	OR, XOR, AND, SHL, SHR,
	EQ, NE, LT, LE, GT, GE,

	//TODO:
	//这里的NOT实现比较特殊

	/*-----------  分支跳转  -----------*/

	JMP, JZ, JNZ, CALL, NVAR, DARG, RET, 
	//adjust stack
	ADJ,
	// enter subroutine, make new stack frame
	ENT,

	/*-----------   Native-Call   -----------*/
	// 这部分是C4为了实现自举使用的一些比较特殊的“作弊”指令
	// 多为仿系统调用

	// 打开文件
	OPEN, 
	// 关文件 close
	CLOS,
	// 读文件
	READ,
	// 写文件
	WRIT,
	// 输出 printf
	PRTF,
	// malloc
	MALC,
	// free
	FREE,
	// memset
	MSET,
	// memcmp 把存储区 str1 和存储区 str2 的前 n 个字节进行比较。
	MCMP,
	// memcpy  从存储区 str2 复制 n 个字节到存储区 str1。
	MCPY,
	// exit
	EXIT
};

int run_vm()
{
	int op,*tmp;
	while(1)
	{
		op = *pc++;
		cycle++;
		if(debug == 1)
		{
			//TODO:
			//debug somthing 
			//这里由于指令集写的和参考工程不太一样，注意不要照抄啊
		}

		/*-----------  save & load  -----------*/
		if (op == IMM) { ax = *pc++; } //这里立即数放在pc的后一个位置上
		else if (op == LEA) { ax = (int)(bp + *pc++); }// 相对于bp取出
		else if (op == LC) { ax = *(char*)ax; }
		else if (op == LI) { ax = *(int*)ax; }		
		// 注意栈是从大的开始存，也就是 ++ 是pop -- 是 push 
		// 这一堆*来*去的特很精髓 *sp取出地址（此时是个普通的数），（char*)把这个数变成地址（这个一般在data上）,然后再上*后把ax当右值赋过来
		// 这样就实现把ax里的数save到data里了
		else if (op == SC) { *(char*)*sp++ = ax; }
		else if (op == SI ) { *(int*)*sp++ = ax; }
		else if (op == PUSH) { *--sp = ax; }// 这里是先把栈顶指针往下了一格，然后再赋值进去

		/*-----------  运算  -----------*/
		else if (op == ADD) ax = *sp++ + ax;
        else if (op == SUB) ax = *sp++ - ax;
        else if (op == MUL) ax = *sp++ * ax;
        else if (op == DIV) ax = *sp++ / ax;
        else if (op == MOD) ax = *sp++ % ax;
        else if (op == AND) ax = *sp++ & ax;
        else if (op == OR)  ax = *sp++ | ax;
        else if (op == XOR) ax = *sp++ ^ ax;
        else if (op == SHL) ax = *sp++ << ax;
        else if (op == SHR) ax = *sp++ >> ax;
		else if (op == EQ)  ax = *sp++ == ax;
        else if (op == NE)  ax = *sp++ != ax;
        else if (op == LT)  ax = *sp++ < ax;
        else if (op == LE)  ax = *sp++ <= ax;
        else if (op == GT)  ax = *sp++ > ax;
        else if (op == GE)  ax = *sp++ >= ax;

		/*-----------  分支跳转  -----------*/
		else if(op == JMP) pc=(int*)*pc;
		// 这里还有点坑啊 是“=”不是“==”   这个应该是pc是个头 然后后面是一体的一个表达式
		else if (op == JZ) { pc = ax ? pc + 1 : (int*)*pc; } // jump to address if ax is zero
		else if (op == JNZ) { pc = ax ? (int*)*pc : pc + 1; }
		else if (op == CALL) { pc = (int*)ax}		


	}
}

int main(int argc, char** argv)
{
	int tmp;
	int fd;
	int *tmpp;

	//TODO:
	//something init



	poolsize = POOL_SIZE;	

	// 为VM分配内存
	if (!(code = (int*)malloc(poolsize)))
	{
		printf("ERROR: Could not malloc(%d) for code area \n", poolsize);
		return -1;
	}

	if (!(stack = (int*)malloc(poolsize)))
	{
		printf("ERROR: Could not malloc(%d) for stack area \n", poolsize);
		return -1;
	}

	if (!(code = (char*)malloc(poolsize)))
	{
		printf("ERROR: Could not malloc(%d) for code area \n",poolsize);
		return -1;
	}

	memset(code, 0, poolsize);
	memset(stack, 0, poolsize);
	memset(data, 0, poolsize);
	
	//TODO:
	//为parser分配内存


	// 初始化VM寄存器
	// 移动bp sp到栈顶 注意只有stack是从大到小的，code和data都是从小到大的，就是0在上 MAX再下
	bp = sp = (int*)((int)stack + poolsize);
	// 往栈中推入两条指令
	*--sp = EXIT;
	*--sp = PUSH;

	//TODO:
	//看不懂 谢谢
	tmpp = sp;

	*--sp = argc;
	*--sp = (int)argv;
	*--sp = (int)tmpp;

	return run_vm();
}