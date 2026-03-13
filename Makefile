TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .
LIBS        := -lcitro3d -lctru -lm

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf
$(TARGET).elf: main.o
