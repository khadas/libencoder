# Make file to  for  encoder library for Linux platform
#
#
#--------------------------------------
.PHONY: CREATE_DIR clean all

OUT_DIR ?= .
$(info "OUT_DIR : $(OUT_DIR)")

CFLAGS += -O2 -std=c99
CFLAGS+= -fPIC -g

LDFLAGS =

#CFLAGS += -static
#CLFAGS += -Wno-implicit-function-declaration -Wno-format -Wl,--fatal-warning
#CLFAGS += -Wall -Werror

INC = -I./include -I. -I../vpuapi/include -I../vpuapi


CFLAGS  +=  $(INC) $(DEFINES) $(PLATFORM_FLAGS)
ARFLAGS += rcs
TARGET = libvpcodec.so

LDLIBS += -lm -lrt
# pthread
LDLIBS += -lpthread -lge2d
LIB= -L$(OUT_DIR)/../vpuapi -lamvenc_api


LDFLAGS += -shared

OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p

SOURCES = libvpmulti_codec.c  \
		  h264bitstream.c \
		  h265bitstream.c \
		  AML_MultiEncoder.c \

OBJECTNAMES=$(patsubst %.c,%.o, $(SOURCES))
OBJECTPATHS=$(addprefix $(OUT_DIR)/$(OBJDIR)/,$(notdir $(OBJECTNAMES)))


all: $(OBJECTNAMES)
	$(CC) -o $(OUT_DIR)/$(TARGET) $(LDFLAGS) $(LIB)  -Wl,-soname,$(TARGET) -Wl,-gc-section -Wl,--start-group $(OBJECTPATHS) $(LDLIBS) -Wl,--end-group

-include $(OBJECTPATHS:.o=.dep)

clean:
	$(RM) $(OUT_DIR)/$(TARGET)
	$(RM) $(OUT_DIR)/$(OBJDIR)/$(ALLOBJS)
	$(RM) $(OUT_DIR)/$(OBJDIR)/$(ALLDEPS)

%.o: %.c
	$(CC) $(CFLAGS) -Wall -c $< -o $(OUT_DIR)/$(OBJDIR)/$@ -MD -MF $(OUT_DIR)/$(OBJDIR)/$(@:.o=.dep)

$(shell mkdir -p $(OUT_DIR))
$(shell mkdir -p $(OUT_DIR)/$(OBJDIR))
