/*
 * Virtual Hantro Device Driver
 *
 * Copyright 2017 Milo Kim <woogyom.kim@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* needed for __init,__exit directives */
#include <linux/ioctl.h>
/* needed for remap_page_range
 *   SetPageReserved
 *   ClearPageReserved
 */
#include <linux/mm.h>
/* obviously, for kmalloc */
#include <linux/slab.h>
/* for struct file_operations, register_chrdev() */
#include <linux/fs.h>
/* standard error codes */
#include <linux/errno.h>
#include <linux/module.h>

#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/moduleparam.h>
/* request_irq(), free_irq() */
#include <linux/interrupt.h>
#include <linux/sched.h>

#include <linux/semaphore.h>
#include <linux/spinlock.h>
/* needed for virt_to_phys() */
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>

#include <asm/irq.h>

#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/delay.h>

/* our own stuff */
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

#include <linux/dma-mapping.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/moduleparam.h>
#include <linux/of_address.h>
#include <linux/kthread.h>
#include <linux/ioport.h>

#define HANTRO_IOC_MAGIC 'k'

#define HANTRO_IOC_GET_BUFFER _IOR(HANTRO_IOC_MAGIC, 1, int)
#define HANTRO_IOC_MAXNR 1

#define VCMD_MAX_SIZE 0x260
#define CORE_REG_NUM 512
#define VCMD_BUF_NUM 3
#define BSTM_BUF_NUM 3
#define ROI_BUF_NUM 12
#define HW_REG_SIZE 0x400000
#define BIT_STREAM_BUF_SIZE 0x400000



#define WRAP_SOC_SYSTEM_BASE 0xfe000000
#define WRAP_SOC_SYSTEM_SIZE 0x400000



#define WRAP_RESET_CTL 0xfe002004
#define WRAP_RESET_SEL 0xfe096000
#define WRAP_CLOCK_CTL 0xfe00019c
#define HW_VCMD_BASE   0xfe310000
#define HW_CORE_BASE   0xfe311000
#define HW_RESET_CTL   0xfe310040
#define HW_VCMD_SIZE   0x1000

//FIXED ME
#define ALLOC_PHY_MEM_ADDR   0x60000000
#define ALLOC_PHY_MEM_SIZE   0x2e00000   //30M byte






#define REG_ID            0x0

#define REG_INIT        0x4
#define HW_ENABLE        BIT(0)

#define REG_CMD            0x8

#define REG_INT_STATUS     0x
#define IRQ_ENABLED        BIT(0)
#define IRQ_BUF_DEQ        BIT(1)
#define IRQ_FRAME_DONE     BIT(2)

#define REG_IRQ                0x311004
#define REG_ENC_SIZE           0x311024
#define REG_ENC_ADDR           0x311020

#define DEVICE_NAME "hantro"
#define CLASS_NAME "venc"
#define IRQ_NUM 134

#define LOG_ALL 0
#define LOG_INFO 1
#define LOG_DEBUG 2
#define LOG_ERROR 3



#define LINMEM_ALIGN    4096
#define NEXT_ALIGNED(x) ((((ptr_t)(x)) + LINMEM_ALIGN-1)/LINMEM_ALIGN*LINMEM_ALIGN)
#define NEXT_ALIGNED_SYS(x, align) ((((ptr_t)(x)) + align - 1) / align * align)
#define pr_log(format,...) printk(""format"\n", ##__VA_ARGS__)

typedef struct{
	unsigned int size;
	unsigned int cached;
	unsigned long phys_addr;
	unsigned long base; /* kernel logical address in use kernel */
	unsigned long virt_addr; /* virtual user space address */
} hantro_buffer_t;

int irq = -1;
unsigned int print_level;
unsigned long vc9000e_reg_base;
unsigned long reg_base = HW_VCMD_BASE;
#define enc_pr(level, x...) \
	do { \
		if (level >= print_level) \
			printk(x); \
	} while (0)

struct virt_hantro {
    struct device *dev;
    void __iomem *base;
};

static struct device*  device;
static int major;
static unsigned int *sh_mem = NULL;
static hantro_buffer_t common_buffer;
static hantro_buffer_t hw_regs;

static DEFINE_MUTEX(hantro_mutex);

static ssize_t vf_show_id(struct device *dev,
              struct device_attribute *attr, char *buf)
{
    struct virt_hantro *vf = dev_get_drvdata(dev);
    u32 val = readl_relaxed(vf->base + REG_ID);

    return scnprintf(buf, PAGE_SIZE, "Chip ID: 0x%.x\n", val);
}

static ssize_t vf_show_bitstream_dump(struct device *dev,
               struct device_attribute *attr, char *buf)
{
    return 0;
}

static ssize_t vf_store_bitstream_dump(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t len)
{
    struct virt_hantro *vf = dev_get_drvdata(dev);
    unsigned long val;

    if (kstrtoul(buf, 0, &val))
        return -EINVAL;

    writel_relaxed(val, vf->base + REG_CMD);

    return len;
}

static DEVICE_ATTR(id, S_IRUGO, vf_show_id, NULL);
static DEVICE_ATTR(bitstream_dump, S_IRUGO | S_IWUSR, vf_show_bitstream_dump, vf_store_bitstream_dump);

static struct attribute *vf_attributes[] = {
    &dev_attr_id.attr,
    &dev_attr_bitstream_dump.attr,
    NULL,
};

static const struct attribute_group vf_attr_group = {
    .attrs = vf_attributes,
};

extern uint32_t frm_num;



static irqreturn_t vf_irq_handler(int irq, void *data)
{
	#if 1
    struct virt_hantro *vf = (struct virt_hantro *)data;
    u32 status;
	u32 status_vcmd1 = 0;
	u32 status_vcmd2 = 0;
    status = readl_relaxed(vf->base + REG_IRQ);
	printk("\n............vf_irq_handler,status core = 0x%x\n",status);

	status_vcmd1 = readl_relaxed(vf->base + 0x310008);
	printk("\n............vf_irq_handler,status_vcmd 0x8 = 0x%x\n",status_vcmd1);
	status_vcmd2 = readl_relaxed(vf->base + 0x310044);
	printk("\n............vf_irq_handler,status_vcmd 0x44 = 0x%x\n",status_vcmd2);

	if (status & IRQ_FRAME_DONE)
	{
		printk( "irq frame done, frame size : %d\n", readl_relaxed(vf->base + REG_ENC_SIZE));
        writel_relaxed(0x4, vf->base + REG_IRQ);
	}

    if (status & IRQ_BUF_DEQ)
        printk( "Command buffer is dequeued\n");
	#endif
    return IRQ_HANDLED;
}

/* executed once the device is opened.
 *
 */
static int hantro_open(struct inode *inodep, struct file *filep)
{
    int ret = 0;

	if (!mutex_trylock(&hantro_mutex)) {
		pr_alert("hantro: device busy!\n");
        ret = -EBUSY;
        goto out;
    }

out:
    return ret;
}

/*  executed once the device is closed or releaseed by userspace
 *  @param inodep: pointer to struct inode
 *  @param filep: pointer to struct file
 */
static int hantro_release(struct inode *inodep, struct file *filep)
{
    mutex_unlock(&hantro_mutex);
    return 0;
}

static int hantro_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int ret = 0;
    unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);

    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    ret = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot);
	return ret;
}

static long hantro_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

	switch (cmd)
	{
		case HANTRO_IOC_GET_BUFFER:

			ret = copy_to_user((void __user *)arg, &common_buffer, sizeof(hantro_buffer_t));
			if (ret) {
				ret = -EFAULT;
			}
			break;

		default:
		return -EINVAL;
	}

	return ret;
}

static ssize_t vcmd_ctrl_show(struct class *cla,
				struct class_attribute *attr, char *buf)
{
	char *pbuf = buf;
    unsigned int *vcmd_buffer=(unsigned int *)sh_mem;
	pbuf +=  sprintf(pbuf, "amrisc registers show: size: 0x%08x\n", vcmd_buffer[0]);
	pbuf +=  sprintf(pbuf, "amrisc registers show: size: 0x%08x\n", vcmd_buffer[520]);
	return pbuf - buf;
}

static CLASS_ATTR_RO(vcmd_ctrl);
static struct attribute *venc_class_attrs[] = {
	&class_attr_vcmd_ctrl.attr,
	NULL
};

ATTRIBUTE_GROUPS(venc_class);
static struct class venc_class = {
	.name = CLASS_NAME,
	.class_groups = venc_class_groups,
};

static const struct file_operations hantro_fops = {
	.owner = THIS_MODULE,
	.open = hantro_open,
	.mmap = hantro_mmap,
	.release = hantro_release,
	.unlocked_ioctl = hantro_ioctl,
	//.poll = hantro_poll,
};

static int vf_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    unsigned int *kvirt_addr = 0;
    struct virt_hantro *vf;
    int ret = 0;





	enc_pr(LOG_DEBUG, "vf_probe\n");
    vf = devm_kzalloc(dev, sizeof(*vf), GFP_KERNEL);
    if (!vf)
        return -ENOMEM;



    vf->dev = dev;
	//remaping the system base address
    vf->base = devm_ioremap(dev, WRAP_SOC_SYSTEM_BASE, WRAP_SOC_SYSTEM_SIZE);
	//end
    pr_log("base register base addr 0xfe000000,kernel virtual addr 0x%lx\n", (unsigned long)vf->base);



	if (!vf->base) {
        return -EINVAL;
    }


	//system bringup
    kvirt_addr = devm_ioremap(dev, WRAP_RESET_CTL, 4);
    *kvirt_addr = 0x2c0000;
    kvirt_addr = devm_ioremap(dev, WRAP_RESET_SEL, 4);
    *kvirt_addr = 0;
    kvirt_addr = devm_ioremap(dev, HW_RESET_CTL, 4);
    *kvirt_addr = 2;
    kvirt_addr = devm_ioremap(dev, WRAP_CLOCK_CTL, 4);
    *kvirt_addr = 0x6000400;
    kvirt_addr = devm_ioremap(dev, WRAP_CLOCK_CTL, 4);
    *kvirt_addr = 0x7000500;
	//end

	/* get interrupt resource */
	irq = platform_get_irq_byname(pdev, "vc9000e_irq0");

	enc_pr(LOG_DEBUG, "hantro -irq: %d\n", irq);

	if (irq < 0) {
		enc_pr(LOG_ERROR, "get hantro irq resource error\n");
        return -EFAULT;
	} else {
        ret = request_irq(irq, vf_irq_handler, IRQF_SHARED, "vc9000e_irq0", (void *)(vf));

	if (ret) {
			enc_pr(LOG_ERROR, "Failed to register irq handler\n");
			return -EFAULT;
        }
    }
	enc_pr(LOG_DEBUG, "request_irq Done -irq: %d\n", irq);


	enc_pr(LOG_DEBUG, "device driver:register_chrdev\n");
    major = register_chrdev(0, DEVICE_NAME, &hantro_fops);

    printk("probe major %d\n", major);
    if (major < 0) {
        pr_info("hantro: fail to register major number!");
        ret = major;
        goto out;
    }

	enc_pr(LOG_DEBUG, "device driver:class_register\n");

	ret = class_register(&venc_class);
	if (ret < 0) {
		pr_info("hantro: error create venc class!");
		return ret;
	}

    device = device_create(&venc_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(device)) {
        class_destroy(&venc_class);
        unregister_chrdev(major, DEVICE_NAME);
        ret = PTR_ERR(device);
        goto out;
    }
	enc_pr(LOG_DEBUG, "device driver:device_create\n");

    mutex_init(&hantro_mutex);

    platform_set_drvdata(pdev, vf);

    ret = sysfs_create_group(&dev->kobj, &vf_attr_group);


    // hw register map
    hw_regs.phys_addr       = reg_base;
    hw_regs.size            = HW_VCMD_SIZE;
    vc9000e_reg_base        = (unsigned long)devm_ioremap(dev, hw_regs.phys_addr, hw_regs.size);
    pr_log("vc9000e_reg_base register base addr 0x%lx,kernel virtual addr 0x%lx\n", hw_regs.phys_addr,vc9000e_reg_base);

	//encoder phy address
	common_buffer.phys_addr = ALLOC_PHY_MEM_ADDR;
	common_buffer.size = ALLOC_PHY_MEM_SIZE;
	//end


out:
    return ret;
}

static int vf_remove(struct platform_device *pdev)
{
    struct virt_hantro *vf = platform_get_drvdata(pdev);
	enc_pr(LOG_DEBUG, "vf_remove\n");

    free_irq(irq, vf);
    sysfs_remove_group(&vf->dev->kobj, &vf_attr_group);

    if (device)
		device_destroy(&venc_class, MKDEV(major, 0));

	class_destroy(&venc_class);
	enc_pr(LOG_DEBUG, "venc_class\n");

	unregister_chrdev(major, DEVICE_NAME);
    return 0;
}

static const struct of_device_id vf_of_match[] = {
    { .compatible = "hantro", },
    { }
};

static struct platform_driver hantro_driver = {
    .probe = vf_probe,
    .remove = vf_remove,
    .driver = {
        .name = "hantro",
        .owner = THIS_MODULE,
        .of_match_table = vf_of_match,
    },
};

static s32 __init hantro_init(void)
{
	s32 res;

	enc_pr(LOG_DEBUG, "opanera hantro_init\n");
	res = platform_driver_register(&hantro_driver);
	enc_pr(LOG_DEBUG, "hantro_init ret %d\n", res);
	return res;
}

static void __exit hantro_exit(void)
{
	enc_pr(LOG_DEBUG, "opanera hantro_exit\n");
	platform_driver_unregister(&hantro_driver);
}

MODULE_DEVICE_TABLE(of, vf_of_match);

module_param(print_level, uint, 0664);
MODULE_PARM_DESC(print_level, "\n print_level\n");
module_param(reg_base, ulong, 0664);
MODULE_PARM_DESC(reg_base, "\n reg_base\n");

MODULE_AUTHOR("Amlogic Inc.");
MODULE_DESCRIPTION("HANTRO linux driver");
MODULE_LICENSE("GPL");

module_init(hantro_init);
module_exit(hantro_exit);
