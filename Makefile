TARGET := hotel_doors
OBJS := vshader.shbin.o main.o
LIBS := -lcitro3d -lctru -lm

.PHONY: all clean

all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf
	3dsxtool $< $@

$(TARGET).elf: $(OBJS)
	arm-none-eabi-g++ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -o $@ $^ $(LIBS)

main.o: main.cpp vshader_shbin.h
	arm-none-eabi-g++ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -D__3DS__ -O2 -fno-exceptions -fno-rtti -I$(DEVKITPRO)/libcitro3d/include -I$(DEVKITPRO)/libctru/include -c $< -o $@

vshader.shbin.o: vshader.v.pica
	picasso -o vshader.shbin $<
	bin2s vshader.shbin > vshader.shbin.s
	arm-none-eabi-g++ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -c vshader.shbin.s -o $@

vshader_shbin.h: vshader.shbin
	echo "extern const u8 vshader_shbin[];" > $@
	echo "extern const u32 vshader_shbin_size;" >> $@

clean:
	rm -f $(TARGET).3dsx $(TARGET).elf $(OBJS) vshader.shbin vshader.shbin.s vshader_shbin.h
