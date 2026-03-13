# 1. Target Name
TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .

# 2. devkitPro Paths
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

# 3. 3DS Specific Architecture Flags (The Magic Fix)
ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

# 4. Libraries and Includes
LIBCTRU     := $(DEVKITPRO)/libctru
CINCLUDES   := -I$(INCLUDES) -I$(LIBCTRU)/include -I$(DEVKITPRO)/libcitro3d/include
LIBDIRS     := -L$(LIBCTRU)/lib -L$(DEVKITPRO)/libcitro3d/lib

# 5. Compiler & Linker Flags
CFLAGS      := $(ARCH) -O2 -mword-relocations -ffunction-sections $(CINCLUDES) -D__3DS__
CXXFLAGS    := $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS     := $(ARCH) -Wl,--gc-sections -Wl,-Map,$(notdir $(TARGET)).map $(LIBDIRS)

LIBS        := -lcitro3d -lctru -lm

# 6. Build Rules
all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf

$(TARGET).elf: main.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.elf *.3dsx *.map
