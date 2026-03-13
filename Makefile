# 1. Target Name
TARGET := Doors3DS

# 2. Get official rules for the .3dsx conversion
include $(DEVKITARM)/3ds_rules

# 3. FORCE the 3DS Math and Memory Rules
MY_ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
MY_INC  := -I$(DEVKITPRO)/libcitro3d/include -I$(DEVKITPRO)/libctru/include
MY_LIB  := -L$(DEVKITPRO)/libcitro3d/lib -L$(DEVKITPRO)/libctru/lib

# 4. FORCE the Compiler and Linker instructions
MY_CXXFLAGS := $(MY_ARCH) -D__3DS__ -O2 -fno-exceptions -fno-rtti $(MY_INC)
MY_LDFLAGS  := -specs=3dsx.specs $(MY_ARCH) $(MY_LIB)
MY_LIBS     := -lcitro3d -lctru -lm

# 5. Build Steps
all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf

$(TARGET).elf: main.o vshader.shbin.o
	$(CXX) $^ $(MY_LDFLAGS) $(MY_LIBS) -o $@

# FIX: Notice it is vshader_shbin.h with an UNDERSCORE now!
main.o: main.cpp vshader_shbin.h
	$(CXX) $(MY_CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.elf *.3dsx *.shbin *.h
