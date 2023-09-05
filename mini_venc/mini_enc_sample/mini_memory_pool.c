/***************************************************************************************************************
*
*  Filename: mini_memory_pool.c
*  -----------------------------------------------------------------------------
*  Description:
*  This source file primarily encapsulates the functionality of memory pool management.
*  The system necessitates the allotment of a contiguous range of physical addresses, conforming to the memory requisites of the entire mini system.
*  This allocation of physical memory is managed in 4096-byte units by a dedicated memory management system.
*  This framework thereby enables the allocation and de-allocation of memory, specifically tailored for the encoder.
*
*  -----------------------------------------------------------------------------
*  Author: Yang.su (yang.su@amlogic.com)
*  Company: Amlogic
*  Copyright: (C) 2023 Amlogic ALL RIGHTS RESERVED
*
*  -----------------------------------------------------------------------------
*  Revision History:
*      [1.0]     - 2023-05-01      - suyang       - Initial creation of file.
*
*  -----------------------------------------------------------------------------
*
****************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "mini_memory_pool.h"
#ifndef HLINA_START_ADDRESS
#define HLINA_START_ADDRESS 0x02000000
#endif

#ifndef HLINA_SIZE
#define HLINA_SIZE 96
#endif

#ifndef HLINA_TRANSL_OFFSET
#define HLINA_TRANSL_OFFSET 0x0
#endif


typedef struct hlinc {
    unsigned long bus_address;
    unsigned short chunks_used;
    unsigned int filp;
} hlina_chunk;



#define CHUNK_SIZE (4096)
#define CLASS_NAME "vencmem"
#define DEVICE_NAME "memalloc"

static struct device*  device;
static unsigned int alloc_size = 32;
static unsigned int alloc_base = HLINA_START_ADDRESS;
static unsigned long addr_transl = HLINA_TRANSL_OFFSET;
static int memalloc_major; /* dynamic */

static hlina_chunk *hlina_chunks;
static unsigned  int  chunks;

int AllocMemory(unsigned int *busaddr, unsigned int size, int  filp);
int FreeMemory(unsigned int busaddr, int filp);
void mini_ResetMems(void);



int memalloc_release(int  filp)
{
    int i = 0;

    for (i = 0; i < chunks; i++) {
       // spin_lock(&mem_lock);
        if (hlina_chunks[i].filp == filp) {

            hlina_chunks[i].filp =  0;
            hlina_chunks[i].chunks_used = 0;
        }
        ///spin_unlock(&mem_lock);
    }

    return 0;
}




static int unsigned paddr = 0;
static void *vaddr =  ((void *)0);

void memalloc_cleanup(  )
{
    if (hlina_chunks)
        free(hlina_chunks);

     //dma_free_coherent(&pf_dev->dev, alloc_size * SZ_1M, vaddr, paddr);

	return;
}





int mini_memalloc_init(unsigned int  phy_mem_address,unsigned int phy_mem_size)
{
    int result;
    int ret = 0;


	if (phy_mem_address == 0 || phy_mem_size == 0)
	{
		return -1;
	}


    alloc_base = phy_mem_address;

    chunks = (alloc_size * 1024 * 1024) / CHUNK_SIZE;

    hlina_chunks = malloc(chunks * sizeof(hlina_chunk));
	if (!hlina_chunks) {
		result = -1;
        goto err;
    }

    mini_ResetMems();

    return 0;

err:
    if (hlina_chunks)
        free(hlina_chunks);

    return result;
}


/* Cycle through the buffers we have, give the first free one */
int AllocMemory(unsigned int *busaddr, unsigned int size, const int  filp)
{
    int i = 0;
    int j = 0;
    unsigned int skip_chunks = 0;

    /* calculate how many chunks we need; round up to chunk boundary*/
    unsigned int alloc_chunks = (size + CHUNK_SIZE - 1) / CHUNK_SIZE;

    *busaddr = 0;

    /* run through the chunk table */
    for (i = 0; i < chunks;)
	{
        skip_chunks = 0;
        /* if this chunk is available */
        if (!hlina_chunks[i].chunks_used)
		{
            /* check that there is enough memory left */
            if (i + alloc_chunks > chunks)
			{
				break;
			}

            /* check that there is enough consecutive chunks available */
            for (j = i; j < i + alloc_chunks; j++)
			{
                if (hlina_chunks[j].chunks_used)
				{
                    skip_chunks = 1;
                    /* skip the used chunks */
                    i = j + hlina_chunks[j].chunks_used;
                    break;
                }
            }

            /* if enough free memory found */
            if (!skip_chunks) {
                *busaddr = hlina_chunks[i].bus_address;
                hlina_chunks[i].filp = filp;
                hlina_chunks[i].chunks_used = alloc_chunks;
                break;
            }
        }
		else
		{
            /* skip the used chunks */
            i += hlina_chunks[i].chunks_used;
        }
    }

    if (*busaddr == 0)
	{
        return -1;
    }

    return 0;
}

/* Free a buffer based on bus address */
int FreeMemory(unsigned int busaddr, const int filp)
{
    int i = 0;

    for (i = 0; i < chunks; i++) {
        /* user space SW has stored the translated bus address, add addr_transl to
		 * translate back to our address space
		 */
        if (hlina_chunks[i].bus_address == busaddr + addr_transl) {
            if (hlina_chunks[i].filp == filp) {
                hlina_chunks[i].filp =  0;
                hlina_chunks[i].chunks_used = 0;
            } else {
                ;
            }
            break;
        }
    }
    return 0;
}

/* Reset "used" status */
void mini_ResetMems(void)
{
    int i = 0;
    unsigned int ba = alloc_base;
    for (i = 0; i < chunks; i++) {
        hlina_chunks[i].bus_address = ba;
        hlina_chunks[i].filp = 0;
        hlina_chunks[i].chunks_used = 0;
        ba += CHUNK_SIZE;
    }
}

int encmem_vce_probe(unsigned int  phy_mem_address,unsigned int phy_mem_size)
{
    mini_memalloc_init(phy_mem_address,phy_mem_size);
    return 0;
}

int encmem_vce_remove( )
{
    memalloc_cleanup();
    return 0;
}
