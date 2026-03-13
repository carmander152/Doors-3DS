# -----------------------------------------------------------------------------
# 1. Basic Project Info
# -----------------------------------------------------------------------------
TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .

# -----------------------------------------------------------------------------
# 2. devkitPro Architecture & Rules
# -----------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

# These are the magic flags that prevent the "Non-contiguous segments" error
ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
export ASFLAGS  := -g $(ARCH)
export CFLAGS   := -g -Wall -O2 -mword-relocations \
                   -ffunction-sections $(ARCH) -D__3DS__
export CXXFLAGS := $(CFLAGS) -fno-exceptions -fno-rtti
export LDFLAGS  := -g $(ARCH) -Wl,-Map,$(notdir $(TARGET)).map

# -----------------------------------------------------------------------------
# 3. Libraries (Order matters: Citro3D before libctru)
# -----------------------------------------------------------------------------
LIBS    := -lcitro3d -lctru -lm
LIBDIRS := $(DEVKITPRO)/libcitro3d $(DEVKITPRO)/libctru

export INCLUDE  := $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

# -----------------------------------------------------------------------------
# 4. Build Instructions
# -----------------------------------------------------------------------------
all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf

$(TARGET).elf: main.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBPATHS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *.o *.elf *.3dsx *.map
