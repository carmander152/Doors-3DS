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

# 3. Generate a complete Rom Spec File (RSF) - This is the "Bouncer" for makerom
app.rsf:
	@cat << 'EOF' > app.rsf
BasicInfo:
  Title                   : "Doors 3DS"
  CompanyCode             : "00"
  ProductCode             : "CTR-P-DOOR"
  ContentType             : Application
  Logo                    : Nintendo

TitleInfo:
  UniqueId                : 0x0F800
  Category                : Application

Option:
  UseOnSD                 : true
  FreeProductCode         : true

AccessControlInfo:
  CoreVersion             : 2
  HandleTableSize         : 512
  Priority                : 0x30
  IdealProcessor          : 0
  AffinityMask            : 1
  SystemMode              : 64MB
  MemoryType              : Application
  FileSystemAccess        : DirectSdmc
  ServiceAccessControl:
    - gsp::Gpu
    - hid:USER
    - apt:U
    - dsp::DSP
    - fs:USER
    - cfg:u
    - ac:u
  SystemCallAccess:
    - ControlMemory
    - QueryMemory
    - ExitProcess
    - CreateThread
    - ExitThread
    - SleepThread
    - GetThreadPriority
    - SetThreadPriority
    - CreateMutex
    - ReleaseMutex
    - CreateSemaphore
    - ReleaseSemaphore
    - CreateEvent
    - SignalEvent
    - ClearEvent
    - CreateTimer
    - SetTimer
    - CancelTimer
    - ClearTimer
    - CreateAddressArbiter
    - ArbitrateAddress
    - CloseHandle
    - WaitSynchronization1
    - WaitSynchronizationN
    - GetSystemTick
    - ConnectToPort
    - SendSyncRequest
    - GetProcessId
    - GetThreadId
    - OutputDebugString

SystemControlInfo:
  SaveDataSize            : 128KB
  RemasterVersion         : 0
  StackSize               : 0x40000
EOF

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
