
ROOT_KDIR=/mnt/fileroot/yang.su/C3/output/c3_aw419_a32_release/build/
ROOT_COMPILE=/mnt/fileroot/yang.su/C3/toolchain/gcc/linux-x86/arm/

KDIR=$(ROOT_KDIR)linux-amlogic-5.15-dev

COMPILE_TOOL=$(ROOT_COMPILE)gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-

#COMPILE_TOOL=/opt/gcc-linaro-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-


CC=$(COMPILE_TOOL)gcc
LD=$(COMPILE_TOOL)ld
AR=$(COMPILE_TOOL)ar
OBJCOPY=$(COMPILE_TOOL)objcopy
ARCH=arm

HEX2BIN = hex2bin.c


MINI_ENC += mini_test.c


MINI_ENC += mini_memory_pool.c


DATA += vce_vcmd0.bin
DATA += vce_vcmd1.bin
DATA += header_2048_1536.bin


#source search path
vpath %.c
vpath %.c ./mini_enc_sample
vpath %.bin ./data

TEST_OBJ +=$(MINI_ENC:.c=.o)
TEST_BIN +=$(DATA:.bin=.o)

OBJ_PATH = ./build
ALL_OBJ_POS = $(addprefix $(OBJ_PATH)/,$(TEST_OBJ))
ALL_BIN_POS = $(addprefix $(OBJ_PATH)/,$(TEST_BIN))

PWD = $(shell pwd)


DRIVER := $(PWD)/driver

INCLUDE = -I./inc

TARGET=$(OBJ_PATH)/mini_enc_sample

all: kernel_modules mini_enc_sample
	#adb push build/mini_enc_sample /
	#adb push driver/hantro.ko /

kernel_modules:
	@$(MAKE) -C $(KDIR) M=$(DRIVER) modules ARCH=$(ARCH) COMMON_DRIVERS_DIR=common_drivers CC=$(CC) LD=$(LD) AR=$(AR)

tools:
	@gcc hex2bin/hex2bin.c -o $(OBJ_PATH)/hex2bin
	@echo build/hex2bin

mini_enc_sample:  $(ALL_OBJ_POS) $(ALL_BIN_POS)
	@$(CC) $(CFLAGS) $(ALL_OBJ_POS) $(ALL_BIN_POS) -o $(TARGET)
	@echo build/mini_enc_sample

data: tools $(DATA_BIN) $(DATA_OBJ)

$(OBJ_PATH)/%.o: %.c
	@$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@
	@echo $@

$(OBJ_PATH)/%.o: data/%.bin
	@$(OBJCOPY) --input-target=binary --output-target=elf32-littlearm $^ $@
	@$(OBJCOPY) \
		--strip-symbol  _binary_data_$(notdir $(basename $@))_bin_end \
		--strip-symbol  _binary_data_$(notdir $(basename $@))_bin_size \
		--redefine-sym  _binary_data_$(notdir $(basename $@))_bin_start=$(notdir $(basename $@)) \
		$@ $@
	@echo $^ $@

clean:
	@$(MAKE) -C $(KDIR) M=$(DRIVER) clean
	rm -f build/*
