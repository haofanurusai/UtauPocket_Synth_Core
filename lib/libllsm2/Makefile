# INC 中 -I.. 指向 lib/ 父目录，使得 #include "libllsm2/llsm.h" 可从 lib/ 下查找。
MODULE_NAME = libllsm2.a
OUT_DIR     = $(BUILD_DIR_ABS)/lib/libllsm2
TARGET      = $(OUT_DIR)/$(MODULE_NAME)

SRCS = coder.c \
       container.c \
       dsputils.c \
       frame.c \
       layer0.c \
       layer1.c \
       llsmrt.c \
       llsmutils.c

OBJS = $(SRCS:%.c=$(OUT_DIR)/%.o)
INC  = -I. -I..

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(OUT_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@
