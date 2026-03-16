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

# 3. Generate a complete Rom Spec File (RSF) compatible with makerom v0.18
app.rsf:
	@echo "BasicInfo:" > app.rsf
	@echo "  Title                   : \"Doors 3DS\"" >> app.rsf
	@echo "  CompanyCode             : \"00\"" >> app.rsf
	@echo "  ProductCode             : \"CTR-P-DOOR\"" >> app.rsf
	@echo "  ContentType             : Application" >> app.rsf
	@echo "  Logo                    : Nintendo" >> app.rsf
	@echo "TitleInfo:" >> app.rsf
	@echo "  UniqueId                : 0x0F800" >> app.rsf
	@echo "  Category                : Application" >> app.rsf
	@echo "Option:" >> app.rsf
	@echo "  UseOnSD                 : true" >> app.rsf
	@echo "AccessControlInfo:" >> app.rsf
	@echo "  Priority                : 0x30" >> app.rsf
	@echo "  IdealProcessor          : 0" >> app.rsf
	@echo "  AffinityMask            : 1" >> app.rsf
	@echo "  SystemMode              : 64MB" >> app.rsf
	@echo "  MemoryType              : Application" >> app.rsf
	@echo "  CoreVersion             : 2" >> app.rsf
	@echo "  HandleTableSize         : 512" >> app.rsf
	@echo "  ServiceAccessControl:" >> app.rsf
	@echo "    - gsp::Gpu" >> app.rsf
	@echo "    - hid:USER" >> app.rsf
	@echo "    - apt:U" >> app.rsf
	@echo "    - y2r:U" >> app.rsf
	@echo "    - ir:u" >> app.rsf
	@echo "    - ptm:u" >> app.rsf
	@echo "    - ac:u" >> app.rsf
	@echo "    - dsp::DSP" >> app.rsf
	@echo "    - cfg:u" >> app.rsf
	@echo "    - nfc:u" >> app.rsf
	@echo "    - ndm:u" >> app.rsf
	@echo "    - boss:P" >> app.rsf
	@echo "    - soc:U" >> app.rsf
	@echo "    - frd:u" >> app.rsf
	@echo "    - nwm::U" >> app.rsf
	@echo "    - ldr:ro" >> app.rsf
	@echo "  SystemCallAccess:" >> app.rsf
	@echo "    - ControlMemory" >> app.rsf
	@echo "    - QueryMemory" >> app.rsf
	@echo "    - ExitProcess" >> app.rsf
	@echo "    - GetProcessAffinityMask" >> app.rsf
	@echo "    - SetProcessAffinityMask" >> app.rsf
	@echo "    - GetProcessIdealProcessor" >> app.rsf
	@echo "    - SetProcessIdealProcessor" >> app.rsf
	@echo "    - CreateTimer" >> app.rsf
	@echo "    - SetTimer" >> app.rsf
	@echo "    - CancelTimer" >> app.rsf
	@echo "    - ClearTimer" >> app.rsf
	@echo "    - CreateEvent" >> app.rsf
	@echo "    - SignalEvent" >> app.rsf
	@echo "    - ClearEvent" >> app.rsf
	@echo "    - CreateAddressArbiter" >> app.rsf
	@echo "    - ArbitrateAddress" >> app.rsf
	@echo "    - CloseHandle" >> app.rsf
	@echo "    - WaitSynchronization1" >> app.rsf
	@echo "    - WaitSynchronizationN" >> app.rsf
	@echo "    - SignalAndWait" >> app.rsf
	@echo "    - GetSystemTick" >> app.rsf
	@echo "    - GetHandleInfo" >> app.rsf
	@echo "    - GetSystemInfo" >> app.rsf
	@echo "    - GetProcessInfo" >> app.rsf
	@echo "    - GetThreadInfo" >> app.rsf
	@echo "    - ConnectToPort" >> app.rsf
	@echo "    - SendSyncRequest1" >> app.rsf
	@echo "    - SendSyncRequest2" >> app.rsf
	@echo "    - SendSyncRequest3" >> app.rsf
	@echo "    - SendSyncRequest4" >> app.rsf
	@echo "    - SendSyncRequest" >> app.rsf
	@echo "    - ReplyAndReceive1" >> app.rsf
	@echo "    - ReplyAndReceive2" >> app.rsf
	@echo "    - ReplyAndReceive3" >> app.rsf
	@echo "    - ReplyAndReceive4" >> app.rsf
	@echo "    - ReplyAndReceive" >> app.rsf
	@echo "    - BindInterrupt" >> app.rsf
	@echo "    - UnbindInterrupt" >> app.rsf
	@echo "    - InvalidateProcessDataCache" >> app.rsf
	@echo "    - StoreProcessDataCache" >> app.rsf
	@echo "    - FlushProcessDataCache" >> app.rsf
	@echo "    - StartInterProcessDma" >> app.rsf
	@echo "    - StopDma" >> app.rsf
	@echo "    - GetDmaState" >> app.rsf
	@echo "    - RestartDma" >> app.rsf
	@echo "    - SetThreadPriority" >> app.rsf
	@echo "    - GetThreadPriority" >> app.rsf
	@echo "    - CreateThread" >> app.rsf
	@echo "    - ExitThread" >> app.rsf
	@echo "    - SleepThread" >> app.rsf
	@echo "    - CreateBlockProcess" >> app.rsf
	@echo "    - CreateProcess" >> app.rsf
	@echo "    - TerminateProcess" >> app.rsf
	@echo "    - GetProcessId" >> app.rsf
	@echo "    - GetThreadId" >> app.rsf
	@echo "    - GetResourceLimit" >> app.rsf
	@echo "    - GetResourceLimitLimitValues" >> app.rsf
	@echo "    - GetResourceLimitCurrentValues" >> app.rsf
	@echo "    - GetThreadContext" >> app.rsf
	@echo "    - Break" >> app.rsf
	@echo "    - OutputDebugString" >> app.rsf
	@echo "    - ControlPerformanceCounter" >> app.rsf
	@echo "SystemControlInfo:" >> app.rsf
	@echo "  SaveDataSize            : 128KB" >> app.rsf
	@echo "  RemasterVersion         : 0" >> app.rsf
	@echo "  StackSize               : 0x40000" >> app.rsf

# 4. Compile the CIA using makerom
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
	rm -f $(TARGET).3dsx $(TARGET).cia $(TARGET).smdh $(TARGET).elf $(OBJS) vshader.shbin vshader.shbin.s vshader_shbin.h banner.bin romfs.bin app.rsf
