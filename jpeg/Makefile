OUT_DIR ?= .
$(info "OUT_DIR : $(OUT_DIR)")

ifneq ($(buildroot), )
export CC=$(shell find $(buildroot)/host/bin -name '*gnu-gcc' )
export CXX=$(shell find $(buildroot)/host/bin -name '*gnu-g++' )
export AR=$(shell find $(buildroot)/host/bin -name '*gnu-ar' )
export SYSROOT=$(shell find $(buildroot) -name sysroot )
CFLAGS+= --sysroot=$(SYSROOT)

$(warning CC=$(CC))
endif


#export CROSS_CC_PREFIX=aarch64-linux-gnu-
#export CC=${CROSS_CC_PREFIX}gcc
#export CXX=${CROSS_CC_PREFIX}g++
#export AR=${CROSS_CC_PREFIX}ar

TARGET=jpeg_enc_test
LDFLAGS += -lm -lrt  -L$(OUT_DIR)/jpeg_enc -ljpegenc_api
CFLAGS+=-g -I./jpeg_enc/include

$(TARGET):test.o test_dma.o
	$(CXX) $(CFLAGS) $(patsubst %, $(OUT_DIR)/%, $^) -o $(OUT_DIR)/$(TARGET) $(LDFLAGS)

#$(AMLENC_LIB):%.o:%.c
#	$(CXX) -c $(CFLAGS) $< -o $@
%.o: %.c
	$(CXX) $(CFLAGS) -c $< -o $(OUT_DIR)/$@
clean:
	-rm -f $(OUT_DIR)/*.o
	-rm -f $(OUT_DIR)/$(TARGET)
	@make "OUT_DIR=${OUT_DIR}" -C jpeg_enc/ clean
