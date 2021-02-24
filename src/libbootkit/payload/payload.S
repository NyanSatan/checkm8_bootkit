.syntax unified
.text

.pool

.global _start
.code 16
_start:
	LDR R4, security_consolidate_environment
	BLX R4

	LDR R4, security_sidp_seal_rom_manifest
	BLX R4

    LDR R4, platform_get_boot_trampoline
	BLX R4

	MOV R5, R0

	MOV R0, #0x0
	LDR R4, platform_bootprep
	BLX R4

	LDR R4, usb_controller_stop
	BLX R4

	LDR R4, timer_stop_all
	BLX R4

    LDR R4, interrupt_mask_all
    BLX R4

	LDR R4, clocks_quiesce
	BLX R4

	LDR R4, enter_critical_section
	BLX R4

	LDR R4, arch_cpu_quiesce
	BLX R4

	LDR R0, LOADADDR
	MOV R1, #0x0
	MOV R2, #0x0
	MOV R3, #0x0

	BLX R5

loop:
	B loop


LOADADDR:
	.long 0xDEAD0001

security_consolidate_environment:
	.long 0xDEAD0002

security_sidp_seal_rom_manifest:
	.long 0xDEAD0003

platform_get_boot_trampoline:
	.long 0xDEAD0004

platform_bootprep:
	.long 0xDEAD0005

usb_controller_stop:
	.long 0xDEAD0006

interrupt_mask_all:
	.long 0xDEAD0007

timer_stop_all:
	.long 0xDEAD0008

clocks_quiesce:
	.long 0xDEAD0009

enter_critical_section:
	.long 0xDEAD000A

arch_cpu_quiesce:
	.long 0xDEAD000B
