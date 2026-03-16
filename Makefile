ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment.")
endif

include $(DEVKITPRO)/devkitARM/3ds_rules

TARGET := hotel_doors
OBJS := vshader.shbin.o main.o
LIBS := -L$(DEVKITPRO)/libcitro3d/lib -L$(DEVKITPRO)/libctru/lib -lcitro3d -lctru -lm

# Define the RomFS directory
ROMFS_DIR := romfs

.PHONY: all clean

# Build both the 3dsx and the smdh metadata file
all: $(TARGET).3dsx $(TARGET).smdh

# Generate the SMDH metadata file (creates a default icon)
$(TARGET).smdh:
	smdhtool --create "Hotel Doors" "DOORS Fan Game" "YourName" $@

# Compile the .3dsx, bundling the SMDH and RomFS
$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool $< $@ --smdh=$(TARGET).smdh --romfs=$(ROMFS_DIR)

# Compile the ELF binary
$(TARGET).elf: $(OBJS)
	$(CXX) -specs=3dsx.specs -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -o $@ $^ $(LIBS)

# Compile main.cpp
main.o: main.cpp vshader_shbin.h
	$(CXX) -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -D__3DS__ -O2 -fno-exceptions -fno-rtti -I$(DEVKITPRO)/libcitro3d/include -I$(DEVKITPRO)/libctru/include -c $< -o $@

# Compile the shader
vshader.shbin.o: vshader.v.pica
	picasso -o vshader.shbin $<
	bin2s vshader.shbin > vshader.shbin.s
	$(AS) -march=armv6k -mfloat-abi=hard -o $@ vshader.shbin.s

# Generate the shader header
vshader_shbin.h: vshader.shbin
	echo "extern const u8 vshader_shbin[];" > $@
	echo "extern const u32 vshader_shbin_size;" >> $@

# Clean up generated files
clean:
	rm -f $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(OBJS) vshader.shbin vshader.shbin.s vshader_shbin.h
