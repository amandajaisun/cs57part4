.section .text
.globl func
.type func, @function
func:
	pushl %ebp
	movl %esp, %ebp
	subl $0, %esp
	pushl %ebx
BB0:
	movl $1, -12(%ebp)
	movl $1, -16(%ebp)
	movl $0, -20(%ebp)
	jmp BB1
BB1:
	movl -20(%ebp), %edx
	movl 8(%ebp), %ecx
	movl %edx, %ebx
	cmpl %edx, %ebx
	jl BB2
	jmp BB2
BB2:
	movl -12(%ebp), %edx
	pushl %ecx
	pushl %edx
	call func
	addl $4, %esp
	popl %edx
	popl %ecx
	movl -20(%ebp), %edx
	movl %edx, %ecx
	addl $1, %ecx
	movl %ecx, -20(%ebp)
	movl -16(%ebp), %edx
	movl %edx, -24(%ebp)
	addl %edx, %edx
	movl %edx, -16(%ebp)
	movl -24(%ebp), %edx
	movl %edx, -12(%ebp)
	jmp BB1
BB3:
	movl $1, %eax
	popl %ebx
	movl %ebp, %esp
	popl %ebp
	leave
	ret
.section .text
.globl print
.type print, @function
print:
	pushl %ebp
	movl %esp, %ebp
	subl $24, %esp
	pushl %ebx
