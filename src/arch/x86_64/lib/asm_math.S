/*
   Copyright 2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

.text

.global asm_sqrt
.type asm_sqrt,@function
asm_sqrt:
    sqrtsd %xmm0,%xmm0
    ret
