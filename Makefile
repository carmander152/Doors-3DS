TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

LIBS    := -lcitro3d -lctru -lm
LIBDIRS := $(DEVKITPRO)/libcitro3d $(DEVKITPRO)/libctru
export INCLUDE  := $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS  := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) -D__3DS__
CXXFLAGS := $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS  := -g $(ARCH) -Wl,-Map,$(notdir $(TARGET)).map

all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf

$(TARGET).elf: main.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBPATHS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *.o *.elf *.3dsx *.map
