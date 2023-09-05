#ifndef MINI_PARAMETER_DEFINE_H
#define MINI_PARAMETER_DEFINE_H

extern unsigned long mini_reg_base;

#define RESETCTRL_RESET1                           (0xfe08e000)
#define CLKCTRL_MINI_CLK_CTRL                   ((0x0067  << 2) +  mini_reg_base)/*0xfe000000*/
#define PWRCTRL_MEM_PD12                           ((0x001c  << 2) + (mini_reg_base + 0x0c000))/*0xfe00c000*/
#define PWRCTRL_MEM_PD15                           ((0x001f  << 2) + (mini_reg_base + 0x0c000))/*0xfe00c000*/
#define MINI_TOP_SW_RESET                       (0xfe08e000)

#define P_CLKCTRL_SYS_CLK_CTRL0                    (mini_reg_base + 0x40)
#define CPUCTRL_SYS_CPU_CLK_CTRL                    (mini_reg_base + 0xe144)

#define MINI_SUBSYS_BASE_SECURE      (mini_reg_base)//0xfe380000
#define MINI_SUBSYS_BASE_NON_SECURE  (mini_reg_base)//0xfe380000

#define MINI_SUBSYS_BASE             MINI_SUBSYS_BASE_NON_SECURE

#define MINI_VCMD_OFFSET             (MINI_SUBSYS_BASE+0x0000)
#define MINI_CORE_OFFSET             (MINI_SUBSYS_BASE+0x1000)

// VCMD registers

#define MINI_VCMD_SWREG016           (MINI_VCMD_OFFSET+0x040)
#define MINI_VCMD_SWREG017           (MINI_VCMD_OFFSET+0x044)
#define MINI_VCMD_SWREG018           (MINI_VCMD_OFFSET+0x048)
#define MINI_VCMD_SWREG019           (MINI_VCMD_OFFSET+0x04C)
#define MINI_VCMD_SWREG020           (MINI_VCMD_OFFSET+0x050)
#define MINI_VCMD_SWREG021           (MINI_VCMD_OFFSET+0x054)
#define MINI_VCMD_SWREG022           (MINI_VCMD_OFFSET+0x058)
#define MINI_VCMD_SWREG023           (MINI_VCMD_OFFSET+0x05C)
#define MINI_VCMD_SWREG024           (MINI_VCMD_OFFSET+0x060)
#define MINI_VCMD_SWREG025           (MINI_VCMD_OFFSET+0x064)

#define MINI_CORE_SWREG009           (MINI_CORE_OFFSET+0x024)
#endif
