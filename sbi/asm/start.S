# start.S
# Starting entry point for the SBI

# This section (.text.init) is loaded into the top
# of the RAM 0x8000_0000, so our first instruction is
# csrr below.
.section .text.init
.global _start
_start:
    # We need the HARTID to determine the stack offset
	csrr	a0, mhartid
.option push
.option norelax
	la		sp, _stack_end
	la		gp, __global_pointer$
.option pop
    # a0 contains the mhartid
    # hartid << 12 = hartid x 4096
	slli	t0, a0, 12
	sub		sp, sp, t0

    # We use tail since we won't be returning.
	tail 	main
.size _start, . - _start
.type _start, function

.section .text
.global park
park:
	wfi         # wait for interrupt
	j	park
.size park, . - park
.type park, function
