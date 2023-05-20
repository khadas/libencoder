obj-m := hantro.o
hantro-objs += mini_driver.o

default::
	$(MAKE) -C  $(KERNEL_SRC) M=`pwd` modules "EXTRA_CFLAGS+=-I$(INCLUDE) -Wno-error $(CONFIGS_BUILD) $(EXTRA_INCLUDE) $(KBUILD_CFLAGS_MODULE) ${VERSION_CONTROL_CFLAGS}" $(CONFIGS)
