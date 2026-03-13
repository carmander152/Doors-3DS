# 1. Project Setup
TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .

# 2. Grab the Official Rules (This fixes the undefined references)
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

# 3. Setup Libraries
LIBS    := -lcitro3d -lctru -lm

# 4. Search Paths
LIBDIRS := $(DEVKITPRO)/libcitro3d $(DEVKITPRO)/libctru
export INCLUDE  := $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

# 5. Build Rules
all: $(TARGET).3dsx

# The magic is in $(LINK.cpp) - it knows where the system symbols are!
$(TARGET).elf: main.o
	$(LINK.cpp) $(LIBPATHS) $^ $(LIBS) -o $@

main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *.o *.elf *.3dsx *.map
