# Name of your output file
TARGET      := Doors3DS
SOURCES     := .
INCLUDES    := .

# LIBRARIES AND PATHS
# This tells the compiler where to find 3ds.h
LIBCTRU		:= $(DEVKITPRO)/libctru
CINCLUDES	:= $(INCLUDES) $(LIBCTRU)/include
LIBS    	:= -lcitro3d -lctru -lm

# Standard devkitPro Rules
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

# Use the correct include paths during compilation
CFLAGS	+= -I$(LIBCTRU)/include
CXXFLAGS += -I$(LIBCTRU)/include

# Build rules
all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf
$(TARGET).elf: main.o
