# start.S
# Boot loader for our operating system.
# Stephen Marz
# 19 May 2022
# COSC562 Operating Systems: Implementation and Design
.section .text.init

#include <config.h>

# Technically, this can be placed anywhere, but there is less
# searching if we put it right at the top of the search range.

# We are required to align by 8 since our target
# alignment from the SBI is also 8. This makes searching
# faster.
# This method allows us to set the entry point address anywhere
# we want.
.align 8
# The SBI will search for the following magic number, expecting
# it to be 8 bytes (.dword = double word = 8 bytes)
.dword OS_TARGET_MAGIC
# The very next field will be the target boot address, which in
# this case is _start
.dword _start

.section .text
.global _start
_start:
.option push
.option norelax
	la		sp, _stack_end
	la		gp, __global_pointer$
.option pop
    # Unlike the SBI, we will be trap returning so that
    # we know the registers sstatus, sip, stvec, and sepc are
    # in a place where we expect them to be. We can just jump
    # into main (tail main) like the SBI, but this ensures the CSRs
    # are what we say they are instead of just assuming from the SBI.
	la		t0, main
	csrw	sepc, t0
	li		t0, 1 << 9
	csrw	sie, t0
    # 1 << 13 - FS:  in the "initial" position
    # 1 << 8  - SPP: into Supervisor Mode
    # 1 << 5  - SPIE: enable interrupts
	li		t0, (1 << 13) | (1 << 8) | (1 << 5)
	csrw	sstatus, t0
    # Load the return address (RA) to the park. When main
    # returns, it will park the HART.
	la		ra, park
	csrr	a0, sscratch
	sret
.type _start, function
.size _start, . - _start

.section .text
.global park
.align 4
park:
	wfi
	j	park
park_end:
.type park, function
.size park, . - park

# void *idleproc(void);
# uint64_t idleprocsize;
# An idle process is essentially the parking loop with STIE enabled
# so we can context switch via timer.
.section .rodata
.global idleproc
idleproc: .dword park
.size idleproc, . - idleproc
.type idleproc, object

.global idleprocsize
idleprocsize: .dword park_end - park
.size idleprocsize, . - idleprocsize
.type idleprocsize, object

