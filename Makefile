# 1. Setup the basic info
TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .

# 2. Grab the official 3DS build rules from the system
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

# 3. Setup the libraries
# Order is very important: Citro3D must come before Libctru
LIBS    := -lcitro3d -lctru -lm

# 4. Standard 3DS compiler flags
ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS  := -g -Wall -O2 -mword-relocations \
           -ffunction-sections -march=armv6k -mtune=mpcore -mfloat-abi=hard \
           $(include) -D__3DS__

CXXFLAGS := $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS  := -g $(ARCH)
LDFLAGS  := -g $(ARCH) -Wl,-Map,$(notdir $(TARGET)).map

# 5. Tell the compiler where the libraries are
LIBDIRS := $(DEVKITPRO)/libcitro3d $(DEVKITPRO)/libctru

export INCLUDE  := $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

# 6. Build Instructions
all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf

$(TARGET).elf: main.o

# This rule ensures we use the 3DS-specific linker
%.elf:
	$(CXX) $(LDFLAGS) -o $@ $(LIBPATHS) $^ $(LIBS)

clean:
	rm -f *.o *.elf *.3dsx *.map
