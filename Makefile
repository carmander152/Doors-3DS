# Project Name
TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .

# 1. Pull in the official devkitPro 3DS rules
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

# 2. Setup the libraries
LIBS    := -lcitro3d -lctru -lm

# 3. Setup the search paths
LIBDIRS := $(DEVKITPRO)/libcitro3d $(DEVKITPRO)/libctru
export INCLUDE  := $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

# 4. Standard 3DS flags
ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS  := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) -D__3DS__
CXXFLAGS := $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS  := -g $(ARCH) -Wl,-Map,$(notdir $(TARGET)).map

# 5. Build Rules
all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf

$(TARGET).elf: main.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBPATHS) $(LIBS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *.o *.elf *.3dsx *.map
