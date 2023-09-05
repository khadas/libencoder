
#ifndef MEMALLOC_H
#define MEMALLOC_H



typedef struct MemallocParams_t{
    unsigned int bus_address;
    unsigned int size;
    unsigned int translation_offset;
    unsigned int mem_type;
} MemallocParams;


#define MEMALLOC_IOC_MAXNR 15



int memalloc_release(int  filp);


void memalloc_cleanup(  );





int mini_memalloc_init(unsigned int  phy_mem_address,unsigned int phy_mem_size);


/* Cycle through the buffers we have, give the first free one */
int AllocMemory(unsigned int *busaddr, unsigned int size, const int  filp);

/* Free a buffer based on bus address */
int FreeMemory(unsigned int busaddr, const int filp);

/* Reset "used" status */
void mini_ResetMems(void);

int encmem_vce_probe(unsigned int  phy_mem_address,unsigned int phy_mem_size);

int encmem_vce_remove();






#endif /* MEMALLOC_H */
