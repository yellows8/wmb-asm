# The assembled html is written to htmlhaxx_x.html by default, where x is dsstation or nzone for the DS Station and Nintendo Zone builds.

SOURCE	:=	nzonehtmlhaxx.s
ifeq ($(ASM_EXPLOIT),1)
SOURCE	:=	old_exploit.s
endif

TARGET_DSSTATION	:=	htmlhaxx_dsstation.html
TARGET_NZONE	:=	htmlhaxx_nzone.html
WEBDIR_DSSTATION	:=	../../../../website/ds/data
WEBDIR_NZONE	:=	../../../../website/ds/data

$(TARGET_NZONE) $(TARGET_DSSTATION):	nzonehtmlhaxx_nzone.elf nzonehtmlhaxx_dsstation.elf
	arm-eabi-objcopy -O binary nzonehtmlhaxx_dsstation.elf $(TARGET_DSSTATION)
	arm-eabi-objcopy -O binary nzonehtmlhaxx_nzone.elf $(TARGET_NZONE)
	cp $(TARGET_DSSTATION) $(WEBDIR_DSSTATION)
	cp $(TARGET_NZONE) $(WEBDIR_NZONE)

nzonehtmlhaxx_dsstation.elf nzonehtmlhaxx_nzone.elf:	$(SOURCE)
	arm-eabi-gcc -x assembler-with-cpp -DDS_STATION -nostartfiles -nostdlib $< -o nzonehtmlhaxx_dsstation.elf
	arm-eabi-gcc -x assembler-with-cpp -DNZONE -nostartfiles -nostdlib $< -o nzonehtmlhaxx_nzone.elf

clean:
	rm -f $(TARGET_DSSTATION) $(TARGET_NZONE) nzonehtmlhaxx_dsstation.elf nzonehtmlhaxx_nzone.elf nzoneurlstacksmash.o

