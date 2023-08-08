# termfont.S
# Terminal font direct attachment
# Stephen Marz
# 19 May 2022
# COSC562 Operating Systems: Design and Implementation

# The terminal font is laid out 8 columns by 16 rows for
# every number and letter.

.section .rodata
.global termfont
termfont: .incbin "asm/terminal.fnt"
.type termfont, object
.size termfont, . - termfont
