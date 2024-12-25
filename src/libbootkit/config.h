#ifndef config_H
#define config_H

#include <sys/types.h>

typedef struct {
    uint32_t memmove;
    uint32_t platform_get_boot_trampoline;
    uint32_t platform_bootprep;
    uint32_t usb_quiesce_no_free;
    uint32_t timer_stop_all;
    uint32_t interrupt_mask_all;
    uint32_t clocks_quiesce;
    uint32_t enter_critical_section;
    uint32_t arch_cpu_quiesce;
    uint32_t prepare_and_jump;
} boot_config_t;

typedef struct {
    uint32_t func_ptr;
    uint32_t security_consolidate_environment;
    uint32_t security_sidp_seal_rom_manifest;
    uint32_t platform_get_boot_trampoline;
    uint32_t platform_bootprep;
    uint32_t usb_controller_stop;
    uint32_t timer_stop_all;
    uint32_t interrupt_mask_all;
    uint32_t clocks_quiesce;
    uint32_t enter_critical_section;
    uint32_t arch_cpu_quiesce;
} boot_config_watch_t;

typedef struct {
    uint16_t cpid;
    const char *platform;
    uint32_t loadaddr;
    uint32_t loadsize;
    uint32_t demotion_reg;
    uint32_t aes_crypto_cmd;
    bool simple;
    bool watch;
    union {
        const boot_config_t boot_config;
        const boot_config_watch_t boot_config_watch;
    };
} rom_config_t;

static const rom_config_t rom_configs[] = {
    {
        .cpid = 0x8747,
        .platform = "s5l8747x",
        .loadaddr = 0x22000000,
        .loadsize = 0x19000,
        .aes_crypto_cmd = 0x6540+1,
        .boot_config = {
            .memmove = 0x8F50,
            .platform_get_boot_trampoline = 0x6164 + 1,
            .platform_bootprep = 0x4A0C + 1,
            .usb_quiesce_no_free = 0x2E50 + 1,
            .interrupt_mask_all = 0xC50 + 1,
            .timer_stop_all = 0x2594 + 1,
            .clocks_quiesce = 0x4F28 + 1,
            .enter_critical_section = 0x5578 + 1,
            .arch_cpu_quiesce = 0x6134 + 1
        }
    },
    {
        .cpid = 0x8942,
        .platform = "s5l8942x",
        .loadaddr = 0x34000000,
        .loadsize = 0x2C000,
        .demotion_reg = 0x3F500000,
        .aes_crypto_cmd = 0x7300 + 1,
        .boot_config = {
            .memmove = 0x980C,
            .platform_get_boot_trampoline = 0x6F04 + 1,
            .prepare_and_jump = 0x60A0 + 1
        }
    },
    {
        .cpid = 0x8947,
        .platform = "s5l8947x",
        .loadaddr = 0x34000000,
        .loadsize = 0x2C000,
        .demotion_reg = 0x3F500000,
        .aes_crypto_cmd = 0x7060+1,
        .boot_config = {
            .memmove = 0x9A3C,
            .platform_get_boot_trampoline = 0x6C74 + 1,
            .platform_bootprep = 0x4EB0 + 1,
            .usb_quiesce_no_free = 0x324C + 1,
            .interrupt_mask_all = 0xD88 + 1,
            .timer_stop_all = 0xA338 + 1, //nullsub
            .clocks_quiesce = 0x5A88 + 1,
            .enter_critical_section = 0x6054 + 1,
            .arch_cpu_quiesce = 0x6C44 + 1
        }
    },
    {
        .cpid = 0x8950,
        .platform = "s5l8950x",
        .loadaddr = 0x10000000,
        .loadsize = 0x60000,
        .demotion_reg = 0x3F500000,
        .aes_crypto_cmd = 0x7300+1,
        .boot_config = {
            .memmove = 0x9ACC,
            .platform_get_boot_trampoline = 0x6E84 + 1,
            .platform_bootprep = 0x562C + 1,
            .usb_quiesce_no_free = 0x381C + 1,
            .interrupt_mask_all = 0xDC4 + 1,
            .timer_stop_all = 0xAF40 + 1, //nullsub
            .clocks_quiesce = 0x5C38 + 1,
            .enter_critical_section = 0x6340 + 1,
            .arch_cpu_quiesce = 0x6ECC + 1
        }
    },
    {       
        .cpid = 0x7002,        
        .platform = "s7002",       
        .loadaddr = 0x46018000,        
        .loadsize = 0x30000,
        .watch = true,
        .boot_config_watch = {    
            .func_ptr = 0x4600FFA4,
            .security_consolidate_environment = 0x59DC + 1,
            .security_sidp_seal_rom_manifest = 0x2218 + 1,  //nullsub
            .platform_get_boot_trampoline = 0x46D8 + 1,        
            .platform_bootprep = 0x42F4 + 1,       
            .usb_controller_stop = 0x26FC + 1,
            .timer_stop_all = 0xD98 + 1,       
            .interrupt_mask_all = 0x4C8C + 1,      
            .clocks_quiesce = 0x4C0C + 1,      
            .enter_critical_section = 0x50B0 + 1,      
            .arch_cpu_quiesce = 0x5F24 + 1
        }  
    },     
    {       
        .cpid = 0x8002,        
        .platform = "t8002",       
        .loadaddr = 0x48818000,        
        .loadsize = 0x108000,
        .watch = true,
        .boot_config_watch = {    
            .func_ptr = 0x48806148,
            .security_consolidate_environment = 0x7DF4 + 1,
            .security_sidp_seal_rom_manifest = 0x3444 + 1,  //nullsub   
            .platform_get_boot_trampoline = 0x6774 + 1,
            .platform_bootprep = 0x6260 + 1,       
            .usb_controller_stop = 0x3444 + 1,  //nullsub      
            .interrupt_mask_all = 0x1410 + 1,      
            .timer_stop_all = 0x124C + 1,      
            .clocks_quiesce = 0x6DE4 + 1,      
            .enter_critical_section = 0x734C + 1,      
            .arch_cpu_quiesce = 0x8328 + 1
        }
    },
    {     
        .cpid = 0x8004,        
        .platform = "t8004",       
        .loadaddr = 0x48818000,        
        .loadsize = 0x128000,
        .watch = true,
        .boot_config_watch = {    
            .func_ptr = 0x48806178,
            .security_consolidate_environment = 0x6F34 + 1,
            .security_sidp_seal_rom_manifest = 0x6F94 + 1, 
            .platform_get_boot_trampoline = 0x5864 + 1,        
            .platform_bootprep = 0x52A0 + 1,       
            .usb_controller_stop = 0x6264 + 1,  //nullsub
            .interrupt_mask_all = 0x1414 + 1,      
            .timer_stop_all = 0x1250 + 1,      
            .clocks_quiesce = 0x5F24 + 1,      
            .enter_critical_section = 0x648C + 1,      
            .arch_cpu_quiesce = 0x74B8 + 1
        }
    }
};

static const rom_config_t *get_config(uint32_t cpid) {
    for (int i = 0; i < sizeof(rom_configs) / sizeof(rom_config_t); i++) {
        const rom_config_t *config = &rom_configs[i];
        if (config->cpid == cpid) {
            return config;
        }
    }

    return NULL;
}

#endif
