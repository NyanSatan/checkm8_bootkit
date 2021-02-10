#ifndef config_H
#define config_H

#include <sys/types.h>

typedef struct {
    uint16_t cpid;
    const char *platform;
    uint32_t loadaddr;
    uint32_t loadsize;
    uint32_t memmove;
    uint32_t platform_get_boot_trampoline;
    uint32_t platform_bootprep;
    uint32_t usb_quiesce_no_free;
    uint32_t timer_stop_all;
    uint32_t interrupt_mask_all;
    uint32_t clocks_quiesce;
    uint32_t enter_critical_section;
    uint32_t arch_cpu_quiesce;
} config_t;

/*
static const config_t s5l8942x_config = {
    .loadaddr = 0x34000000,
    .loadsize = 0x2C000,
    .memmove = 0x980C,
    .platform_get_boot_trampoline = 0x6F04 + 1,
    .prepare_and_jump = 0x60A0 + 1
};
*/

static const config_t configs[] = {
    {
        .cpid = 0x8950,
        .platform = "s5l8950x",
        .loadaddr = 0x10000000,
        .loadsize = 0x60000,
        .memmove = 0x9ACC,
        .platform_get_boot_trampoline = 0x6E84 + 1,
        .platform_bootprep = 0x562C + 1,
        .usb_quiesce_no_free = 0x381C + 1,
        .interrupt_mask_all = 0xDC4 + 1,
        .timer_stop_all = 0xAF40 + 1, //nullsub
        .clocks_quiesce = 0x5C38 + 1,
        .enter_critical_section = 0x6340 + 1,
        .arch_cpu_quiesce = 0x6ECC + 1
    },

    {
        .cpid = 0x8747,
        .platform = "s5l8747x",
        .loadaddr = 0x22000000,
        .loadsize = 0x19000,
        .memmove = 0x8F50,
        .platform_get_boot_trampoline = 0x6164 + 1,
        .platform_bootprep = 0x4A0C + 1,
        .usb_quiesce_no_free = 0x2E50 + 1,
        .interrupt_mask_all = 0xC50 + 1,
        .timer_stop_all = 0x2594 + 1,
        .clocks_quiesce = 0x4F28 + 1,
        .enter_critical_section = 0x5578 + 1,
        .arch_cpu_quiesce = 0x6134 + 1
    },

    {
        .cpid = 0x8947,
        .platform = "s5l8947x",
        .loadaddr = 0x34000000,
        .loadsize = 0x2C000,
        .memmove = 0x9A3C,
        .platform_get_boot_trampoline = 0x6C74 + 1,
        .platform_bootprep = 0x4EB0 + 1,
        .usb_quiesce_no_free = 0x324C + 1,
        .interrupt_mask_all = 0xD88 + 1,
        .timer_stop_all = 0xA338 + 1, //nullsub
        .clocks_quiesce = 0x5A88 + 1,
        .enter_critical_section = 0x6054 + 1,
        .arch_cpu_quiesce = 0x6C44 + 1
    },

    {
        .cpid = 0x7002,
        .platform = "s7002",
        .loadaddr = 0x46018000,
        .loadsize = 0x30000,
        .memmove = 0x89F4,
        .platform_get_boot_trampoline = 0x46D8 + 1,
        .platform_bootprep = 0x42F4 + 1,
        .usb_quiesce_no_free = 0x26FC + 1,
        .timer_stop_all = 0xD98 + 1,
        .interrupt_mask_all = 0x4C8C + 1,
        .clocks_quiesce = 0x4C0C + 1,
        .enter_critical_section = 0x50B0 + 1,
        .arch_cpu_quiesce = 0x5F24 + 1
    },

    {
        .cpid = 0x8002,
        .platform = "t8002",
        .loadaddr = 0x48818000,
        .loadsize = 0x108000,
        .memmove = 0xB6F8,
        .platform_get_boot_trampoline = 0x6774 + 1,
        .platform_bootprep = 0x6260 + 1,
        .usb_quiesce_no_free = 0x8A88 + 1,
        .interrupt_mask_all = 0x1410 + 1,
        .timer_stop_all = 0x124C + 1,
        .clocks_quiesce = 0x6DE4 + 1,
        .enter_critical_section = 0x734C + 1,
        .arch_cpu_quiesce = 0x8328 + 1
    },

    {
        .cpid = 0x8004,
        .platform = "t8004",
        .loadaddr = 0x48818000,
        .loadsize = 0x128000,
        .memmove = 0xA884,
        .platform_get_boot_trampoline = 0x5864 + 1,
        .platform_bootprep = 0x52A0 + 1,
        .usb_quiesce_no_free = 0x7C18 + 1,
        .interrupt_mask_all = 0x1414 + 1,
        .timer_stop_all = 0x1250 + 1,
        .clocks_quiesce = 0x5F24 + 1,
        .enter_critical_section = 0x648C + 1,
        .arch_cpu_quiesce = 0x74B8 + 1
    }

};

#endif
