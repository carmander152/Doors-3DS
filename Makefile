ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment.")
endif

include $(DEVKITPRO)/devkitARM/3ds_rules

TARGET := hotel_doors
ROMFS  := romfs
OBJS   := vshader.shbin.o main.o
LIBS   := -L$(DEVKITPRO)/libcitro3d/lib -L$(DEVKITPRO)/libctru/lib -lcitro3d -lctru -lm

# Metadata for the Homebrew Launcher
APP_TITLE       := Hotel Doors
APP_DESCRIPTION := 3DS Doors Fan Game
APP_AUTHOR      := AI & Peer

.PHONY: all clean

all: $(TARGET).3dsx

# Rule to create the missing SMDH file
$(TARGET).smdh:
	smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $@

# Updated to depend on the .smdh and include it in the build
$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool $< $@ --romfs=$(ROMFS) --smdh=$(TARGET).smdh

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
	echo "" >> $@ # Added this to fix the assembly newline warning

clean:
	rm -f $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh $(OBJS) vshader.shbin vshader.shbin.s vshader_shbin.h
