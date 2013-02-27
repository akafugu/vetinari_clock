# Makefile
# (C) 2011-13 Akafugu Corporation
#
# This program is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU General Public License for more details.

# Define your programmer in this file: ~/user.mk
-include ~/user.mk

SILENT ?= @
CROSS ?= avr-

include Makefile.config

# These are set in Makefile.conf
# MCU
# F_CPU
# TARGET
# SRCS
# VALUE_DEFS 
# YESNO_DEFS

OBJS = $(SRCS:.c=.o)

ifneq ($(CROSS), )
  CC = $(CROSS)gcc
  CXX = $(CROSS)g++
  OBJCOPY = $(CROSS)objcopy
  OBJDUMP = $(CROSS)objdump
  SIZE = $(CROSS)size
endif

ifneq ($(F_CPU),)
  CFLAGS += -DF_CPU=$(F_CPU)
endif

## Special defines

define CHECK_YESNO_ANSWER
  ifeq ($$($(1)), YES)
    CFLAGS += -D$(1)
  endif
endef

$(foreach i,$(YESNO_DEFS),$(eval $(call CHECK_YESNO_ANSWER,$(i))))

define CHECK_VALUE_ANSWER
  ifneq ($$($(1)), )
    CFLAGS += -D$(1)=$$($(1))
  endif
endef

$(foreach i,$(VALUE_DEFS),$(eval $(call CHECK_VALUE_ANSWER,$(i))))

##

OPT=s

CFLAGS += -g -O$(OPT) \
-ffreestanding -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums \
-Wall -Wstrict-prototypes \
-Wa,-adhlns=$(<:.c=.lst) -std=gnu99 -mmcu=$(MCU) 

LDFLAGS = -Wl,-Map=$(TARGET).map,--cref

all: $(TARGET).elf size

size: $(TARGET).elf
	$(SILENT) $(SIZE) -C --mcu=$(MCU) $(TARGET).elf 

ifneq ($(wildcard $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).eep $(TARGET).map $(OBJS:%.o=%.d) $(OBJS:%.o=%.lst)), )
clean:
	-rm $(wildcard $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).eep $(TARGET).map $(OBJS:%.o=%.d) $(OBJS:%.o=%.lst))
else
clean:
	@echo "Nothing to clean."
endif

.SECONDARY:

%.elf: $(OBJS)
	@echo "Linking:" $@...
	$(SILENT) $(CC) $(CFLAGS) $(OBJS) --output $@ $(LDFLAGS)

%.o : %.cpp
	@echo "[$(TARGET)] Compiling:" $@... 
	$(SILENT) $(CXX) $(CXXFLAGS) -MMD -MF $(@:%.o=%.d) -c $< -o $@

%.o : %.c
	@echo "[$(TARGET)] Compiling:" $@...
	$(SILENT) $(CC) $(CFLAGS) -MMD -MF $(@:%.o=%.d) -c $< -o $@

%.d : %.cpp
	@echo "[$(TARGET)] Generating dependency:" $@...
	$(SILENT) $(CXX) $(CXXFLAGS) -MM -MT $(addsuffix .o, $(basename $@)) -MF $@ $<

%.d : %.c
	@echo "[$(TARGET)] Generating dependency:" $@...
	$(SILENT) $(CC) $(CFLAGS) -MM -MT $(addsuffix .o, $(basename $@)) -MF $@ $<

###############

## Programming

AVRDUDE := avrdude

AVRDUDE_FLAGS += -p $(MCU)
ifneq ($(AVRDUDE_PORT), )
  AVRDUDE_FLAGS += -P $(AVRDUDE_PORT)
endif
ifneq ($(AVRDUDE_PROGRAMMER), )
  AVRDUDE_FLAGS += -c $(AVRDUDE_PROGRAMMER)
endif
ifneq ($(AVRDUDE_SPEED), )
  AVRDUDE_FLAGS += -b $(AVRDUDE_SPEED)
endif

#Add more verbose output if we dont have SILENT set
ifeq ($(SILENT), )
  AVRDUDE_FLAGS += -v -v
endif

AVRDUDE_FLAGS += -B 250

# Fuses for 32KHz oscillator
AVRDUDE_WRITE_FUSE ?= -U lfuse:w:0xd6:m -U hfuse:w:0xde:m

ifneq ($(AVRDUDE_PROGRAMMER), )
flash: $(TARGET).hex $(TARGET).eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$(TARGET).hex -U eeprom:w:$(TARGET).eep

fuse:
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FUSE) 

install: fuse flash

%.hex: %.elf
	@echo "Creating flash file:" $@...
	$(SILENT) $(OBJCOPY) -O ihex -R .eeprom $< $@

%.eep: %.elf
	@echo "Creating eeprom file:" $@...
	$(SILENT) $(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O ihex $< $@
else
FLASH_MSG="You need to set AVRDUDE_PROGRAMMER/AVRDUDE_PORT/AVRDUDE_SPEED in ~/user.mk"
flash:
	@echo $(FLASH_MSG)

fuse:
	@echo $(FLASH_MSG)
endif

###############

# Check which .o files we already have and include their dependency files.
PRIOR_OBJS := $(wildcard $(OBJS))
include $(PRIOR_OBJS:%.o=%.d)
