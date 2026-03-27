ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment.")
endif

include $(DEVKITPRO)/devkitARM/3ds_rules

TARGET   := Plex-3DS
OBJS     := source/main.o

LIBPATHS := -L$(DEVKITPRO)/libctru/lib -L$(DEVKITPRO)/portlibs/3ds/lib
INCLUDES := -I$(DEVKITPRO)/libctru/include -I$(DEVKITPRO)/portlibs/3ds/include -Isource/

# Linker order is strict: High-level libs -> Low-level libs -> System libs
LIBS     := -lcitro2d -lcitro3d -ljpeg -lcurl -lmbedtls -lmbedx509 -lmbedcrypto -lz -lctru -lm

.PHONY: all clean

all: $(TARGET).3dsx $(TARGET).cia

# 1. Build the Icon Metadata
$(TARGET).smdh: icon.png
	smdhtool --create "Plex 3DS" "Plex Client" "Gemini AI" icon.png $@

# 2. Build the 3DSX (For Homebrew Launcher)
$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool $< $@ --smdh=$(TARGET).smdh

# 3. Build the Banner (with generated silent audio track for bannertool compatibility)
banner.bnr: banner.png
	@echo "Generating silent audio for banner..."
	sox -n -r 44100 -c 2 audio.wav trim 0.0 1.0
	bannertool makebanner -i banner.png -a audio.wav -o banner.bnr

# 4. Generate the RSF configuration (Multi-line echo for makerom compatibility)
app.rsf:
	@echo "BasicInfo:" > app.rsf
	@echo "  Title: \"Plex 3DS\"" >> app.rsf
	@echo "  ProductCode: \"CTR-P-PLEX\"" >> app.rsf
	@echo "  UniqueId: 0xF1337" >> app.rsf
	@echo "  Category: Application" >> app.rsf
	@echo "Option:" >> app.rsf
	@echo "  UseOnSD: true" >> app.rsf
	@echo "SystemControlInfo:" >> app.rsf
	@echo "  MemoryType: Application" >> app.rsf
	@echo "  CPUCode: 0x00000000" >> app.rsf
	@echo "AccessControlInfo:" >> app.rsf
	@echo "  FileSystemAccess:" >> app.rsf
	@echo "    - DirectSdmc" >> app.rsf
	@echo "  ServiceAccessControl:" >> app.rsf
	@echo "    - apt:U" >> app.rsf
	@echo "    - gsp::Gpu" >> app.rsf
	@echo "    - hid:USER" >> app.rsf
	@echo "    - fs:USER" >> app.rsf
	@echo "    - soc:U" >> app.rsf
	@echo "    - http:C" >> app.rsf

# 5. Build the CIA (For FBI Installation)
$(TARGET).cia: $(TARGET).elf $(TARGET).smdh banner.bnr app.rsf
	makerom -f cia -o $@ -elf $< -rsf app.rsf -icon $(TARGET).smdh -banner banner.bnr -exefslogo -target t

# Linking stage
$(TARGET).elf: $(OBJS)
	$(CXX) -specs=3dsx.specs -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -o $@ $^ $(LIBPATHS) $(LIBS)

# Compilation stage
source/main.o: source/main.cpp
	$(CXX) -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -D__3DS__ -O2 -fno-exceptions -fno-rtti -std=c++11 $(INCLUDES) -c $< -o $@

clean:
	rm -f $(TARGET).3dsx $(TARGET).cia $(TARGET).smdh banner.bnr audio.wav $(TARGET).elf $(OBJS) app.rsf
