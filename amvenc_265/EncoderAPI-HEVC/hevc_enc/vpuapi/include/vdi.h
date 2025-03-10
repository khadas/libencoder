//------------------------------------------------------------------------------
// File: vdi.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef _VDI_H_
#define _VDI_H_

#include "vpuconfig.h"
#include "vputypes.h"



/************************************************************************/
/* COMMON REGISTERS                                                     */
/************************************************************************/
#define VPU_PRODUCT_NAME_REGISTER                 0x1040
#define VPU_PRODUCT_CODE_REGISTER                 0x1044

#define SUPPORT_MULTI_CORE_IN_ONE_DRIVER
#define MAX_VPU_CORE_NUM MAX_NUM_VPU_CORE
#ifndef __ANDROID__
/*
 * Fixed for glibc2.33
 * error: The futex facility returned an unexpected error code.
 *
 */
#define AML_FIXED_FOR_GLIBC_2_33
#define VPU_SHARED_FILE_NAME "/tmp/vpu_mutex.map"
#define VPU_SHARED_FILE_SIZE 4096
#endif

#ifdef SUPPORT_SRC_BUF_CONTROL
#define MAX_VPU_BUFFER_POOL 2000
#else
#define MAX_VPU_BUFFER_POOL (64*MAX_NUM_INSTANCE+12*3)//+12*3 => mvCol + YOfsTable + COfsTable
#endif
#define WAVE420L_TEMP_BUF_SIZE          0x00A00000

#define VpuWriteReg( CORE, ADDR, DATA )                 vdi_write_register( CORE, ADDR, DATA )					// system register write
#define VpuReadReg( CORE, ADDR )                        vdi_read_register( CORE, ADDR )							// system register read
#define VpuWriteMem( CORE, ADDR, DATA, LEN, ENDIAN )    vdi_write_memory( CORE, ADDR, DATA, LEN, ENDIAN )		// system memory write
#define VpuReadMem( CORE, ADDR, DATA, LEN, ENDIAN )     vdi_read_memory( CORE, ADDR, DATA, LEN, ENDIAN )		// system memory read

typedef Uint32 u32;
typedef Int32 s32;
typedef unsigned long ulong;
typedef Int16 s16;
typedef Uint16 u16;
typedef Uint8 u8;
typedef Int8 s8;

typedef struct vpu_bit_firmware_info_t {
    u32 size; /* size of this structure*/
    u32 core_idx;
    u32 reg_base_offset;
    u16 bit_code[512];
} vpu_bit_firmware_info_t;

typedef struct vpudrv_intr_info_t {
	u32 timeout;
	s32 intr_reason;
} vpudrv_intr_info_t;

typedef struct vpudrv_inst_info_t {
    u32 core_idx;
    u32 inst_idx;
    s32 inst_open_count; /* for output only*/
} vpudrv_inst_info_t;

typedef struct vpu_buffer_t {
    u32 size;
    u32 cached;
    PhysicalAddress phys_addr;
    ulong base;
    ulong virt_addr;
} vpu_buffer_t;

//typedef struct vpu_dma_buf_info_t {
//    u32 num_planes;
//    int fd[3];
//    ulong phys_addr[3]; /* phys address for DMA buffer */
//} vpu_dma_buf_info_t;

typedef struct vpu_multi_dma_buf_info_t {
    u32 num_planes;
    s32 fd[3];
    ulong phys_addr[3]; /* phys address for DMA buffer */
} vpu_multi_dma_buf_info_t;

#define VPUDRV_BUF_LEN struct vpu_buffer_t
#define VPUDRV_INST_LEN struct vpudrv_inst_info_t
#define VPUDRV_DMABUF_MULTI_LEN struct vpu_multi_dma_buf_info_t

#define VDI_MAGIC  'V'
#define VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY \
	_IOW(VDI_MAGIC, 0, VPUDRV_BUF_LEN)

#define VDI_IOCTL_FREE_PHYSICALMEMORY \
	_IOW(VDI_MAGIC, 1, VPUDRV_BUF_LEN)

#define VDI_IOCTL_WAIT_INTERRUPT \
          _IOW(VDI_MAGIC, 2, vpudrv_intr_info_t)

#define VDI_IOCTL_SET_CLOCK_GATE \
	_IOW(VDI_MAGIC, 3, u32)

#define VDI_IOCTL_RESET \
	_IOW(VDI_MAGIC, 4, u32)

#define VDI_IOCTL_GET_INSTANCE_POOL \
	_IOW(VDI_MAGIC, 5, VPUDRV_BUF_LEN)

#define VDI_IOCTL_GET_COMMON_MEMORY \
	_IOW(VDI_MAGIC, 6, VPUDRV_BUF_LEN)

#define VDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO \
	_IOW(VDI_MAGIC, 8, VPUDRV_BUF_LEN)

#define VDI_IOCTL_OPEN_INSTANCE \
	_IOW(VDI_MAGIC, 9, VPUDRV_INST_LEN)

#define VDI_IOCTL_CLOSE_INSTANCE \
	_IOW(VDI_MAGIC, 10, VPUDRV_INST_LEN)

#define VDI_IOCTL_GET_INSTANCE_NUM \
	_IOW(VDI_MAGIC, 11, VPUDRV_INST_LEN)

#define VDI_IOCTL_GET_REGISTER_INFO \
	_IOW(VDI_MAGIC, 12, VPUDRV_BUF_LEN)

#define VDI_IOCTL_FLUSH_BUFFER \
	_IOW(VDI_MAGIC, 13, VPUDRV_BUF_LEN)

//#define VDI_IOCTL_CONFIG_DMA \
//        _IOW(VDI_MAGIC, 14, struct vpu_dma_buf_info_t)

//#define VDI_IOCTL_UNMAP_DMA \
//        _IOW(VDI_MAGIC, 15, u32)

#define VDI_IOCTL_CONFIG_MULTI_DMA \
            _IOW(VDI_MAGIC, 16, VPUDRV_DMABUF_MULTI_LEN)

#define VDI_IOCTL_UNMAP_MULTI_DMA \
            _IOW(VDI_MAGIC, 17, VPUDRV_DMABUF_MULTI_LEN)


typedef enum {
    VDI_LITTLE_ENDIAN = 0,      /* 64bit LE */
    VDI_BIG_ENDIAN,             /* 64bit BE */
    VDI_32BIT_LITTLE_ENDIAN,
    VDI_32BIT_BIG_ENDIAN,
    /* WAVE PRODUCTS */
    VDI_128BIT_LITTLE_ENDIAN    = 16,
    VDI_128BIT_LE_BYTE_SWAP,
    VDI_128BIT_LE_WORD_SWAP,
    VDI_128BIT_LE_WORD_BYTE_SWAP,
    VDI_128BIT_LE_DWORD_SWAP,
    VDI_128BIT_LE_DWORD_BYTE_SWAP,
    VDI_128BIT_LE_DWORD_WORD_SWAP,
    VDI_128BIT_LE_DWORD_WORD_BYTE_SWAP,
    VDI_128BIT_BE_DWORD_WORD_BYTE_SWAP,
    VDI_128BIT_BE_DWORD_WORD_SWAP,
    VDI_128BIT_BE_DWORD_BYTE_SWAP,
    VDI_128BIT_BE_DWORD_SWAP,
    VDI_128BIT_BE_WORD_BYTE_SWAP,
    VDI_128BIT_BE_WORD_SWAP,
    VDI_128BIT_BE_BYTE_SWAP,
    VDI_128BIT_BIG_ENDIAN        = 31,
    VDI_ENDIAN_MAX
} EndianMode;


#define VDI_128BIT_ENDIAN_MASK      0xf

typedef struct vpu_pending_intr_t {
    int instance_id[COMMAND_QUEUE_DEPTH];
    int int_reason[COMMAND_QUEUE_DEPTH];
    int order_num[COMMAND_QUEUE_DEPTH];
    int in_use[COMMAND_QUEUE_DEPTH];
    int num_pending_intr;
    int count;
} vpu_pending_intr_t;

typedef enum {
    VDI_LINEAR_FRAME_MAP  = 0,
    VDI_TILED_FRAME_V_MAP = 1,
    VDI_TILED_FRAME_H_MAP = 2,
    VDI_TILED_FIELD_V_MAP = 3,
    VDI_TILED_MIXED_V_MAP = 4,
    VDI_TILED_FRAME_MB_RASTER_MAP = 5,
    VDI_TILED_FIELD_MB_RASTER_MAP = 6,
    VDI_TILED_FRAME_NO_BANK_MAP = 7,
    VDI_TILED_FIELD_NO_BANK_MAP = 8,
    VDI_LINEAR_FIELD_MAP  = 9,
    VDI_TILED_MAP_TYPE_MAX
} vdi_gdi_tiled_map;

typedef struct vpu_instance_pool_t {
    u8 codecInstPool[MAX_NUM_INSTANCE][MAX_INST_HANDLE_SIZE];
    vpu_buffer_t    vpu_common_buffer;
    u32 vpu_instance_num;
    u8 instance_pool_inited;
    void*           pendingInst;
    u32 pendingInstIdxPlus1;
//    video_mm_t      vmem;
    vpu_pending_intr_t pending_intr_list;
} vpu_instance_pool_t;


#if defined (__cplusplus)
extern "C" {
#endif
	int vdi_probe(u32 core_idx);
	int vdi_init(u32 core_idx);
	int vdi_release(u32 core_idx);	//this function may be called only at system off.

	vpu_instance_pool_t * vdi_get_instance_pool(u32 core_idx);
    int vdi_allocate_common_memory(u32 core_idx);
	int vdi_get_common_memory(u32 core_idx, vpu_buffer_t *vb);
	int vdi_allocate_dma_memory(u32 core_idx, vpu_buffer_t *vb);
	int vdi_attach_dma_memory(u32 core_idx, vpu_buffer_t *vb);
	void vdi_free_dma_memory(u32 core_idx, vpu_buffer_t *vb);
	int vdi_get_sram_memory(u32 core_idx, vpu_buffer_t *vb);
	int vdi_dettach_dma_memory(u32 core_idx, vpu_buffer_t *vb);
    int vdi_flush_memory(u32 core_idx, vpu_buffer_t *vb);
    int vdi_config_dma(u32 core_idx, vpu_multi_dma_buf_info_t *info);
    int vdi_unmap_dma(u32 core_idx, vpu_multi_dma_buf_info_t *info);

	int vdi_wait_interrupt(u32 core_idx, int timeout, unsigned int addr_bit_int_reason);
	int vdi_wait_vpu_busy(u32 core_idx, int timeout, unsigned int addr_bit_busy_flag);
	int vdi_wait_bus_busy(u32 core_idx, int timeout, unsigned int gdi_busy_flag);
	int vdi_hw_reset(u32 core_idx);

	int vdi_set_clock_gate(u32 core_idx, int enable);
	int vdi_get_clock_gate(u32 core_idx);
    /**
     * @brief       make clock stable before changing clock frequency
     * @detail      Before inoking vdi_set_clock_freg() caller MUST invoke vdi_ready_change_clock() function.
     *              after changing clock frequency caller also invoke vdi_done_change_clock() function.
     * @return  0   failure
     *          1   success
     */
    int vdi_ready_change_clock(u32 core_idx);
    int vdi_set_change_clock(u32 core_idx, u32 clock_mask);
    int vdi_done_change_clock(u32 core_idx);

	int  vdi_get_instance_num(u32 core_idx);

	void vdi_write_register(u32 core_idx, unsigned int addr, unsigned int data);
	unsigned int vdi_read_register(u32 core_idx, unsigned int addr);
    void vdi_fio_write_register(u32 core_idx, unsigned int addr, unsigned int data);
    unsigned int vdi_fio_read_register(u32 core_idx, unsigned int addr);
	int vdi_clear_memory(u32 core_idx, PhysicalAddress addr, int len, int endian);
	int vdi_write_memory(u32 core_idx, PhysicalAddress addr, unsigned char *data, int len, int endian);
	int vdi_read_memory(u32 core_idx, PhysicalAddress addr, unsigned char *data, int len, int endian);

	int vdi_lock(u32 core_idx);
	int vdi_lock_check(u32 core_idx);
	void vdi_unlock(u32 core_idx);
	int vdi_disp_lock(u32 core_idx);
	void vdi_disp_unlock(u32 core_idx);
    void vdi_set_sdram(u32 core_idx, PhysicalAddress addr, int len, unsigned char data, int endian);
	void vdi_log(u32 core_idx, int cmd, int step);
	int vdi_open_instance(u32 core_idx, u32 inst_idx);
	int vdi_close_instance(u32 core_idx, u32 inst_idx);
	int vdi_set_bit_firmware_to_pm(u32 core_idx, const u16 *code);
    int vdi_get_system_endian(u32 core_idx);
    int vdi_convert_endian(u32 core_idx, unsigned int endian);
	void vdi_print_vpu_status(u32 coreIdx);


#if defined (__cplusplus)
}
#endif

#endif //#ifndef _VDI_H_
