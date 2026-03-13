TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .

# PATHS
LIBCTRU     := $(DEVKITPRO)/libctru
LIBS        := -lcitro3d -lctru -lm

# Standard devkitPro Rules
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

# Tell the Linker where to look for the -lcitro3d and -lctru files
LDFLAGS += -L$(LIBCTRU)/lib -L$(DEVKITPRO)/libcitro3d/lib

# Tell the Compiler where to look for headers
CFLAGS   += -I$(LIBCTRU)/include -I$(DEVKITPRO)/libcitro3d/include
CXXFLAGS += -I$(LIBCTRU)/include -I$(DEVKITPRO)/libcitro3d/include

all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf
$(TARGET).elf: main.o
