# trampoline.S
# This file contains the trampoline code required to
# spawn and trap a process.
# Stephen Marz
# 19 May 2022
# COSC562 Operating Systems: Implementation and Design

.altmacro
.macro loadgp  i, r=t6
	ld	x\i, (\i * 8)(\r)
.endm

.macro savegp  i, r=t6
    sd  x\i, (\i * 8)(\r)
.endm

.macro loadfp  i, r=t6
	fld f\i, (\i * 8 + 256)(\r)
.endm

.macro savefp  i, r=t6
	fsd f\i, (\i * 8 + 256)(\r)
.endm

.section .trampoline
_spawn_kthread:
    csrr        t6, sscratch

	# Spawn table fields
	# 0    - gpregs
    # 256  - fpregs
    # 512  - sepc
    # 520  - sstatus
    # 528  - sie
    # 536  - satp
    # 544  - sscratch
    # 552  - stvec
    # 560  - trap_satp
    # 568  - trap_stack

	ld		t0, 512(t6)   # sepc
	csrw	sepc, t0

	ld		t4, 520(t6)   # sstatus
	csrw	sstatus, t4

	ld		t0, 528(t6)  # sie
	csrw	sie, t0

	ld		t0, 552(t6)  # stvec
	csrw	stvec, t0

	ld		t1, 536(t6)  # satp

	ld		t6, 544(t6)  # sscratch
	csrw	sscratch, t6

    # We need a proper sscratch before we
    # turn on the MMU
	csrw	satp, t1

    # We usually don't care about the FP regs,
    # but just in case.
    srli    t4, t4, 13
    andi    t4, t4, 3
    beqz    t4, 1f

    .set i, 0
    .rept 32
        loadfp %i
        .set i, i + 1
    .endr

1:
	.set i, 1
	.rept 31
		loadgp  %i
		.set i, i + 1
	.endr

	sret
.align 4
_spawn_kthread_end:
.size _spawn_kthread, . - _spawn_kthread
.type _spawn_kthread, function

# This must be aligned by 4 since this will go into stvec.
.align 4
_trap:
    csrrw   t6, sscratch, t6

    .set i, 1
    .rept 30
        savegp  %i
    .set i, i + 1
    .endr

    mv      t5, t6
    csrrw   t6, sscratch, t6
    savegp  31, t5

    csrr    t6, sscratch

    csrr    t1, sstatus
    srli    t1, t1, 13
    andi    t1, t1, 3
    beqz    t1, 1f

    .set i, 0
    .rept 32
        savefp %i
        .set i, i + 1
    .endr
1:
    ld      t5, 560(t6)
    ld      sp, 568(t6)
    csrw    satp, t5

    # src/trap.c
    call    os_trap_handler

    # sscratch will be mapped in kernel space
    # at this point.
    csrr    t6, sscratch
    ld      t5, 536(t6)
    csrw    satp, t5

    # Check to see if we need to load the floating point registers
    # This is based on the 2-bit FS field in the sstatus register
    csrr    t5, sstatus
    srli    t5, t5, 13
    andi    t5, t5, 3
    beqz    t5, 1f
    .set i, 0
    .rept 32
        loadfp  %i
    .set i, i + 1
    .endr
1:
    .set i, 1
    .rept 31
        loadgp %i
    .set i, i + 1
    .endr
    
    sret
_trap_end:

.size _trap, . - _trap
.type _trap, function

.section .text
.global process_asm_run
process_asm_run:
    # a0 - Trap frame
    csrw    sscratch, a0
    tail    _spawn_kthread

.size process_asm_run, . - process_asm_run
.type process_asm_run, function
.weak process_asm_run

# The following exports the symbols of the trampoline code (thread and trap)
# You will use these in src/process.c and map these into the process' MMU table.
.section .rodata
.global trampoline_thread_start
trampoline_thread_start: .dword _spawn_kthread
.size trampoline_thread_start, . - trampoline_thread_start
.type trampoline_thread_start, object
.weak trampoline_thread_start

.global trampoline_thread_end
trampoline_thread_end: .dword _spawn_kthread_end
.size trampoline_thread_end, . - trampoline_thread_end
.type trampoline_thread_end, object
.weak trampoline_thread_end

.global trampoline_thread_size
trampoline_thread_size: .dword _spawn_kthread_end - _spawn_kthread
.size trampoline_thread_size, . - trampoline_thread_size
.type trampoline_thread_size, object
.weak trampoline_thread_size

.global trampoline_trap_start
trampoline_trap_start: .dword _trap
.size trampoline_trap_start, . - trampoline_trap_start
.type trampoline_trap_start, object
.weak trampoline_trap_start

.global trampoline_trap_end
trampoline_trap_end: .dword _trap_end
.size trampoline_trap_end, . - trampoline_trap_end
.type trampoline_trap_end, object
.weak trampoline_trap_end

.global trampoline_trap_size
trampoline_trap_size: .dword _trap_end - _trap
.size trampoline_trap_size, . - trampoline_trap_size
.type trampoline_trap_size, object
.weak trampoline_trap_size
