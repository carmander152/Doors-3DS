TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .

# 1. Setup devkitPro Paths
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

# 2. Tell the compiler where the 3DS libraries are hiding
LIBCTRU     := $(DEVKITPRO)/libctru
CINCLUDES   := -I$(LIBCTRU)/include -I$(DEVKITPRO)/libcitro3d/include
LIBDIRS     := -L$(LIBCTRU)/lib -L$(DEVKITPRO)/libcitro3d/lib

# 3. Setup the Libraries (The order matters here!)
LIBS        := -lcitro3d -lctru -lm

# 4. Apply the settings to the build process
CFLAGS      += -g -Wall -O2 -mfloat-abi=hard -march=armv6k -mtune=mpcore -marm $(CINCLUDES)
CXXFLAGS    += $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS     += -g $(ARCH) $(LIBDIRS)

# 5. Build Rules
all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf
$(TARGET).elf: main.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)
