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


CFLAGS +=-fPIC
CFLAGS += -g
LDFLAGS += -lm -lrt -shared

SUFFIXES += .d
NODEPS := clean distclean

C_SRCS := $(wildcard *.c) $(wildcard **/*.c)
C_OBJS := $(C_SRCS:%.c=%.o)

CXX_SRCS := $(wildcard *.cpp) $(wildcard **/*.cpp)
CXX_OBJS := $(CXX_SRCS:%.cpp=%.o)

#DEPFILES := $(patsubst %c,%d,$(C_SRCS))
#DEPFILES += $(patsubst %cpp,%d,$(CXX_SRCS))

INC = -I./include -I.
TARGET = libjpegenc_api.so

all: $(TARGET)

#ifeq "$(findstring $(MAKECMDGOALS),$(NODEPS))"  ""
#  -include $(DEPFILES)
#endif


$(TARGET): $(C_OBJS) $(CXX_OBJS)
	@echo " [LINK] $(OUT_DIR)/$@"
	$(CXX) $(LDFLAGS) -Wl,-soname,$(TARGET) -o $(OUT_DIR)/$@ $(patsubst %, $(OUT_DIR)/%, $^)

%.o: %.c
	@echo " [CXX] $<"
	@$(CC) -c $(CFLAGS) $(INC) $< -o $(OUT_DIR)/$@

%.o: %.cpp
	@echo " [CXX] $<"
	@$(CXX) -c $(CFLAGS) $(INC) $< -o $(OUT_DIR)/$@

%.d: %.cpp
	@echo " [DEP] $<"
	@$(CXX) $(CFLAGS) -MM -MT "$(patsubst %.cpp,%.o,$<)" $< -MF $(OUT_DIR)/$@

%.d: %.c
	@echo " [DEP] $<"
	@$(CC) $(CFLAGS) -MM -MT "$(patsubst %.c,%.o,$<)" $< -MF $(OUT_DIR)/$@

clean:
#	@rm -f $(DEPFILES)
	@rm -f $(patsubst %, $(OUT_DIR)/%, $(C_OBJS))
	@rm -f $(patsubst %, $(OUT_DIR)/%, $(CXX_OBJS))
	@rm -f $(OUT_DIR)/$(TARGET)

$(shell mkdir -p $(OUT_DIR))