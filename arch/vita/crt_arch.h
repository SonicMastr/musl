__asm__(
".text \n"
".global " START " \n"
".type " START ",%function \n"
START ": \n"
"	mov fp, #0 \n"
"	mov lr, #0 \n"
"	ldr a3, 1f \n" // 1 Forward
"	add a3, pc, a3 \n"
"	mov a4, sp \n"
"2:	and ip, a4, #-16 \n"
"	mov sp, ip \n"
"	bl " START "_vita_c \n"
".weak _DYNAMIC \n"
".hidden _DYNAMIC \n"
".align 2 \n"
"1:	.word _DYNAMIC-2b \n"
);
