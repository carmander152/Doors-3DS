ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment.")
endif

include $(DEVKITPRO)/devkitARM/3ds_rules

TARGET := hotel_doors
OBJS := vshader.shbin.o main.o
LIBS := -L$(DEVKITPRO)/libcitro3d/lib -L$(DEVKITPRO)/libctru/lib -lcitro3d -lctru -lm
ROMFS_DIR := romfs

.PHONY: all clean

all: $(TARGET).elf $(TARGET).3dsx $(TARGET).cia

$(TARGET).smdh: icon.png
	smdhtool --create "Doors 3DS" "Remake of LSplash's Roblox game Doors" "carmander152" icon.png $@

$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool $< $@ --smdh=$(TARGET).smdh --romfs=$(ROMFS_DIR)

romfs.bin: $(ROMFS_DIR)
	3dstool -c -t romfs -f $@ --romfs-dir $(ROMFS_DIR)

banner.bin: banner.png audio.wav
	bannertool makebanner -i banner.png -a audio.wav -o $@

app.rsf:
	@echo "BasicInfo:" > app.rsf
	@echo "  Title                   : \"Doors 3DS\"" >> app.rsf
	@echo "  CompanyCode             : \"00\"" >> app.rsf
	@echo "  ProductCode             : \"CTR-P-DOOR\"" >> app.rsf
	@echo "  ContentType             : Application" >> app.rsf
	@echo "  Logo                    : Nintendo" >> app.rsf
	@echo "TitleInfo:" >> app.rsf
	@echo "  UniqueId                : 0xF800" >> app.rsf
	@echo "  Category                : Application" >> app.rsf
	@echo "Option:" >> app.rsf
	@echo "  UseOnSD                 : true" >> app.rsf
	@echo "AccessControlInfo:" >> app.rsf
	@echo "  CoreVersion             : 2" >> app.rsf
	@echo "  Priority                : 48" >> app.rsf
	@echo "  HandleTableSize         : 512" >> app.rsf
	@echo "  ServiceAccessControl:" >> app.rsf
	@echo "    - apt:U" >> app.rsf
	@echo "    - gsp::Gpu" >> app.rsf
	@echo "    - hid:USER" >> app.rsf
	@echo "    - dsp::DSP" >> app.rsf
	@echo "    - fs:USER" >> app.rsf
	@echo "  SystemCallAccess:" >> app.rsf
	@echo "    - ControlMemory" >> app.rsf
	@echo "    - ControlProcessMemory" >> app.rsf
	@echo "    - ExitProcess" >> app.rsf
	@echo "    - GetProcessInfo" >> app.rsf
	@echo "    - GetProcessList" >> app.rsf
	@echo "    - GetThreadList" >> app.rsf
	@echo "    - WaitSynchronization1" >> app.rsf
	@echo "    - WaitSynchronizationN" >> app.rsf
	@echo "    - CreateEvent" >> app.rsf
	@echo "    - SignalEvent" >> app.rsf
	@echo "    - ClearEvent" >> app.rsf
	@echo "    - CreateTimer" >> app.rsf
	@echo "    - SetTimer" >> app.rsf
	@echo "    - CancelTimer" >> app.rsf
	@echo "    - ClearTimer" >> app.rsf
	@echo "    - CreateMemoryBlock" >> app.rsf
	@echo "    - MapMemoryBlock" >> app.rsf
	@echo "    - UnmapMemoryBlock" >> app.rsf
	@echo "    - CreateAddressArbiter" >> app.rsf
	@echo "    - ArbitrateAddress" >> app.rsf
	@echo "    - CloseHandle" >> app.rsf
	@echo "    - ConnectToPort" >> app.rsf
	@echo "    - SendSyncRequest" >> app.rsf
	@echo "    - GetSystemTick" >> app.rsf
	@echo "    - GetSystemInfo" >> app.rsf
	@echo "    - GetProcessId" >> app.rsf
	@echo "    - GetThreadId" >> app.rsf
	@echo "    - OutputDebugString" >> app.rsf
	@echo "SystemControlInfo:" >> app.rsf
	@echo "  SaveDataSize            : 128KB" >> app.rsf
	@echo "  StackSize               : 0x40000" >> app.rsf

$(TARGET).cia: $(TARGET).elf $(TARGET).smdh banner.bin app.rsf romfs.bin
	makerom -f cia -o $@ -elf $< -rsf app.rsf -icon $(TARGET).smdh -banner banner.bin -romfs romfs.bin -exefslogo -target t

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
	rm -f $(TARGET).3dsx $(TARGET).cia $(TARGET).smdh $(TARGET).elf $(OBJS) vshader.shbin vshader.shbin.s vshader_shbin.h banner.bin romfs.bin app.rsf
