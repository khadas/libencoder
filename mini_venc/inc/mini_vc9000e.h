#ifndef VC9000E_H
#define VC9000E_H

#define Wr64(addr, data) *(volatile uint64_t *)(addr)=(data)
#define Rd64(addr) *(volatile uint64_t *)(addr)
#define Wr8(addr, data) *(volatile uint8_t *)(addr)=(data)
#define Rd8(addr) *(volatile uint8_t *)(addr)
#define Wr(addr, data) *(volatile uint32_t *)(addr)=(data)
#define Rd(addr) *(volatile uint32_t *)(addr)
#define Wr_reg_bits(reg, val, start, len) \
  Wr(reg, ((Rd(reg) & ~(((1L<<(len))-1)<<(start))) | ((uint32_t)(val & ((1L<<len)-1)) << (start))))

extern void     vc9000e_wr_only_reg (uint32_t addr, uint32_t data);
extern uint32_t vc9000e_rd_reg      (uint32_t addr);
extern void     vc9000e_rd_check_reg(uint32_t addr, uint32_t exp_data,  uint32_t mask);
extern void     vc9000e_wr_reg      (uint32_t addr, uint32_t data);
extern void     vc9000e_poll_reg    (uint32_t addr, uint32_t exp_data,  uint32_t mask);

extern void     vc9000e_reg_set     (uint32_t frm_num);

extern void     vc9000e_set_clk     (void);

#endif  /* VC9000E_H */
