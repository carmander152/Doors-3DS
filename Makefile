# 1. Project Setup
TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .

# 2. Grab the Official Rules (CRITICAL: This defines the linker map)
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

# 3. Setup Libraries
LIBS    := -lcitro3d -lctru -lm

# 4. Search Paths
LIBDIRS := $(DEVKITPRO)/libcitro3d $(DEVKITPRO)/libctru
export INCLUDE  := $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

# 5. Architecture Flags (Fixes the VFP/Register errors)
ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

# 6. Build Rules (The indented lines MUST start with a Tab)
all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf

$(TARGET).elf: main.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBPATHS) $(LIBS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *.o *.elf *.3dsx *.map
