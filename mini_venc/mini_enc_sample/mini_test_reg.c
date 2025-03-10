/***************************************************************************************************************
/***************************************************************************************************************
*
*  Filename: mini_test.c
*  -----------------------------------------------------------------------------
*  Description:
*  Certainly, this file encapsulates the fundamental implementation of the miniature encoder.
*  It principally involves initialization at the system-on-chip (SoC) level, initialization of the hardware Intellectual Property (IP),
*  configuration of the co-processor, initialization of the core registers of the encoder hardware,
*  and memory utilization targeting specific requirements. Furthermore, the encoder is designed to initiate on a frame-by-frame basis
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>	/* SIGIO */
#include <sys/mman.h> /* mmap */
#include <sys/ioctl.h>	/* fopen/fread */
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include "mini_vc9000e_parameter.h"
//#include "mini_enc_regs.h"
#include "mini_memory_pool.h"



typedef struct{
	unsigned int size;
	unsigned int cached;
	unsigned int phys_addr;
	unsigned int base; /* kernel logical address in use kernel */
	unsigned int virt_addr; /* virtual user space address */
} hantro_buffer_t;

typedef struct venc_context {
    unsigned int width;
    unsigned int height;
	unsigned int stride_width;
    unsigned int profile;
    unsigned int quality;
    unsigned int fps;
    unsigned int gop;
    unsigned char encType;
	unsigned char qpMaxP;
	unsigned char qpMaxI;
	unsigned char qpMinP;
	unsigned char qpMinI;

} venc_context_t;



typedef struct venc_ringbuffer
{
	//reference and recon common buffer
	unsigned char refRingBufEnable;
	unsigned int refRingBufExtendHeight;
	unsigned int pRefRingBuf_base;
	unsigned int refRingBuf_luma_rd_offset;
	unsigned int refRingBuf_luma_wr_offset;
	unsigned int refRingBuf_cb_rd_offset;
	unsigned int refRingBuf_cb_wr_offset;
	unsigned int refRingBuf_cr_rd_offset;
	unsigned int refRingBuf_cr_wr_offset;
	unsigned int refRingBuf_4n_rd_offset;
	unsigned int refRingBuf_4n_wr_offset;

	unsigned int refRingBuf_luma_size;
	unsigned int refRingBuf_chroma_size;
	unsigned int refRingBuf_4n_size;
	unsigned int refRingBuf_recon_chroma_half_size;

	unsigned int refRingBuf_recon_luma_addr;
	unsigned int refRingBuf_recon_cb_addr;
	unsigned int refRingBuf_recon_cr_addr;
	unsigned int refRingBuf_recon_4n_addr;

	unsigned int ref_frame_stride;
	unsigned int ref_frame_stride_ch;
	unsigned int ref_ds_luma_stride;

	unsigned int tblSize;
	unsigned int tblLumaSize;
	unsigned int tblChromaSize;

	unsigned int compressTbl_phys_luma_addr0;
	unsigned int compressTbl_phys_chroma_addr0;

	unsigned int compressTbl_phys_luma_addr1;
	unsigned int compressTbl_phys_chroma_addr1;
}venc_ringbuffer_t;

typedef enum {
	RECON_BUF_LUMA_ADDRESS = 0,
	RECON_BUF_CHROMA_ADDRESS,
	REFF_BUF_LUMA_ADDRESS,
	REF_BUF_CHROMA_ADDRESS,
	LIMU_LUMA_ADDRESS,
	LIMU_CHROMA_ADDRESS,
	RING_BUF_ADDRESS,  // Independent deal with ENUM_STR_TYPE.
	JOB_POOL,
	COMMAND_LINE,  // one option matches not only a kind of type but also other kinds. Independent deal with MIX_TYPE.
	VENC_REG,
	VENC_BSB_BUF,
	COEFF_SCAN,
	NAL_SIZE_TABLE,
	CMP_TABLE,
	CTB_PER_FRAME,
	AVC_COLLOCATED,
	COMPRESS_TBL,
	LUMA_BUF_SIZE,
	VENC_VCMD0,
	VENC_VCMD1,
	VENC_VCMD2
} MINI_ENCODER_MEM_INDEX;


#define DEBUG_REG 0
#define LOG_PRINTF 0
//#define RTOS_MINI_SYSTEM

#define STRIDE(variable, alignment)\
  ((variable + alignment - 1) & (~(alignment - 1)))
#define VCMD_MAX_SIZE 0x1000
#define VCMD_BUF_NUM 2
#define BSTM_BUF_NUM 1
#define ROI_BUF_NUM 1
#define HW_REG_SIZE 0x100000
#define BIT_STREAM_BUF_SIZE 0x400000
#define ROI_DATA_MAX_SIZE 0x40000
#define DEVICE_FILENAME "/dev/hantro"
#define Wr(addr, data) *(volatile unsigned int *)(addr)=(data)
#define Rd(addr) *(volatile unsigned int *)(addr)

#define VCMD_REG_BASE  0xfe310000
#define WRAP_RESET_CTL 0xfe002004
#define WRAP_RESET_SEL 0xfe096000
#define WRAP_CLOCK_CTL 0xfe00019c
#define HW_VCMD_BASE   0xfe310000
#define HW_CORE_BASE   0xfe311000
#define HW_RESET_CTL   0xfe310040
#define MAX_CORE_NUM 1

/* Page size and align for linear memory chunks. */
#define LINMEM_ALIGN    4096
#define NEXT_ALIGNED(x) ((((ptr_t)(x)) + LINMEM_ALIGN-1)/LINMEM_ALIGN*LINMEM_ALIGN)
#define NEXT_ALIGNED_SYS(x, align) ((((ptr_t)(x)) + align - 1) / align * align)
#define HANTRO_IOC_MAGIC 'k'
#define HANTRO_IOC_GET_BUFFER _IOR(HANTRO_IOC_MAGIC, 1, int)
#define HANTRO_IOC_MAXNR 1
#define HSWREG(n) ((n)*4)

extern unsigned int header_1080p[];
extern unsigned int header_2560p[];

extern unsigned int  header_2048_1536[];

extern unsigned int vce_vcmd0[];
extern unsigned int vce_vcmd1[];

int fd;
unsigned long vc9000e_reg_base;


static hantro_buffer_t common_buffer;
static hantro_buffer_t hw_regs;
static hantro_buffer_t vcmd_buffer[VCMD_BUF_NUM];

static hantro_buffer_t raw_data;
static int  raw_data_number;

static hantro_buffer_t recon_buffer[1];

static hantro_buffer_t reference_buffer[1];

static hantro_buffer_t compressTbl[2];

unsigned long Debug_Dram_usage = 0;

static hantro_buffer_t bitstream_buffer[BSTM_BUF_NUM];

static hantro_buffer_t recon_buffer[1];

static hantro_buffer_t compress_coeff_SCAN[MAX_CORE_NUM];

static venc_ringbuffer_t mini_sys_ringbuffer;

void *header;

void *vce_vcmd[VCMD_BUF_NUM];
void *vce_raw_data[VCMD_BUF_NUM];

venc_context_t g_context;

#if DEBUG_REG


//sam mdf
FILE *g_debug_fp;
void  debug_init(char *file)
{
	g_debug_fp = fopen(file,"w");
	if (!g_debug_fp)
	{
		#if LOG_PRINTF
		printf("debug_init err\n");
		#endif
	}

}

int debug(  char *__format,...)
{
	signed int rc;
	va_list args;

	va_start(args, __format);
	vfprintf(g_debug_fp, __format, args);
	va_end(args);
	return 0;
}

void  debug_uninit(void)
{
	fclose(g_debug_fp);
}
//end




void EncTraceRegs(const void *ewl, unsigned int readWriteFlag,unsigned int mbNum, unsigned int *regs)
{
	int i;
	int lastRegAddr = 556*4;  //0x48C;
	char rw = 'W';
	static int frame = 0;
	char log_name[128]={0};
	char bin_name[128]={0};
	char num[20]={0};

	FILE *fp = NULL;

	strcpy(bin_name,"/data/bin/vcmd_bin");

	sprintf(num,"_%06d",frame);
	strcat(bin_name ,num);

	fp = fopen(bin_name, "wb");

	if (fp == NULL) {
	  return;
	}


	/* Dump registers in same denali format as the system model */
	for (i = 0; i < lastRegAddr; i += 4) {

		fwrite((void*)&regs[i / 4], 4, 1, fp);
	}

	fclose(fp);

	strcpy(log_name,"/data/log/reg_debug");

	sprintf(num,"_%06d",frame);
	strcat(log_name ,num);


	debug_init(log_name);

	//REGS_LOGI((void *)ewl, "pic=%d\n", frame);
	//REGS_LOGI((void *)ewl, "mb=%d\n", mbNum);

	/* After frame is finished, registers are read */
	if (readWriteFlag) {
		rw = 'R';

	}
	frame++;


	/* Dump registers in same denali format as the system model */
	for (i = 0; i < lastRegAddr; i += 4) {
	/* DMV penalty tables not possible to read from ASIC: 0x180-0x27C */
	//if ((i != 0xA0) && (i != 0x38) && (i < 0x180 || i > 0x27C))
	// if (i != HSWREG(ASIC_REG_INDEX_STATUS))
	  debug( "%c %08x/%08x\n", rw, i,
	            regs != NULL ? regs[i / 4] : MiniReadReg(ewl, i));
	}

	/* Regs with enable bits last, force encoder enable high for frame start */

	debug( "\n");

	debug_uninit();

}

#endif

void CompressTableSize(unsigned int compressor, unsigned int width, unsigned int height,
                             unsigned int *lumaCompressTblSize,
                             unsigned int *chromaCompressTblSize) {
  unsigned int tblLumaSize = 0;
  unsigned int tblChromaSize = 0;
  unsigned int tblSize = 0;

  if (compressor & 1) {
    tblLumaSize = ((width + 63) / 64) * ((height + 63) / 64) * 8;  //ctu_num * 8
    tblLumaSize = ((tblLumaSize + 15) >> 4) << 4;
    *lumaCompressTblSize = tblLumaSize;
  }
  if (compressor & 2) {
    int cbs_w = ((width >> 1) + 7) / 8;
    int cbs_h = ((height >> 1) + 3) / 4;
    int cbsg_w = (cbs_w + 15) / 16;
    tblChromaSize = cbsg_w * cbs_h * 16;
    *chromaCompressTblSize = tblChromaSize;
  }
  return;
}


/**
 * @fn void vce_frame_start(venc_context_t *context,int frm_num,venc_ringbuffer_t * mini_sys_ringbuffer,unsigned int source_address)
 * @brief Adds two integers and returns the result.
 *
 * This function takes two integers as input, adds them together, and returns the result.
 *
 * @param context The first integer to add.
 * @param frm_num The second integer to add.
 *
 * @return void.
 */
void vce_frame_start(venc_context_t *context,int frm_num,venc_ringbuffer_t * mini_sys_ringbuffer,unsigned int source_address)
{
    unsigned int Luma_addr = 0;
    unsigned int Cb_addr = 0;
    unsigned int Cr_addr = 0;
    unsigned int    i = 0;
    unsigned int  *regMirror = NULL;

    Luma_addr = source_address;

    if (frm_num == 0)// I frame
        {
            regMirror = (unsigned int *)vcmd_buffer[0].virt_addr + 1;
            Cb_addr = Luma_addr + context->stride_width * context->height;
            Cr_addr =Luma_addr + context->stride_width * context->height + context->stride_width * context->height / 2;
        #if LOG_PRINTF
            printf("vce_frame_start >> frm_num %d,Luma_addr = 0x%x\n",frm_num,Luma_addr);
        #endif

            regMirror[0x14 / 4] = (regMirror[0x14 / 4] & ~(0xffc00000)) | ((context->width>>3 << 0x00000016) & 0xffc00000);
            regMirror[0x14 / 4] = (regMirror[0x14 / 4] & ~(0x003ff800)) | ((context->height>>3 << 0x0000000b) & 0x003ff800);
            regMirror[0x14 / 4] = (regMirror[0x14 / 4] & ~(0x00000006)) | ((1 << 0x00000001) & 0x00000006);
            regMirror[0x98 / 4] = (regMirror[0x98 / 4] & ~(0x000fffc0)) | ((context->width << 0x00000006) & 0x000fffc0);
            regMirror[0x3b4 / 4] = (regMirror[0x3b4 / 4] & ~(0xfffff000)) | ((context->width<<2 << 0x0000000c) & 0xfffff000);
            regMirror[0x414 / 4] = (regMirror[0x414 / 4] & ~(0xfff80000)) | ((context->width>>3 << 0x00000013) & 0xfff80000);
            regMirror[0x350 / 4] = (regMirror[0x350 / 4] & ~(0xfffff000)) | ((context->width<<2 << 0x0000000c) & 0xfffff000);
            regMirror[0x354 / 4] = (regMirror[0x354 / 4] & ~(0xffffc000)) | ((context->width << 0x0000000e) & 0xffffc000);
            regMirror[0x1a4 / 4] = (regMirror[0x1a4 / 4] & ~(0xffffffff)) | ((0x927a0 << 0x00000000) & 0xffffffff);
            regMirror[0x30 / 4] = (regMirror[0x30 / 4] & ~(0xffffffff)) | ((Luma_addr << 0x00000000) & 0xffffffff);
            regMirror[0x34 / 4] = (regMirror[0x34 / 4] & ~(0xffffffff)) | ((Cb_addr << 0x00000000) & 0xffffffff);
            regMirror[0x38 / 4] = (regMirror[0x38 / 4] & ~(0xffffffff)) | ((Cr_addr << 0x00000000) & 0xffffffff);
            regMirror[0x348 / 4] = (regMirror[0x348 / 4] & ~(0xfffff000)) | ((context->stride_width << 0x0000000c) & 0xfffff000);
            regMirror[0x34c / 4] = (regMirror[0x34c / 4] & ~(0xfffff000)) | ((context->stride_width << 0x0000000c) & 0xfffff000);
            regMirror[0x20 / 4] = (regMirror[0x20 / 4] & ~(0xffffffff)) | ((bitstream_buffer[0].phys_addr << 0x00000000) & 0xffffffff);
            regMirror[0x98 / 4] = (regMirror[0x98 / 4] & ~(0xf0000000)) | ((1 << 0x0000001c) & 0xf0000000);
            regMirror[0xf0 / 4] = (regMirror[0xf0 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->compressTbl_phys_luma_addr0 << 0x00000000) & 0xffffffff);
            regMirror[0xf8 / 4] = (regMirror[0xf8 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->compressTbl_phys_chroma_addr0 << 0x00000000) & 0xffffffff);
            regMirror[0x100 / 4] = (regMirror[0x100 / 4] & ~(0xffffffff)) | ((0x0 << 0x00000000) & 0xffffffff);
            regMirror[0x108 / 4] = (regMirror[0x108 / 4] & ~(0xffffffff)) | ((0x0 << 0x00000000) & 0xffffffff);
            regMirror[0xb8 / 4] = (regMirror[0xb8 / 4] & ~(0xffffffff)) | ((compress_coeff_SCAN[0].phys_addr << 0x00000000) & 0xffffffff);
            regMirror[0x14c / 4] = (regMirror[0x14c / 4] & ~(0xffffffff)) | ((recon_buffer[0].phys_addr << 0x00000000) & 0xffffffff);
            regMirror[0x2c / 4] = (regMirror[0x2c / 4] & ~(0xffffffff)) | ((frm_num << 0x00000000) & 0xffffffff);
            regMirror[0x3c / 4] = (regMirror[0x3c / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_luma_wr_offset << 0x00000000) & 0xffffffff);
            regMirror[0x40 / 4] = (regMirror[0x40 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_cb_wr_offset << 0x00000000) & 0xffffffff);
            regMirror[0x48 / 4] = (regMirror[0x48 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_luma_rd_offset << 0x00000000) & 0xffffffff);
            regMirror[0x4c / 4] = (regMirror[0x4c / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_cb_rd_offset << 0x00000000) & 0xffffffff);
            regMirror[0xe0 / 4] = (regMirror[0xe0 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_luma_size << 0x00000000) & 0xffffffff);
            regMirror[0xe4 / 4] = (regMirror[0xe4 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_chroma_size << 0x00000000) & 0xffffffff);
            regMirror[0x120 / 4] = (regMirror[0x120 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_4n_wr_offset << 0x00000000) & 0xffffffff);
            regMirror[0x124 / 4] = (regMirror[0x124 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_4n_size << 0x00000000) & 0xffffffff);
            regMirror[0x128 / 4] = (regMirror[0x128 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_4n_rd_offset << 0x00000000) & 0xffffffff);
            regMirror[0x81c / 4] = (regMirror[0x81c / 4] & ~(0xffffffff)) | ((0xcc000106 << 0x00000000) & 0xffffffff);
            regMirror[0x820 / 4] = (regMirror[0x820 / 4] & ~(0xffffffff)) | ((vcmd_buffer[1].phys_addr << 0x00000000) & 0xffffffff);
            regMirror[0x828 / 4] = (regMirror[0x828 / 4] & ~(0xffffffff)) | ((0x2 << 0x00000000) & 0xffffffff);
            regMirror[0xb4 / 4] = (regMirror[0xb4 / 4] & ~(0x08000000)) | ((0x1 << 0x0000001b) & 0x08000000);

            Wr(VC9000E_VCMD_SWREG016, 0x00000021);
        }
    else
    {
        regMirror = (unsigned int *)vcmd_buffer[1].virt_addr + 1;
        Cb_addr = Luma_addr + context->stride_width * context->height;
        Cr_addr =Luma_addr + context->stride_width * context->height + context->stride_width * context->height / 2;

        regMirror[0x14 / 4] = (regMirror[0x14 / 4] & ~(0xffc00000)) | ((context->width>>3 << 0x00000016) & 0xffc00000);
        regMirror[0x14 / 4] = (regMirror[0x14 / 4] & ~(0x003ff800)) | ((context->height>>3 << 0x0000000b) & 0x003ff800);
        regMirror[0x14 / 4] = (regMirror[0x14 / 4] & ~(0x00000006)) | ((0 << 0x00000001) & 0x00000006);
        regMirror[0x98 / 4] = (regMirror[0x98 / 4] & ~(0x000fffc0)) | ((context->width << 0x00000006) & 0x000fffc0);
        regMirror[0x350 / 4] = (regMirror[0x350 / 4] & ~(0xfffff000)) | ((context->width<<2 << 0x0000000c) & 0xfffff000);
        regMirror[0x354 / 4] = (regMirror[0x354 / 4] & ~(0xffffc000)) | ((context->width << 0x0000000e) & 0xffffc000);
        regMirror[0x3b4 / 4] = (regMirror[0x3b4 / 4] & ~(0xfffff000)) | ((context->width<<2 << 0x0000000c) & 0xfffff000);
        regMirror[0x414 / 4] = (regMirror[0x414 / 4] & ~(0xfff80000)) | ((context->width>>3 << 0x00000013) & 0xfff80000);
        regMirror[0x2c / 4] = (regMirror[0x2c / 4] & ~(0xffffffff)) | ((frm_num << 0x00000000) & 0xffffffff);
        regMirror[0x30 / 4] = (regMirror[0x30 / 4] & ~(0xffffffff)) | ((Luma_addr << 0x00000000) & 0xffffffff);
        regMirror[0x34 / 4] = (regMirror[0x34 / 4] & ~(0xffffffff)) | ((Cb_addr << 0x00000000) & 0xffffffff);
        regMirror[0x38 / 4] = (regMirror[0x38 / 4] & ~(0xffffffff)) | ((Cr_addr << 0x00000000) & 0xffffffff);
        regMirror[0x348 / 4] = (regMirror[0x348 / 4] & ~(0xfffff000)) | ((context->stride_width << 0x0000000c) & 0xfffff000);
        regMirror[0x34c / 4] = (regMirror[0x34c / 4] & ~(0xfffff000)) | ((context->stride_width << 0x0000000c) & 0xfffff000);
        regMirror[0x20 / 4] = (regMirror[0x20 / 4] & ~(0xffffffff)) | ((bitstream_buffer[0].phys_addr << 0x00000000) & 0xffffffff);
        regMirror[0x98 / 4] = (regMirror[0x98 / 4] & ~(0xf0000000)) | ((1 << 0x0000001c) & 0xf0000000);

        if (frm_num%2)
        {
            regMirror[0xf0 / 4] = (regMirror[0xf0 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->compressTbl_phys_luma_addr1 << 0x00000000) & 0xffffffff);
            regMirror[0xf8 / 4] = (regMirror[0xf8 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->compressTbl_phys_chroma_addr1 << 0x00000000) & 0xffffffff);
            regMirror[0x100 / 4] = (regMirror[0x100 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->compressTbl_phys_luma_addr0 << 0x00000000) & 0xffffffff);
            regMirror[0x108 / 4] = (regMirror[0x108 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->compressTbl_phys_chroma_addr0 << 0x00000000) & 0xffffffff);
        }
        else
        {
            regMirror[0xf0 / 4] = (regMirror[0xf0 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->compressTbl_phys_luma_addr0 << 0x00000000) & 0xffffffff);
            regMirror[0xf8 / 4] = (regMirror[0xf8 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->compressTbl_phys_chroma_addr0 << 0x00000000) & 0xffffffff);
            regMirror[0x100 / 4] = (regMirror[0x100 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->compressTbl_phys_luma_addr1 << 0x00000000) & 0xffffffff);
            regMirror[0x108 / 4] = (regMirror[0x108 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->compressTbl_phys_chroma_addr1 << 0x00000000) & 0xffffffff);
        }
        regMirror[0xb8 / 4] = (regMirror[0xb8 / 4] & ~(0xffffffff)) | ((compress_coeff_SCAN[0].phys_addr << 0x00000000) & 0xffffffff);
        regMirror[0x14c / 4] = (regMirror[0x14c / 4] & ~(0xffffffff)) | ((recon_buffer[0].phys_addr << 0x00000000) & 0xffffffff);
        regMirror[0x3c / 4] = (regMirror[0x3c / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_luma_wr_offset << 0x00000000) & 0xffffffff);
        regMirror[0x40 / 4] = (regMirror[0x40 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_cb_wr_offset << 0x00000000) & 0xffffffff);
        regMirror[0x48 / 4] = (regMirror[0x48 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_luma_rd_offset << 0x00000000) & 0xffffffff);
        regMirror[0x4c / 4] = (regMirror[0x4c / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_cb_rd_offset << 0x00000000) & 0xffffffff);
        regMirror[0xe0 / 4] = (regMirror[0xe0 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_luma_size << 0x00000000) & 0xffffffff);
        regMirror[0xe4 / 4] = (regMirror[0xe4 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_chroma_size << 0x00000000) & 0xffffffff);
        regMirror[0x120 / 4] = (regMirror[0x120 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_4n_wr_offset << 0x00000000) & 0xffffffff);
        regMirror[0x124 / 4] = (regMirror[0x124 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_4n_size << 0x00000000) & 0xffffffff);
        regMirror[0x128 / 4] = (regMirror[0x128 / 4] & ~(0xffffffff)) | ((mini_sys_ringbuffer->refRingBuf_4n_rd_offset << 0x00000000) & 0xffffffff);
        regMirror[0xb4 / 4] = (regMirror[0xb4 / 4] & ~(0x08000000)) | ((0x1 << 0x0000001b) & 0x08000000);
        regMirror[0x81c / 4] = (regMirror[0x81c / 4] & ~(0xffffffff)) | ((0xcc000106 << 0x00000000) & 0xffffffff);
        regMirror[0x828 / 4] = (regMirror[0x828 / 4] & ~(0xffffffff)) | ((frm_num+2 << 0x00000000) & 0xffffffff);
        regMirror[0x820 / 4] = (regMirror[0x820 / 4] & ~(0xffffffff)) | ((vcmd_buffer[1].phys_addr << 0x00000000) & 0xffffffff);
        Wr(VC9000E_VCMD_OFFSET+0x00000060, frm_num+1);
    }

    return;
}

/*set RingBuffer registers */
int  mini_InnerLoop_Update(venc_context_t *context,int pic_type,venc_ringbuffer_t * mini_sys_ringbuffer)
{
    //each block buffer size
    unsigned int lumaBufSize     = mini_sys_ringbuffer->refRingBuf_luma_size;
    unsigned int chromaBufSize  =  mini_sys_ringbuffer->refRingBuf_chroma_size;
    unsigned int lumaBufSize4N  =  mini_sys_ringbuffer->refRingBuf_4n_size;
    unsigned int chromaHalfSize =  mini_sys_ringbuffer->refRingBuf_recon_chroma_half_size;

    //each block base address offset from RingBuffer base address.
    unsigned int lumaBaseAddress =  mini_sys_ringbuffer->pRefRingBuf_base;
    unsigned int chromaBaseAddress = mini_sys_ringbuffer->pRefRingBuf_base + lumaBufSize;
    unsigned int luma4NBaseAddress = mini_sys_ringbuffer->pRefRingBuf_base + lumaBufSize + chromaBufSize;
	#if LOG_PRINTF
	printf("mini_InnerLoop_Update Start\n");
	#endif
    if (pic_type == 0)//I slice  CHECK //pic->sliceInst->type == I_SLICE
    {
		mini_sys_ringbuffer->refRingBuf_recon_luma_addr = mini_sys_ringbuffer->pRefRingBuf_base;
		mini_sys_ringbuffer->refRingBuf_recon_cb_addr = mini_sys_ringbuffer->pRefRingBuf_base + lumaBufSize;
		mini_sys_ringbuffer->refRingBuf_recon_cr_addr = mini_sys_ringbuffer->refRingBuf_recon_cb_addr + chromaHalfSize;
		mini_sys_ringbuffer->refRingBuf_recon_4n_addr = mini_sys_ringbuffer->refRingBuf_recon_cb_addr + chromaBufSize;
		#if LOG_PRINTF
		printf("debug mini_InnerLoop_Update >> refRingBuf_cb_wr_offset = 0x%x, chromaBufSize = 0x%x\n",mini_sys_ringbuffer->refRingBuf_cb_wr_offset,chromaBufSize);

		printf("debug mini_InnerLoop_Update >> recon.lum = 0x%x,>recon.cb = 0x%x,recon_4n_base = 0x%x\n",mini_sys_ringbuffer->refRingBuf_recon_luma_addr,mini_sys_ringbuffer->refRingBuf_recon_cb_addr ,mini_sys_ringbuffer->refRingBuf_recon_4n_addr );
		#endif
    }
    else
    {
        //reference data reading offset from each block base address.
		unsigned int lumaReadOffset   = mini_sys_ringbuffer->refRingBuf_recon_luma_addr - lumaBaseAddress;
		unsigned int cbReadOffset     = mini_sys_ringbuffer->refRingBuf_recon_cb_addr - chromaBaseAddress;
		unsigned int crReadOffset     = mini_sys_ringbuffer->refRingBuf_recon_cr_addr - chromaBaseAddress;
		unsigned int luma4NReadOffset = mini_sys_ringbuffer->refRingBuf_recon_4n_addr - luma4NBaseAddress;

		//update ref read address
		mini_sys_ringbuffer->refRingBuf_luma_rd_offset = lumaReadOffset;
		mini_sys_ringbuffer->refRingBuf_cb_rd_offset = cbReadOffset;
		mini_sys_ringbuffer->refRingBuf_4n_rd_offset = luma4NReadOffset;
		unsigned int width,width_4n,height;
		width = ((context->width + 63) >> 6) << 6;
		width_4n = ((context->width + 15) / 16) * 4;

        //actual luma/chroma/4N size
		unsigned int lumaSize   = lumaBufSize - mini_sys_ringbuffer->refRingBufExtendHeight * mini_sys_ringbuffer->ref_frame_stride /4;
		unsigned int chromaSize = chromaBufSize - mini_sys_ringbuffer->refRingBufExtendHeight/2 * mini_sys_ringbuffer->ref_frame_stride_ch /4;
		unsigned int lumaSize4N = lumaBufSize4N - mini_sys_ringbuffer->refRingBufExtendHeight/4 * mini_sys_ringbuffer->ref_ds_luma_stride/4;
		#if LOG_PRINTF
		printf("debug mini_InnerLoop_Update >> actual luma/chroma/4N size.lumaSize = 0x%x,chromaSize = 0x%x,lumaSize4N = 0x%x,ref_ds_luma_stride = 0x%x\n",lumaSize,chromaSize,lumaSize4N,mini_sys_ringbuffer->ref_ds_luma_stride);
		#endif
		//update recon address
		if (lumaReadOffset + lumaSize < lumaBufSize)
			mini_sys_ringbuffer->refRingBuf_recon_luma_addr = mini_sys_ringbuffer->refRingBuf_recon_luma_addr + lumaSize;
		else
			mini_sys_ringbuffer->refRingBuf_recon_luma_addr = mini_sys_ringbuffer->refRingBuf_recon_luma_addr + lumaSize - lumaBufSize;


		if (cbReadOffset + chromaSize < chromaBufSize)
			mini_sys_ringbuffer->refRingBuf_recon_cb_addr = mini_sys_ringbuffer->refRingBuf_recon_cb_addr + chromaSize;
		else
			mini_sys_ringbuffer->refRingBuf_recon_cb_addr = mini_sys_ringbuffer->refRingBuf_recon_cb_addr + chromaSize - chromaBufSize;


		if (crReadOffset + chromaSize < chromaBufSize)
			mini_sys_ringbuffer->refRingBuf_recon_cr_addr = mini_sys_ringbuffer->refRingBuf_recon_cr_addr  + chromaSize;
		else
			mini_sys_ringbuffer->refRingBuf_recon_cr_addr  = mini_sys_ringbuffer->refRingBuf_recon_cr_addr  + chromaSize - chromaBufSize;


		if (luma4NReadOffset + lumaSize4N < lumaBufSize4N)
			mini_sys_ringbuffer->refRingBuf_recon_4n_addr = mini_sys_ringbuffer->refRingBuf_recon_4n_addr + lumaSize4N;
		else
			mini_sys_ringbuffer->refRingBuf_recon_4n_addr = mini_sys_ringbuffer->refRingBuf_recon_4n_addr + lumaSize4N - lumaBufSize4N;

	}


    //recon data writing offset from each block base address.
    mini_sys_ringbuffer->refRingBuf_luma_wr_offset = mini_sys_ringbuffer->refRingBuf_recon_luma_addr - lumaBaseAddress;
    mini_sys_ringbuffer->refRingBuf_cb_wr_offset   = mini_sys_ringbuffer->refRingBuf_recon_cb_addr - chromaBaseAddress;
    mini_sys_ringbuffer->refRingBuf_4n_wr_offset   = mini_sys_ringbuffer->refRingBuf_recon_4n_addr - luma4NBaseAddress;
#if LOG_PRINTF
	printf("debug mini_InnerLoop_Update >> recon.lum = 0x%x,>recon.cb = 0x%x,recon_4n_base = 0x%x\n",mini_sys_ringbuffer->refRingBuf_recon_luma_addr,mini_sys_ringbuffer->refRingBuf_recon_cb_addr ,mini_sys_ringbuffer->refRingBuf_recon_4n_addr );
	printf("debug mini_InnerLoop_Update >> lumaBaseAddress = 0x%x,chromaBaseAddress = 0x%x,luma4NBaseAddress = 0x%x\n",lumaBaseAddress,chromaBaseAddress,luma4NBaseAddress);

    printf("debug mini_InnerLoop_Update >> refRingBuf_luma_wr_offset = 0x%x,refRingBuf_chroma_wr_offset = 0x%x,refRingBuf_4n_wr_offset = 0x%x\n", mini_sys_ringbuffer->refRingBuf_luma_wr_offset,mini_sys_ringbuffer->refRingBuf_cb_wr_offset,mini_sys_ringbuffer->refRingBuf_4n_wr_offset);

    printf("debug mini_InnerLoop_Update >> refRingBuf_luma_rd_offset = 0x%x,refRingBuf_chroma_rd_offset = 0x%x,refRingBuf_4n_rd_offset = 0x%x\n",mini_sys_ringbuffer->refRingBuf_luma_rd_offset,mini_sys_ringbuffer->refRingBuf_cr_rd_offset ,mini_sys_ringbuffer->refRingBuf_4n_rd_offset );

	printf("mini_InnerLoop_Update End\n");
#endif

	return 0;

}


void vce_stream_dump(unsigned int frm_num)
{
    FILE *file_bitstream;
    char name[80];

    unsigned int bitstream_size = Rd(VC9000E_CORE_SWREG009);

	//bitstream_size=	BIT_STREAM_BUF_SIZE;
    sprintf(name, "/data/bin/frm%d.bin", frm_num);
    file_bitstream = fopen(name, "wb");
    if (file_bitstream == NULL) {
        return;
    }

    fwrite((void*)bitstream_buffer[frm_num].virt_addr, bitstream_size, 1, file_bitstream);
	#if LOG_PRINTF
	printf("bitstream_buffer[frm_num].virt_addr = 0x%x\n", (void*)bitstream_buffer[frm_num].virt_addr);
    printf("bitstream dump success, size 0x%x\n", bitstream_size);
	#endif
    fclose(file_bitstream);
}

FILE *es_bitstream;

int mini_es_dump_init(void)
{
	int ret = 0;
	char name[80];
	sprintf(name, "/data/es.265");
	es_bitstream = fopen(name, "wb");
	if (es_bitstream == NULL)
	{
		ret = -1;
		return ret;
	}
	return ret;
}

void mini_es_gen_header_dump_init(void *header,int head_size)
{

	fwrite((void*)header, head_size, 1, es_bitstream);
	#if LOG_PRINTF
	printf("bitstream mini_es_gen_header_dump_init success, size 0x%x\n", head_size);
	#endif
}


void mini_es_dump(unsigned int frm_num)
{

	FILE *file_bitstream;
	char name[80];

	unsigned int bitstream_size = Rd(VC9000E_CORE_SWREG009);
	fwrite((void*)bitstream_buffer[frm_num].virt_addr, bitstream_size, 1, es_bitstream);
	#if LOG_PRINTF
	printf("bitstream_buffer[frm_num].virt_addr = 0x%x\n", (void*)bitstream_buffer[frm_num].virt_addr);
	printf("mini_es_dump success, size 0x%x\n", bitstream_size);
	#endif
}

void vce_es_dump_close(void)
{
	#if LOG_PRINTF
	printf("vce_es_dump_close success\n");
	#endif
	fclose(es_bitstream);
}

unsigned int recon_chroma_half_size;
unsigned int ref_frame_stride;

void GetSizeAndSetRegs(venc_context_t *context,
                              unsigned int  *internalImageLumaSize, unsigned int  *lumaSize,
                              unsigned int  *lumaSize4N, unsigned int  *chromaSize,
                              unsigned int  *u32FrameContextSize)
{
	unsigned int  width = context->width;
	unsigned int  height = context->height;
	unsigned int  alignment = 64;
	unsigned int  alignment_ch = 64;
	unsigned int  width_4n, height_4n;

	width = ((width + 63) >> 6) << 6;
	height = ((height + 63) >> 6) << 6;

	width_4n = ((context->width + 15) / 16) * 4;
	height_4n = height / 4;

	recon_chroma_half_size =
      ((width * height) + (width * height) * (8 - 8) / 8) /4;

	/* 6.0 software:  merge*/
	*u32FrameContextSize = 0;



	ref_frame_stride =
        STRIDE(STRIDE(width * 4 * 8 / 8, 16), alignment);

    *lumaSize = ref_frame_stride * height / 4;

    *lumaSize4N = STRIDE(STRIDE(width_4n * 4 * 8 / 8, 16),alignment) *height_4n / 4;
    *internalImageLumaSize = ((*lumaSize + *lumaSize4N + 0xf) & (~0xf));
    *internalImageLumaSize = STRIDE(*internalImageLumaSize, alignment);
    *chromaSize = *lumaSize / 2;

}
int mini_system_encoder_init(venc_context_t *context)
{
	int ret = 0;
	unsigned int tblLumaSize = 0;
	unsigned int tblChromaSize = 0;
	unsigned int tblSize = 0;
	unsigned int lumaBufSize;
	unsigned int RefRingBufExtendHeight = 0;
	unsigned int chromaBufSize;
	unsigned int lumaBufSize4N;
	unsigned int lumaSize = 0;
	unsigned int lumaSize4N = 0;
	unsigned int chromaSize = 0;
	unsigned int u32FrameContextSize = 0;
	unsigned int internalImageLumaSize = 0;
	unsigned int RefRingBufSize = 0;
	unsigned int ref_frame_stride_ch = 0;
	unsigned int ref_ds_luma_stride = 0;
	unsigned int pgsize = 0;
	unsigned int alignment = 0;
	unsigned int alignment_ch = 0;
	unsigned int head_size = 0;
	unsigned int width_4n = 0;

	int i = 0;

#if LOG_PRINTF
	printf("mini_system_encoder_init:\n");
#endif

	vce_vcmd[0] = (void*)vce_vcmd0;
	vce_vcmd[1] = (void*)vce_vcmd1;

	if (context->width == 2560)
		header = (void*)header_2560p;
	else if (context->width == 1920)
		header = (void*)header_1080p;

//	head_size = 0x5a;
    head_size = 0x59;//2560
#if LOG_PRINTF
	printf("head_size = 0x%x\n",head_size);
#endif

	mini_es_gen_header_dump_init(header,head_size);


	alignment = 64;
	alignment_ch = 64;
	RefRingBufExtendHeight = 128;
	pgsize = 4096;


#ifndef RTOS_MINI_SYSTEM
	//open device
	fd = open(DEVICE_FILENAME, O_RDWR | O_SYNC);
	if (fd < 0)
	{
        return -1;
    }

	//Get all the encoder phy mem address
	ret = ioctl(fd, HANTRO_IOC_GET_BUFFER, &common_buffer);

	if (0 != ret)
	{
	   return ret;
	}
#endif




#if LOG_PRINTF
	//printf("ioctl get encoder mem buffer start,phys_addr:0x%lx,size:0x%x,mmap virt_addr 0x%lx\n", common_buffer.phys_addr, common_buffer.size, common_buffer.virt_addr);
#endif


#ifdef RTOS_MINI_SYSTEM

	common_buffer.phys_addr = 0; //FIX ME

	common_buffer.size = 0;//FIX ME

#endif



	mini_memalloc_init(common_buffer.phys_addr,common_buffer.size);

	//hw register phy address
	hw_regs.phys_addr = VCMD_REG_BASE;
	hw_regs.size = HW_REG_SIZE;
	vc9000e_reg_base = hw_regs.phys_addr;

#ifndef RTOS_MINI_SYSTEM

	hw_regs.virt_addr = (ulong)mmap(0,
			hw_regs.size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd,
			hw_regs.phys_addr);


	//remap register base
	vc9000e_reg_base = hw_regs.virt_addr;
#endif


#if LOG_PRINTF
	printf("ioctl get hw register start,phys_addr:0x%lx,size:0x%x,mmap virt_addr 0x%lx\n", hw_regs.phys_addr ,hw_regs.size , hw_regs.virt_addr);
#endif


	/***************************************************************************/

	//vcmd0 mem
	vcmd_buffer[0].size = VCMD_MAX_SIZE;
	ret = AllocMemory(&vcmd_buffer[0].phys_addr,vcmd_buffer[0].size,VENC_VCMD0);


	if (ret != 0)
	{
		return -1;
	}



//dram alloc
#ifndef RTOS_MINI_SYSTEM

	vcmd_buffer[0].virt_addr = (ulong)mmap(0,
				vcmd_buffer[0].size,
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				fd,
				vcmd_buffer[0].phys_addr);
#endif


#if LOG_PRINTF
	printf("VCMD0 mem mmap phys_addr 0x%lx, virt_addr 0x%lx\n",vcmd_buffer[0].phys_addr,vcmd_buffer[0].virt_addr);
	printf("#######Here SamSu debug mini system phys dram usage >> vcmd_buffer : size :0x%x,phys_addr:0x%lx\n", vcmd_buffer[0].size,vcmd_buffer[0].phys_addr);
	Debug_Dram_usage += vcmd_buffer[0].size;
#endif



	//vcmd1 mem
	vcmd_buffer[1].size = VCMD_MAX_SIZE;
	ret = AllocMemory(&vcmd_buffer[1].phys_addr,vcmd_buffer[1].size,VENC_VCMD1);

	if (ret != 0)
	{
		return -1;
	}

#ifndef RTOS_MINI_SYSTEM
	vcmd_buffer[1].virt_addr = (ulong)mmap(0,
				vcmd_buffer[1].size,
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				fd,
				vcmd_buffer[1].phys_addr);
#endif


#if LOG_PRINTF
	printf("VCMD1 mem mmap phys_addr0 0x%lx, virt_addr0 0x%lx\n",vcmd_buffer[1].phys_addr,vcmd_buffer[1].virt_addr);
	printf("#######Here SamSu debug mini system phys dram usage >> vcmd_buffer : size :0x%x,phys_addr:0x%lx\n", vcmd_buffer[1].size,vcmd_buffer[1].phys_addr);
#endif

	Debug_Dram_usage += vcmd_buffer[1].size;


	/*
	allocate scan coeff buffer before reference frame to avoid roimap overeading(64~1536 bytes) hit reference frame:
	reference frame could be in DEC400(1024B burst size)
	*/
	for (i = 0; i < 1; i++)
	{
#if LOG_PRINTF
		printf("mem debug >> compress_coeff_SCAN\n");
#endif
		compress_coeff_SCAN[i].size =  288 * 1024 / 8;
		ret = AllocMemory(&compress_coeff_SCAN[i].phys_addr ,compress_coeff_SCAN[i].size,COEFF_SCAN);
		if (ret != 0)
		{
			return -1;
		}
	}

#if LOG_PRINTF
	printf("#######Here SamSu debug mini system phys dram usage >> compress_coeff_SCAN : size :0x%x,phys_addr:0x%lx\n",compress_coeff_SCAN[0].size,compress_coeff_SCAN[0].phys_addr);
#endif

	Debug_Dram_usage += compress_coeff_SCAN[0].size;

	////////////////////////////////////////////////////////////////////////////////////////

	//RECON

	memset(&mini_sys_ringbuffer,0,sizeof(venc_ringbuffer_t));

	GetSizeAndSetRegs(context, &internalImageLumaSize, &lumaSize,
                           &lumaSize4N, &chromaSize, &u32FrameContextSize);



	width_4n = ((context->width + 15) / 16) * 4;

	ref_ds_luma_stride = STRIDE(
        STRIDE(width_4n * 4 * 8 / 8, 16), 64);

	ref_frame_stride_ch =
        STRIDE((context->width * 4 * 8 / 8), 64);

	RefRingBufExtendHeight = 128;

	lumaBufSize = lumaSize + RefRingBufExtendHeight *  ref_frame_stride /4;
	chromaBufSize = chromaSize + RefRingBufExtendHeight/2 * ref_frame_stride_ch /4;
	lumaBufSize4N = lumaSize4N + RefRingBufExtendHeight/4 *ref_ds_luma_stride /4;
#if LOG_PRINTF
	printf("mem debug >> ringbuffer:lumaBufSize = %d,chromaBufSize = %d,lumaBufSize4N = %d,alignment = %d,alignment_ch = %d\n",lumaBufSize,chromaBufSize,lumaBufSize4N,alignment,alignment_ch);
	printf("mem debug >> ringbuffer:lumaSize = %d,chromaSize = %d,lumaSize4N = %d,RefRingBufExtendHeight = %ds\n",lumaSize,chromaSize,lumaSize4N,RefRingBufExtendHeight);
	printf("mem debug >> ringbuffer:ref_frame_stride = %d,ref_frame_stride_ch = %d,ref_ds_luma_stride = %d\n",ref_frame_stride, ref_frame_stride_ch,ref_ds_luma_stride);
#endif
	RefRingBufSize = lumaBufSize + chromaBufSize + lumaBufSize4N;

	recon_buffer[0].size  =  RefRingBufSize;

	ret = AllocMemory(&recon_buffer[0].phys_addr ,recon_buffer[0].size,RECON_BUF_LUMA_ADDRESS);

	if ( ret != 0 )
	{
		return -1;
	}

#ifndef RTOS_MINI_SYSTEM
	recon_buffer[0].virt_addr = (ulong)mmap(0,
				 recon_buffer[0].size,
				 PROT_READ | PROT_WRITE,
				 MAP_SHARED,
				 fd,
				 recon_buffer[0].phys_addr);
#endif

#if LOG_PRINTF
	printf("#######Here SamSu debug mini system phys dram usage >> recon_buffer : size :0x%x,phys_addr:0x%lx\n",recon_buffer[0].size,recon_buffer[0].phys_addr);
#endif

	Debug_Dram_usage += recon_buffer[0].size;


#if LOG_PRINTF
	printf("recon_buffer mmap phys_addr 0x%lx, virt_addr 0x%lx\n",recon_buffer[0].phys_addr,recon_buffer[0].virt_addr);
#endif



	mini_sys_ringbuffer.refRingBufExtendHeight = RefRingBufExtendHeight;
	mini_sys_ringbuffer.refRingBuf_luma_size   = lumaBufSize;
	mini_sys_ringbuffer.refRingBuf_chroma_size = chromaBufSize;
	mini_sys_ringbuffer.refRingBuf_4n_size     = lumaBufSize4N;
	mini_sys_ringbuffer.refRingBuf_recon_chroma_half_size = recon_chroma_half_size;
	mini_sys_ringbuffer.pRefRingBuf_base       = recon_buffer[0].phys_addr;
	mini_sys_ringbuffer.ref_frame_stride = ref_frame_stride;
	mini_sys_ringbuffer.ref_frame_stride_ch = ref_frame_stride_ch;
	mini_sys_ringbuffer.ref_ds_luma_stride = ref_ds_luma_stride;
	//end


	//allocate compressor table here Only 2 reference
	CompressTableSize(0x3, context->width , context->height, &tblLumaSize,
							&tblChromaSize);

	tblSize = tblLumaSize + tblChromaSize;

	#if LOG_PRINTF
	printf("mem debug >> tblSize = 0x%x,tblLumaSize = 0x%x,tblChromaSize = 0x%x\n",tblSize,tblLumaSize,tblChromaSize);
	#endif

	compressTbl[0].size = tblSize;
	compressTbl[1].size = tblSize;

	ret = AllocMemory(&compressTbl[0].phys_addr ,compressTbl[0].size,COMPRESS_TBL);
	ret = AllocMemory(&compressTbl[1].phys_addr ,compressTbl[1].size,COMPRESS_TBL);
	#if LOG_PRINTF
	printf("#######Here SamSu debug mini system phys dram usage >> compressTbl0 : size :0x%x,phys_addr:0x%lx\n",compressTbl[0].size,compressTbl[0].phys_addr);
	printf("#######Here SamSu debug mini system phys dram usage >> compressTbl1 : size :0x%x,phys_addr:0x%lx\n",compressTbl[1].size,compressTbl[1].phys_addr);
		#endif
	Debug_Dram_usage += compressTbl[0].size*2;


	mini_sys_ringbuffer.tblSize = tblSize;
	mini_sys_ringbuffer.tblLumaSize = tblLumaSize;
	mini_sys_ringbuffer.tblChromaSize = tblChromaSize;

	mini_sys_ringbuffer.compressTbl_phys_luma_addr0 = compressTbl[0].phys_addr ;
	mini_sys_ringbuffer.compressTbl_phys_chroma_addr0 = compressTbl[0].phys_addr + tblLumaSize;

	mini_sys_ringbuffer.compressTbl_phys_luma_addr1 = compressTbl[1].phys_addr ;
	mini_sys_ringbuffer.compressTbl_phys_chroma_addr1 = compressTbl[1].phys_addr + tblLumaSize;

	// bitstream buffer

	bitstream_buffer[0].size  = BIT_STREAM_BUF_SIZE;

	ret = AllocMemory(&bitstream_buffer[0].phys_addr ,bitstream_buffer[0].size,VENC_BSB_BUF);


	if (ret != 0)
	{
		return -1;
	}

#ifndef RTOS_MINI_SYSTEM

	bitstream_buffer[0].virt_addr = (ulong)mmap(0,
				  bitstream_buffer[0].size,
				  PROT_READ | PROT_WRITE,
				  MAP_SHARED,
				  fd,
				  bitstream_buffer[0].phys_addr);
	memset((void*)bitstream_buffer[0].virt_addr,0,BIT_STREAM_BUF_SIZE);

#endif


#if LOG_PRINTF
	printf("bitstream_buffer mmap phys_addr0 0x%lx, virt_addr 0x%lx\n",bitstream_buffer[0].phys_addr,bitstream_buffer[0].virt_addr);

#endif

		#if LOG_PRINTF
	printf("#######Here SamSu debug mini system phys dram usage >> bitstream_buffer : size :0x%x,phys_addr:0x%lx\n",bitstream_buffer[0].size,bitstream_buffer[0].phys_addr);
#endif
	Debug_Dram_usage += bitstream_buffer[0].size;
		#if LOG_PRINTF
	printf("#######Here SamSu debug mini system phys dram usage >> Debug_Dram_usage size :0x%x\n",bitstream_buffer[0].phys_addr + bitstream_buffer[0].size - vcmd_buffer[0].phys_addr);

#endif

	//YUV ADDRESS
	//NV12
	raw_data.size  = context->stride_width * context->height * 3 / 2 *raw_data_number;

	ret = AllocMemory(&raw_data.phys_addr ,raw_data.size,LIMU_LUMA_ADDRESS);

	if ( ret != 0 )
	{
		return -1;
	}

#ifndef RTOS_MINI_SYSTEM
	raw_data.virt_addr = (ulong)mmap(0,
				raw_data.size,
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				fd,
				raw_data.phys_addr);
	memset((void*)raw_data.virt_addr,0, g_context.stride_width * g_context.height * 3 / 2*raw_data_number);
#endif


#if LOG_PRINTF
	printf("raw_data mmap phys_addr 0x%lx, virt_addr 0x%lx , size = 0x%x\n",raw_data.phys_addr,raw_data.virt_addr,raw_data.size);
#endif
////////////////////////////////////////////////////////////////////////////////////////

	return 0;

}




int mini_system_encoder_open(venc_context_t *context,int cmd_index)
{
	int ret = 0;
#ifndef	RTOS_MINI_SYSTEM
	memcpy((void*)vcmd_buffer[0].virt_addr, vce_vcmd[0], VCMD_MAX_SIZE);
	memcpy((void*)vcmd_buffer[1].virt_addr, vce_vcmd[1], VCMD_MAX_SIZE);
#endif
	Wr(VC9000E_VCMD_SWREG017, 0xffffffff);//Interrupt status/clr
	Wr(VC9000E_VCMD_SWREG018, 0x0000003f);
	Wr(VC9000E_VCMD_SWREG023, 0x00100000);//[31:28] axi_endian; [23:16] axi_burst_len; [15:8] axi_id_rd; [7:0] axi_id_wr
	Wr(VC9000E_VCMD_SWREG025, 0x00000004);//[15:0] abn_intr_gate; [31:16] norm_intr_gate
	Wr(VC9000E_VCMD_SWREG024, 0x00000001);//sw_rdy_cmdbuf_coun

	Wr(VC9000E_VCMD_SWREG020, vcmd_buffer[0].phys_addr );// cmdbuf_start_addr_lsb

	Wr(VC9000E_VCMD_SWREG021, 0x00000000);// cmdbuf_start_addr_msb

	Wr(VC9000E_VCMD_SWREG022, 0x106);//cmdbuf_size_in_64bit
	// Wr(VC9000E_VCMD_SWREG022, 0x2);//cmdbuf_size_in_64bit
	return ret;
}


int main(int argc, char *argv[])
{
    int ret = 0;

    char *ptr = NULL;
	int i,j;
	int head_size = 0;
	unsigned int filesize = 0;
	FILE *fp = NULL;
	unsigned int hw_status;

	int encoded_frame_num = 0;
#ifdef	RTOS_MINI_SYSTEM
	*(volatile unsigned int *)(WRAP_RESET_CTL)=0x2c0000;
	*(volatile unsigned int *)(WRAP_RESET_SEL)=0;
	*(volatile unsigned int *)(HW_RESET_CTL)=2;
	*(volatile unsigned int *)(WRAP_CLOCK_CTL)=0x6000400;
	*(volatile unsigned int *)(WRAP_CLOCK_CTL)=0x7000500;
#endif
	raw_data_number = 1;

	encoded_frame_num = 10;
	memset(&g_context,0,sizeof(venc_context_t));
    g_context.width = 2560;
    g_context.height = 1440;
	g_context.stride_width = g_context.width;
    g_context.encType = 1;
    g_context.fps = 15;
	g_context.gop = 15;
    g_context.quality = 26;

#ifndef	RTOS_MINI_SYSTEM
	mini_es_dump_init();

	fp = fopen("/data/2560p_10f.yuv","r+");

	if (fp == NULL) {
		return -1;
	}
	fseek(fp,0,SEEK_END);
	filesize = ftell(fp);
	rewind(fp);
#if LOG_PRINTF
	printf("read YUV DONE\n");
#endif
#endif
	ret = mini_system_encoder_init(&g_context);

	if (0 != ret)
	{
		return ret;
	}

#if LOG_PRINTF
	printf("mini_system_encoder_init Done, ret %d\n", ret);
#endif
	ret = mini_system_encoder_open(&g_context,0);

	//start encoding
	for (j=0;j<encoded_frame_num;j++)
	{
#ifndef	RTOS_MINI_SYSTEM
	   fread((void*)raw_data.virt_addr, g_context.stride_width * g_context.height * 3 / 2, 1, fp);
#endif
		mini_InnerLoop_Update(&g_context,j, &mini_sys_ringbuffer);

		vce_frame_start(&g_context,j,&mini_sys_ringbuffer,raw_data.phys_addr);

		for (i=0;i<50000000;i++) {
			;//for delay
		}
        mini_es_dump(0);
		printf("mini_system_encoder_init Done, encoded_frame_num %d\n", j);

	}
#ifndef	RTOS_MINI_SYSTEM
	fclose(fp);
	vce_es_dump_close();
#endif
	return 0;
}

