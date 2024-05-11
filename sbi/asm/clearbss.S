# clearbss.S
# Routine to clear the BSS section.
# Called in main.c
.section .text
.global clear_bss
clear_bss:
    la      t0, _bss_start
    la      t1, _bss_end

    # The range of the BSS section is t0 through t1
1:
    # If t0 is >= t1, we're done
    bge     t0, t1, 1f
    # Store doubleword (8 bytes)
    sd      zero, (t0)
    addi    t0, t0, 8
    j       1b
1:

    ret
.size clear_bss, . - clear_bss
.type clear_bss, function
