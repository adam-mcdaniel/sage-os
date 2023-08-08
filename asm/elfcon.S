
# Use this to test your ELF reader. This requires that you
# compile the user library before linking.

# After you get this tested, then use your block driver to
# read this from the hard drive.
.section .rodata
elfcon_prog:
    # .incbin "user/console.elf"
.type elfcon_prog, function
.size elfcon_prog, .-elfcon_prog

.global elfcon
elfcon: .dword elfcon_prog
.type elfcon, object
.size elfcon, .-elfcon
