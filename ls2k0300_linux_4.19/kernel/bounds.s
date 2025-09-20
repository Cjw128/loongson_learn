	.file	"bounds.c"
 # GNU C89 (LoongArch GNU toolchain rc1.6 (20241115)) version 8.3.0 (loongarch64-linux-gnu)
 #	compiled by GNU C version 8.5.0, GMP version 6.1.0, MPFR version 3.1.4, MPC version 1.0.3, isl version isl-0.18-GMP

 # GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
 # options passed:  -nostdinc -I ./arch/loongarch/include
 # -I ./arch/loongarch/include/generated -I ./include
 # -I ./arch/loongarch/include/uapi
 # -I ./arch/loongarch/include/generated/uapi -I ./include/uapi
 # -I ./include/generated/uapi -I ./arch/loongarch/include/asm/mach-la64
 # -I ./arch/loongarch/include/asm/mach-generic -imultilib base/lp64s
 # -iprefix /opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin/../lib/gcc/loongarch64-linux-gnu/8.3.0/
 # -isysroot /opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin/../loongarch64-linux-gnu/sysroot
 # -D __KERNEL__ -D CC_USING_PATCHABLE_FUNCTION_ENTRY
 # -D VMLINUX_LOAD_ADDRESS=0x9000000000200000 -D DATAOFFSET=0
 # -D GAS_HAS_SET_HARDFLOAT -U LOONGARCHEB -U _LOONGARCHEB -U __LOONGARCHEB
 # -U __LOONGARCHEB__ -U LOONGARCHEL -U _LOONGARCHEL -U __LOONGARCHEL
 # -U __LOONGARCHEL__ -D LOONGARCHEL -D _LOONGARCHEL -D __LOONGARCHEL
 # -D __LOONGARCHEL__ -U _LOONGARCH_ISA
 # -D _LOONGARCH_ISA=_LOONGARCH_ISA_LOONGARCH64 -D KBUILD_BASENAME="bounds"
 # -D KBUILD_MODNAME="bounds"
 # -isystem /opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin/../lib/gcc/loongarch64-linux-gnu/8.3.0/include
 # -include ./include/linux/kconfig.h
 # -include ./include/linux/compiler_types.h -MD kernel/.bounds.s.d
 # kernel/bounds.c -G 0 -mno-check-zero-division -mstrict-align -mabi=lp64s
 # -march=loongarch64 -mfpu=none -msimd=none -mcmodel=normal
 # -mtune=loongarch64 -auxbase-strip kernel/bounds.s -O2 -Wall -Wundef
 # -Wstrict-prototypes -Wno-trigraphs -Werror=implicit-function-declaration
 # -Werror=return-type -Wno-format-security -Wno-frame-address
 # -Wformat-truncation=0 -Wformat-overflow=0 -Wno-int-in-bool-context
 # -Wframe-larger-than=1024 -Wno-unused-but-set-variable
 # -Wunused-const-variable=0 -Wdeclaration-after-statement
 # -Wno-pointer-sign -Wno-stringop-truncation -Wno-array-bounds
 # -Wstringop-overflow=0 -Wno-restrict -Wno-maybe-uninitialized
 # -Werror=implicit-int -Werror=strict-prototypes -Werror=date-time
 # -Werror=incompatible-pointer-types -Werror=designated-init
 # -Wno-packed-not-aligned -std=gnu90 -fno-strict-aliasing -fno-common
 # -fshort-wchar -fno-PIE -ffreestanding -fno-asynchronous-unwind-tables
 # -fno-delete-null-pointer-checks -fno-stack-protector
 # -fno-var-tracking-assignments -fpatchable-function-entry=2
 # -fno-strict-overflow -fno-merge-all-constants -fmerge-constants
 # -fstack-check=no -fconserve-stack -fmacro-prefix-map=./= -fverbose-asm
 # --param allow-store-data-races=0
 # options enabled:  -faggressive-loop-optimizations -falign-labels
 # -fauto-inc-dec -fbranch-count-reg -fcaller-saves
 # -fchkp-check-incomplete-type -fchkp-check-read -fchkp-check-write
 # -fchkp-instrument-calls -fchkp-narrow-bounds -fchkp-optimize
 # -fchkp-store-bounds -fchkp-use-static-bounds
 # -fchkp-use-static-const-bounds -fchkp-use-wrappers -fcode-hoisting
 # -fcombine-stack-adjustments -fcompare-elim -fcprop-registers
 # -fcrossjumping -fcse-follow-jumps -fdefer-pop -fdevirtualize
 # -fdevirtualize-speculatively -fdwarf2-cfi-asm -fearly-inlining
 # -feliminate-unused-debug-types -fexpensive-optimizations
 # -fforward-propagate -ffp-int-builtin-inexact -ffunction-cse -fgcse
 # -fgcse-lm -fgnu-runtime -fgnu-unique -fguess-branch-probability
 # -fhoist-adjacent-loads -fident -fif-conversion -fif-conversion2
 # -findirect-inlining -finline -finline-atomics
 # -finline-functions-called-once -finline-small-functions -fipa-bit-cp
 # -fipa-cp -fipa-icf -fipa-icf-functions -fipa-icf-variables -fipa-profile
 # -fipa-pure-const -fipa-reference -fipa-sra -fipa-vrp
 # -fira-hoist-pressure -fira-share-save-slots -fira-share-spill-slots
 # -fisolate-erroneous-paths-dereference -fivopts -fkeep-static-consts
 # -fleading-underscore -flifetime-dse -flra-remat -flto-odr-type-merging
 # -fmath-errno -fmerge-constants -fmerge-debug-strings
 # -fmove-loop-invariants -fomit-frame-pointer -foptimize-sibling-calls
 # -foptimize-strlen -fpartial-inlining -fpeephole -fpeephole2 -fplt
 # -fprefetch-loop-arrays -freg-struct-return -freorder-blocks
 # -freorder-functions -frerun-cse-after-loop
 # -fsched-critical-path-heuristic -fsched-dep-count-heuristic
 # -fsched-group-heuristic -fsched-interblock -fsched-last-insn-heuristic
 # -fsched-pressure -fsched-rank-heuristic -fsched-spec
 # -fsched-spec-insn-heuristic -fsched-stalled-insns-dep -fschedule-fusion
 # -fschedule-insns -fschedule-insns2 -fsemantic-interposition
 # -fshow-column -fshrink-wrap -fshrink-wrap-separate -fsigned-zeros
 # -fsplit-ivs-in-unroller -fsplit-wide-types -fssa-backprop -fssa-phiopt
 # -fstdarg-opt -fstore-merging -fstrict-volatile-bitfields -fsync-libcalls
 # -fthread-jumps -ftoplevel-reorder -ftrapping-math -ftree-bit-ccp
 # -ftree-builtin-call-dce -ftree-ccp -ftree-ch -ftree-coalesce-vars
 # -ftree-copy-prop -ftree-cselim -ftree-dce -ftree-dominator-opts
 # -ftree-dse -ftree-forwprop -ftree-fre -ftree-loop-if-convert
 # -ftree-loop-im -ftree-loop-ivcanon -ftree-loop-optimize
 # -ftree-parallelize-loops= -ftree-phiprop -ftree-pre -ftree-pta
 # -ftree-reassoc -ftree-scev-cprop -ftree-sink -ftree-slsr -ftree-sra
 # -ftree-switch-conversion -ftree-tail-merge -ftree-ter -ftree-vrp
 # -funit-at-a-time -fverbose-asm -fwrapv -fwrapv-pointer
 # -fzero-initialized-in-bss -mglibc -mvecarg

	.text
#APP
	.macro	parse_r var r, imm
	\var	= -1
	.ifc	\r, $r0
	\var	= 0
	.endif
	.ifc	\r, $r1
	\var	= 1
	.endif
	.ifc	\r, $r2
	\var	= 2
	.endif
	.ifc	\r, $r3
	\var	= 3
	.endif
	.ifc	\r, $r4
	\var	= 4
	.endif
	.ifc	\r, $r5
	\var	= 5
	.endif
	.ifc	\r, $r6
	\var	= 6
	.endif
	.ifc	\r, $r7
	\var	= 7
	.endif
	.ifc	\r, $r8
	\var	= 8
	.endif
	.ifc	\r, $r9
	\var	= 9
	.endif
	.ifc	\r, $r10
	\var	= 10
	.endif
	.ifc	\r, $r11
	\var	= 11
	.endif
	.ifc	\r, $r12
	\var	= 12
	.endif
	.ifc	\r, $r13
	\var	= 13
	.endif
	.ifc	\r, $r14
	\var	= 14
	.endif
	.ifc	\r, $r15
	\var	= 15
	.endif
	.ifc	\r, $r16
	\var	= 16
	.endif
	.ifc	\r, $r17
	\var	= 17
	.endif
	.ifc	\r, $r18
	\var	= 18
	.endif
	.ifc	\r, $r19
	\var	= 19
	.endif
	.ifc	\r, $r20
	\var	= 20
	.endif
	.ifc	\r, $r21
	\var	= 21
	.endif
	.ifc	\r, $r22
	\var	= 22
	.endif
	.ifc	\r, $r23
	\var	= 23
	.endif
	.ifc	\r, $r24
	\var	= 24
	.endif
	.ifc	\r, $r25
	\var	= 25
	.endif
	.ifc	\r, $r26
	\var	= 26
	.endif
	.ifc	\r, $r27
	\var	= 27
	.endif
	.ifc	\r, $r28
	\var	= 28
	.endif
	.ifc	\r, $r29
	\var	= 29
	.endif
	.ifc	\r, $r30
	\var	= 30
	.endif
	.ifc	\r, $r31
	\var	= 31
	.endif
	.iflt	\var
	.error	"Unable to parse register name \r"
	.endif
	.endm
#NO_APP
	.section	.text.startup,"ax",@progbits
	.align	2
	.align	4
	.globl	main
	.type	main, @function
main:
	.section	__patchable_function_entries,"a",@progbits
	.8byte	.LPFE1
	.section	.text.startup
.LPFE1:
	nop	
	nop	
 # kernel/bounds.c:19: 	DEFINE(NR_PAGEFLAGS, __NR_PAGEFLAGS);
#APP
 # 19 "kernel/bounds.c" 1
	
.ascii "->NR_PAGEFLAGS 21 __NR_PAGEFLAGS"	 #
 # 0 "" 2
 # kernel/bounds.c:20: 	DEFINE(MAX_NR_ZONES, __MAX_NR_ZONES);
 # 20 "kernel/bounds.c" 1
	
.ascii "->MAX_NR_ZONES 3 __MAX_NR_ZONES"	 #
 # 0 "" 2
 # kernel/bounds.c:22: 	DEFINE(NR_CPUS_BITS, ilog2(CONFIG_NR_CPUS));
 # 22 "kernel/bounds.c" 1
	
.ascii "->NR_CPUS_BITS 1 ilog2(CONFIG_NR_CPUS)"	 #
 # 0 "" 2
 # kernel/bounds.c:24: 	DEFINE(SPINLOCK_SIZE, sizeof(spinlock_t));
 # 24 "kernel/bounds.c" 1
	
.ascii "->SPINLOCK_SIZE 4 sizeof(spinlock_t)"	 #
 # 0 "" 2
 # kernel/bounds.c:28: }
#NO_APP
	or	$r4,$r0,$r0	 #,
	jr	$r1		 #
	.size	main, .-main
	.ident	"GCC: (LoongArch GNU toolchain rc1.6 (20241115)) 8.3.0"
	.section	.note.GNU-stack,"",@progbits
