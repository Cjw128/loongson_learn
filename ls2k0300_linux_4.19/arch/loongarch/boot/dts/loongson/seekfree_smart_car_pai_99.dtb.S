#include <asm-generic/vmlinux.lds.h>
.section .dtb.init.rodata,"a"
.balign STRUCT_ALIGNMENT
.global __dtb_seekfree_smart_car_pai_99_begin
__dtb_seekfree_smart_car_pai_99_begin:
.incbin "arch/loongarch/boot/dts/loongson/seekfree_smart_car_pai_99.dtb" 
__dtb_seekfree_smart_car_pai_99_end:
.global __dtb_seekfree_smart_car_pai_99_end
.balign STRUCT_ALIGNMENT
