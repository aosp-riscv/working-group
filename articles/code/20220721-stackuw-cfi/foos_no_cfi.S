	.file	"foos.c"
	.option nopic
	.text
	.align	1
	.globl	foo_3
	.type	foo_3, @function
foo_3:
.LFB0:
	.cfi_startproc
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sd	ra,8(sp)
	.cfi_offset 1, -8
	call	unwind_by_backtrace
	nop
	ld	ra,8(sp)
	.cfi_restore 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE0:
	.size	foo_3, .-foo_3
	.align	1
	.globl	foo_2
	.type	foo_2, @function
foo_2:
.LFB1:
	addi	sp,sp,-16
	sd	ra,8(sp)
	call	foo_3
	nop
	ld	ra,8(sp)
	addi	sp,sp,16
	jr	ra
.LFE1:
	.size	foo_2, .-foo_2
	.align	1
	.globl	foo_1
	.type	foo_1, @function
foo_1:
.LFB2:
	.cfi_startproc
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sd	ra,8(sp)
	.cfi_offset 1, -8
	call	foo_2
	nop
	ld	ra,8(sp)
	.cfi_restore 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE2:
	.size	foo_1, .-foo_1
	.align	1
	.globl	foo_0
	.type	foo_0, @function
foo_0:
.LFB3:
	.cfi_startproc
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sd	ra,8(sp)
	.cfi_offset 1, -8
	call	foo_1
	nop
	ld	ra,8(sp)
	.cfi_restore 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE3:
	.size	foo_0, .-foo_0
	.ident	"GCC: (g5964b5cd727) 11.1.0"
	.section	.note.GNU-stack,"",@progbits
