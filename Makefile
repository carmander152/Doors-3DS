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

# Build both the 3dsx (Homebrew Launcher) and the cia (Home Screen)
all: $(TARGET).3dsx $(TARGET).cia

# ---------------------------------------------------
# 3DSX Build (Homebrew Launcher)
# ---------------------------------------------------

$(TARGET).smdh: icon.png
	smdhtool --create "Doors 3DS" "Remake of LSplash's Roblox game Doors" "carmander152" icon.png $@

$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool $< $@ --smdh=$(TARGET).smdh --romfs=$(ROMFS_DIR)

# ---------------------------------------------------
# CIA Build (Home Screen)
# ---------------------------------------------------

# 1. Create the RomFS binary
romfs.bin: $(ROMFS_DIR)
	3dstool -c -t romfs -f $@ --romfs-dir $(ROMFS_DIR)

# 2. Create the Home Menu Banner (requires a 256x128 banner.png and an audio.wav)
banner.bin: banner.png audio.wav
	bannertool makebanner -i banner.png -a audio.wav -o $@

# 3. Compile the CIA using makerom
$(TARGET).cia: $(TARGET).elf $(TARGET).smdh banner.bin app.rsf romfs.bin
	makerom -f cia -o $@ -elf $< -rsf app.rsf -icon $(TARGET).smdh -banner banner.bin -romfs romfs.bin -exefslogo -target t

# ---------------------------------------------------
# Core Compilation
# ---------------------------------------------------

$(TARGET).elf: $(OBJS)
	$(CXX) -specs=3dsx.specs -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -o $@ $^ $(LIBS)

main.o: main.cpp vshader_shbin.h
	$(CXX) -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -D__3DS__ -O2 -fno-exceptions -fno-rtti -I$(DEVKITPRO)/libcitro3d/include -I$(DEVKITPRO)/libctru/include -c $< -o $@

vshader.shbin.o: vshader.v.pica
	picasso -o vshader.shbin $<
	bin2s vshader.shbin > vshader.shbin.s
	$(AS) -march=armv6k -mfloat-abi=hard -o $@ vshader.shbin.s

vshader_shbin.h: vshader.shbin
	echo "extern const u8 vshader_shbin[];" > $@
	echo "extern const u32 vshader_shbin_size;" >> $@

clean:
	rm -f $(TARGET).3dsx $(TARGET).cia $(TARGET).smdh $(TARGET).elf $(OBJS) vshader.shbin vshader.shbin.s vshader_shbin.h banner.bin romfs.bin
