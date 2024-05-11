.section .text
.altmacro

.macro savegp i
    sd  x\i, (8 * \i)(t6)
.endm

.macro loadgp i
    ld  x\i, (8 * \i)(t6)
.endm

.align 4
.global sbi_trap_vector
sbi_trap_vector:
    # Swap t6 with the mscratch register
    # Recall that mscratch contains the trap frame
    # for this HART.
    csrrw   t6, mscratch, t6

    # The old T6 is now in mscratch and T6
    # contains the trap frame address.

    # Save context
    .set i, 1
    .rept 30
        savegp %i
        .set i, i + 1
    .endr

    # We saved X1 - X30, now move the trap
    # frame address to T5.
    mv      t5, t6
    # Swap the trap frame and T6 back to their original
    # positions
    csrrw   t6, mscratch, t6
    # Save t6 into the trap frame
    sd      t6, 8*31(t5)

.option push
.option norelax
    # We need a stack pointer for the trap
    la      sp, _stack_end
    # Set the global pointer
    la      gp, __global_pointer$
.option pop
    # Now that we have multiple HARTs, we need multiple stacks
    csrr    t0, mhartid
    # t0 << 12 is the HART ID x 4096
    slli    t0, t0, 12
    # Move the stack pointer to the correct position for this HART.
    sub     sp, sp, t0

    # Go into C
    call    c_trap_handler

    # We can update the trap frame from C code, so from here,
    # update the registers.
    csrr    t6, mscratch

    # Load all the GP registers back
    .set i, 1
    .rept 31
        loadgp %i
        .set i, i + 1
    .endr

    # Go back!
    mret
.size sbi_trap_vector, . - sbi_trap_vector
.type sbi_trap_vector, function
