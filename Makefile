# Linux/GNU make. Windows builds use UtauPocket.sln.
CROSS_COMPILE ?=
ifeq ($(origin CC),default)
CC = $(CROSS_COMPILE)gcc
endif
ifeq ($(origin CXX),default)
CXX = $(CROSS_COMPILE)g++
endif
ifeq ($(origin AR),default)
AR = $(CROSS_COMPILE)ar
endif
PRECISION ?= float
ifneq ($(filter $(PRECISION),float double),$(PRECISION))
$(error PRECISION must be float or double)
endif
BUILD_DIR ?= build/$(PRECISION)
OPT ?= -O3
CPPFLAGS += -DFP_TYPE=$(PRECISION) -include include/utau_pocket_config.h -Iinclude -Isrc -Ilib -Ilib/ciglet -Ilib/ciglet/external -Ilib/libllsm2 -Ilib/libpyin -Ilib/llsm_file_utils -Ilib/arduino_midi_library/src
COMMON = $(OPT) -g -Wall -Wextra -ffunction-sections -fdata-sections -fvisibility=hidden
CFLAGS += $(COMMON) -std=c11
CXXFLAGS += $(COMMON) -std=c++14 -fvisibility-inlines-hidden
CORE_CPP = $(wildcard src/*.cpp)
LLSM_C = $(addprefix lib/libllsm2/,coder.c container.c dsputils.c frame.c layer0.c layer1.c llsmrt.c llsmutils.c)
DSP_C = lib/ciglet/ciglet.c lib/ciglet/external/fast_median.c lib/ciglet/external/fftsg_h.c
CORE_C = $(LLSM_C) $(DSP_C) lib/llsm_file_utils/llsm_file_utils.c
TOOL_C = $(CORE_C) lib/ciglet/external/wavfile.c $(wildcard lib/libpyin/*.c) $(wildcard lib/libgvps/*.c)
CORE_OBJS = $(addprefix $(BUILD_DIR)/obj/,$(CORE_CPP:.cpp=.o) $(CORE_C:.c=.o))
PIC_OBJS = $(addprefix $(BUILD_DIR)/pic/,$(CORE_CPP:.cpp=.o) $(CORE_C:.c=.o))
TOOL_OBJS = $(addprefix $(BUILD_DIR)/obj/,$(TOOL_C:.c=.o)) $(BUILD_DIR)/obj/utils/wav2llsm/wav2llsm.o
STATIC = $(BUILD_DIR)/lib/libutau_pocket.a
SHARED = $(BUILD_DIR)/lib/libutau_pocket.so
TOOL = $(BUILD_DIR)/bin/wav2llsm
PLAYER_DIR = apps/player
PLAYER_CPP = $(addprefix $(PLAYER_DIR)/,main.cpp cmd_parser.cpp midi_serial.cpp audio_tinyalsa.cpp)
PLAYER_C = $(addprefix $(PLAYER_DIR)/vendor/tinyalsa/src/,pcm.c pcm_hw.c snd_card_plugin.c limits.c)
PLAYER_OBJS = $(addprefix $(BUILD_DIR)/player/,$(PLAYER_CPP:.cpp=.o) $(PLAYER_C:.c=.o))
PLAYER = $(BUILD_DIR)/bin/utau_pocket
PLAYER_INCLUDES = -I$(PLAYER_DIR) -I$(PLAYER_DIR)/vendor/tinyalsa/include
.PHONY: all static shared utils wav2llsm player utau_pocket clean
all: static shared utils player
static: $(STATIC)
shared: $(SHARED)
utils wav2llsm: $(TOOL)
player utau_pocket: $(PLAYER)
$(PLAYER): $(PLAYER_OBJS) $(STATIC)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -Wl,--gc-sections -o $@ $(PLAYER_OBJS) $(STATIC) -lm -ldl
$(BUILD_DIR)/player/%.o: %.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(PLAYER_INCLUDES) $(CXXFLAGS) -MMD -MP -c $< -o $@
$(BUILD_DIR)/player/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(PLAYER_INCLUDES) -D_GNU_SOURCE $(CFLAGS) -MMD -MP -c $< -o $@
$(STATIC): $(CORE_OBJS) Makefile
	@mkdir -p $(@D)
	rm -f $@
	$(AR) rcs $@ $(CORE_OBJS)
$(SHARED): $(PIC_OBJS) utau_pocket.map Makefile
	@mkdir -p $(@D)
	$(CXX) -shared $(LDFLAGS) -Wl,--gc-sections,--no-undefined,--version-script=utau_pocket.map -o $@ $(PIC_OBJS) -lm
$(TOOL): $(TOOL_OBJS) Makefile
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -Wl,--gc-sections -o $@ $(TOOL_OBJS) -lm
$(BUILD_DIR)/obj/%.o: %.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@
$(BUILD_DIR)/obj/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
$(BUILD_DIR)/pic/%.o: %.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DUTAU_POCKET_BUILD -DUTAU_POCKET_SHARED -fPIC -MMD -MP -c $< -o $@
$(BUILD_DIR)/pic/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -MMD -MP -c $< -o $@
clean:
	rm -rf $(BUILD_DIR)
-include $(CORE_OBJS:.o=.d) $(PIC_OBJS:.o=.d) $(TOOL_OBJS:.o=.d) $(PLAYER_OBJS:.o=.d)
