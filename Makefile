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
	smdhtool --create "Doors 3DS" "Remake of LSplashes Roblox game Doors for 3DS" "carmander152" icon.png $@

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

# 3. Generate a complete Rom Spec File (RSF)
app.rsf:
	@echo "BasicInfo:" > app.rsf
	@echo "  Title                   : \"Doors 3DS\"" >> app.rsf
	@echo "  CompanyCode             : \"00\"" >> app.rsf
	@echo "  ProductCode             : \"CTR-P-DOOR\"" >> app.rsf
	@echo "  ContentType             : Application" >> app.rsf
	@echo "  Logo                    : Nintendo" >> app.rsf
	@echo "TitleInfo:" >> app.rsf
	@echo "  UniqueId                : 0x0D00A" >> app.rsf
	@echo "  Category                : Application" >> app.rsf
	@echo "Option:" >> app.rsf
	@echo "  UseOnSD                 : true" >> app.rsf
	@echo "AccessControlInfo:" >> app.rsf
	@echo "  ExtSaveDataId: 0x0D00A" >> app.rsf
	@echo "  SystemSaveDataId1: 0x00000000" >> app.rsf
	@echo "  SystemSaveDataId2: 0x00000000" >> app.rsf
	@echo "  OtherUserSaveDataId1: 0x00000" >> app.rsf
	@echo "  OtherUserSaveDataId2: 0x00000" >> app.rsf
	@echo "  OtherUserSaveDataId3: 0x00000" >> app.rsf
	@echo "  UseOtherVariationSaveData: false" >> app.rsf
	@echo "  UseExtSaveData: false" >> app.rsf
	@echo "  UseExtSaveData1: false" >> app.rsf
	@echo "  UseExtSaveData2: false" >> app.rsf
	@echo "  UseExtSaveData3: false" >> app.rsf
	@echo "  AppType: Regular" >> app.rsf
	@echo "  IdealProcessor: 0" >> app.rsf
	@echo "  AffinityMask: 1" >> app.rsf
	@echo "  Priority: 16" >> app.rsf
	@echo "  MaxCpu: 0x9E" >> app.rsf
	@echo "  MemoryType: Application" >> app.rsf
	@echo "  CoreVersion: 2" >> app.rsf
	@echo "  SystemMode: 64MB" >> app.rsf
	@echo "  HandleTableSize: 512" >> app.rsf
	@echo "  DisableP3D: false" >> app.rsf
	@echo "  EnableL2Cache: true" >> app.rsf
	@echo "  ResourceLimitCategory: Application" >> app.rsf
	@echo "  ReleaseKernelMajor: 0" >> app.rsf
	@echo "  ReleaseKernelMinor: 0" >> app.rsf
	@echo "  FileSystemAccess:" >> app.rsf
	@echo "    - CategorySystemApplication" >> app.rsf
	@echo "    - CategoryHardwareCheck" >> app.rsf
	@echo "    - CategoryFileSystemTool" >> app.rsf
	@echo "    - Debug" >> app.rsf
	@echo "    - TwlCardBackup" >> app.rsf
	@echo "    - TwlNandData" >> app.rsf
	@echo "    - Boss" >> app.rsf
	@echo "    - DirectSdmc" >> app.rsf
	@echo "    - Core" >> app.rsf
	@echo "    - CtrNandRo" >> app.rsf
	@echo "    - CtrNandRw" >> app.rsf
	@echo "    - CtrNandRoWrite" >> app.rsf
	@echo "    - CategorySystemSettings" >> app.rsf
	@echo "    - CardBoard" >> app.rsf
	@echo "    - ExportImportIvs" >> app.rsf
	@echo "    - DirectSdmcWrite" >> app.rsf
	@echo "    - SwitchCleanup" >> app.rsf
	@echo "    - SaveDataMove" >> app.rsf
	@echo "    - Shop" >> app.rsf
	@echo "    - Shell" >> app.rsf
	@echo "    - CategoryHomeMenu" >> app.rsf
	@echo "    - SeedDB" >> app.rsf
	@echo "  IoAccess:" >> app.rsf
	@echo "    - Fsrv" >> app.rsf
	@echo "    - FsrvWrite" >> app.rsf
	@echo "    - I2c" >> app.rsf
	@echo "    - Spi" >> app.rsf
	@echo "    - Ndma" >> app.rsf
	@echo "  InterruptNumbers:" >> app.rsf
	@echo "  SystemCallAccess:" >> app.rsf
	@echo "    - ArbitrateAddress" >> app.rsf
	@echo "    - Break" >> app.rsf
	@echo "    - CancelTimer" >> app.rsf
	@echo "    - ClearEvent" >> app.rsf
	@echo "    - ClearTimer" >> app.rsf
	@echo "    - CloseHandle" >> app.rsf
	@echo "    - ConnectToPort" >> app.rsf
	@echo "    - ControlMemory" >> app.rsf
	@echo "    - CreateAddressArbiter" >> app.rsf
	@echo "    - CreateEvent" >> app.rsf
	@echo "    - CreateMemoryBlock" >> app.rsf
	@echo "    - CreateMutex" >> app.rsf
	@echo "    - CreateSemaphore" >> app.rsf
	@echo "    - CreateThread" >> app.rsf
	@echo "    - CreateTimer" >> app.rsf
	@echo "    - DuplicateHandle" >> app.rsf
	@echo "    - ExitProcess" >> app.rsf
	@echo "    - ExitThread" >> app.rsf
	@echo "    - GetProcessId" >> app.rsf
	@echo "    - GetProcessIdOfThread" >> app.rsf
	@echo "    - GetProcessIdealProcessor" >> app.rsf
	@echo "    - GetProcessInfo" >> app.rsf
	@echo "    - GetResourceLimit" >> app.rsf
	@echo "    - GetResourceLimitCurrentValues" >> app.rsf
	@echo "    - GetResourceLimitLimitValues" >> app.rsf
	@echo "    - GetSystemInfo" >> app.rsf
	@echo "    - GetThreadContext" >> app.rsf
	@echo "    - GetThreadId" >> app.rsf
	@echo "    - GetThreadIdealProcessor" >> app.rsf
	@echo "    - GetThreadInfo" >> app.rsf
	@echo "    - GetThreadPriority" >> app.rsf
	@echo "    - MapMemoryBlock" >> app.rsf
	@echo "    - OutputDebugString" >> app.rsf
	@echo "    - QueryMemory" >> app.rsf
	@echo "    - ReleaseMutex" >> app.rsf
	@echo "    - ReleaseSemaphore" >> app.rsf
	@echo "    - SendSyncRequest1" >> app.rsf
	@echo "    - SendSyncRequest2" >> app.rsf
	@echo "    - SendSyncRequest3" >> app.rsf
	@echo "    - SendSyncRequest4" >> app.rsf
	@echo "    - SendSyncRequest" >> app.rsf
	@echo "    - SetThreadPriority" >> app.rsf
	@echo "    - SetTimer" >> app.rsf
	@echo "    - SignalEvent" >> app.rsf
	@echo "    - SleepThread" >> app.rsf
	@echo "    - WaitSynchronization1" >> app.rsf
	@echo "    - WaitSynchronizationN" >> app.rsf
	@echo "  ServiceAccessControl:" >> app.rsf
	@echo "    - APT:A" >> app.rsf
	@echo "    - APT:U" >> app.rsf
	@echo "    - ac:u" >> app.rsf
	@echo "    - am:app" >> app.rsf
	@echo "    - am:net" >> app.rsf
	@echo "    - boss:U" >> app.rsf
	@echo "    - cam:u" >> app.rsf
	@echo "    - cecd:u" >> app.rsf
	@echo "    - cfg:u" >> app.rsf
	@echo "    - cfg:i" >> app.rsf
	@echo "    - csnd:SND" >> app.rsf
	@echo "    - dsp::DSP" >> app.rsf
	@echo "    - fs:USER" >> app.rsf
	@echo "    - gsp::Gpu" >> app.rsf
	@echo "    - gsp::Lcd" >> app.rsf
	@echo "    - hid:USER" >> app.rsf
	@echo "    - http:C" >> app.rsf
	@echo "    - ir:rst" >> app.rsf
	@echo "    - ir:u" >> app.rsf
	@echo "    - ir:USER" >> app.rsf
	@echo "    - mic:u" >> app.rsf
	@echo "    - ndm:u" >> app.rsf
	@echo "    - news:u" >> app.rsf
	@echo "    - nfc:u" >> app.rsf
	@echo "    - nim:aoc" >> app.rsf
	@echo "    - nim:u" >> app.rsf
	@echo "    - ptm:sysm" >> app.rsf
	@echo "    - ptm:u" >> app.rsf
	@echo "    - pxi:dev" >> app.rsf
	@echo "    - qtm:s" >> app.rsf
	@echo "    - soc:U" >> app.rsf
	@echo "    - ssl:C" >> app.rsf
	@echo "SystemControlInfo:" >> app.rsf
	@echo "  SaveDataSize: 0KB" >> app.rsf
	@echo "  RemasterVersion: 0" >> app.rsf
	@echo "  StackSize: 0x40000" >> app.rsf

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
