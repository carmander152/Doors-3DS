# Name of your output file
TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .

# 3DS Libraries for 3D and Graphics
LIBS    := -lcitro3d -lctru -lm

# Standard devkitPro Rules
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

# Build instructions
all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf
$(TARGET).elf: main.o
