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

$(TARGET).elf: vshader.shbin.o main.o
	$(CXX) $^ $(MY_LDFLAGS) $(MY_LIBS) -o $@

# --- EXPLICIT SHADER RULES ---
# Step 1: Turn text into a shader binary
vshader.shbin: vshader.v.pica
	picasso -o $@ $<

# Step 2: Turn binary into an object file and generate the header
vshader.shbin.o: vshader.shbin
	bin2s $< > vshader.shbin.s
	$(CXX) $(MY_ARCH) -c vshader.shbin.s -o $@
	echo "extern const u8 vshader_shbin[];" > vshader_shbin.h
	echo "extern const u32 vshader_shbin_size;" >> vshader_shbin.h

# Step 3: Tell main.o to wait for the header
vshader_shbin.h: vshader.shbin.o

main.o: main.cpp vshader_shbin.h
	$(CXX) $(MY_CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.elf *.3dsx *.shbin *.h *.s
