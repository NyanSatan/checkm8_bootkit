#ifndef config_H
#define config_H

#include <sys/types.h>

typedef struct {
    uint16_t cpid;
    const char *platform;
    uint32_t loadaddr;
    uint32_t loadsize;
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
} config_t;


static const config_t configs[] = {
    {       
        .cpid = 0x7002,        
        .platform = "s7002",       
        .loadaddr = 0x46018000,        
        .loadsize = 0x30000,
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
    },     

    {       
        .cpid = 0x8002,        
        .platform = "t8002",       
        .loadaddr = 0x48818000,        
        .loadsize = 0x108000,      
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
    },

    {     
        .cpid = 0x8004,        
        .platform = "t8004",       
        .loadaddr = 0x48818000,        
        .loadsize = 0x128000,      
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
};

#endif
