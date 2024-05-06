# Make file to  for  encoder library for Linux platform
#
#
#--------------------------------------
.PHONY: CREATE_DIR clean all

OUT_DIR ?= .
$(info "OUT_DIR : $(OUT_DIR)")

ifeq ($(ARM), 0)
CFLAGS += -O2 -std=c99
else
CFLAGS += -DARM
endif

CFLAGS += -fPIC -g

LDFLAGS =

#CFLAGS += -static
#CLFAGS += -Wno-implicit-function-declaration -Wno-format -Wl,--fatal-warning
#CLFAGS += -Wall -Werror

INC = -I./include -I. -I../vpuapi/include -I../vpuapi


CFLAGS  +=  $(INC)
ARFLAGS += rcs
TARGET = aml_enc_test

LDLIBS += -L$(OUT_DIR)/../vpuapi -L$(OUT_DIR)/../amvenc_lib
LDLIBS += -lm -lrt -lamvenc_api -lvpcodec
# pthread
LDLIBS += -lpthread -lion


#LDFLAGS += -shared

OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p

SOURCES = test.c test_dma.c
OBJECNAMES = test.o test_dma.o

OBJECTNAMES=$(patsubst %.c,%.o, $(SOURCES))
OBJECTPATHS=$(addprefix $(OUT_DIR)/$(OBJDIR)/,$(notdir $(OBJECTNAMES)))


$(TARGET): $(OBJECTNAMES)
	$(CC) -o $(OUT_DIR)/$(TARGET) $(LDFLAGS) $(LDLIBS) $(OBJECTPATHS)

%.o:%.c
	$(CC) $(LDFLAGS) -c $< -o $(OUT_DIR)/$(OBJDIR)/$@

-include $(OBJECTPATHS:.o=.dep)

clean:
	$(RM) $(OUT_DIR)/$(TARGET)
	$(RM) $(OUT_DIR)/$(OBJDIR)/$(ALLOBJS)
	$(RM) $(OUT_DIR)/$(OBJDIR)/$(ALLDEPS)

$(shell mkdir -p $(OUT_DIR))
$(shell mkdir -p $(OUT_DIR)/$(OBJDIR))
