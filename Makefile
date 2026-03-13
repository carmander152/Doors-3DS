TARGET := Doors3DS

include $(DEVKITARM)/3ds_rules

MY_ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
MY_INC  := -I$(DEVKITPRO)/libcitro3d/include -I$(DEVKITPRO)/libctru/include
MY_LIB  := -L$(DEVKITPRO)/libcitro3d/lib -L$(DEVKITPRO)/libctru/lib

MY_CXXFLAGS := $(MY_ARCH) -D__3DS__ -O2 -fno-exceptions -fno-rtti $(MY_INC)
MY_LDFLAGS  := -specs=3dsx.specs $(MY_ARCH) $(MY_LIB)
MY_LIBS     := -lcitro3d -lctru -lm

all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf

# We added vshader.shbin.o so the compiler builds the shader!
$(TARGET).elf: main.o vshader.shbin.o
	$(CXX) $^ $(MY_LDFLAGS) $(MY_LIBS) -o $@

# We tell main.o to wait for the shader header to be built first
main.o: main.cpp vshader.shbin.h
	$(CXX) $(MY_CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.elf *.3dsx *.shbin *.h
