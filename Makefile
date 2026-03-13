# Name of your output file
TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .

# Libraries for 3D and System
LIBS    := -lcitro3d -lctru -lm

# Standard devkitPro Rules
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

# Build rules
all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf
$(TARGET).elf: main.o
