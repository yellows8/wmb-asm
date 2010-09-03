# The assembled html is written to htmlhaxx_x.html by default, where x is dsstation or nzone for the DS Station and Nintendo Zone builds.

SOURCE	:=	nzonehtmlhaxx.s
ifeq ($(ASM_EXPLOIT),1)
SOURCE	:=	old_exploit.s
endif

TARGET_DSSTATION	:=	htmlhaxx_dsstation.html
TARGET_NZONE	:=	htmlhaxx_nzone.html
WEBDIR_DSSTATION	:=	$(HOME)/website/ds/data
WEBDIR_NZONE	:=	$(HOME)/website/ds/data

$(TARGET_NZONE) $(TARGET_DSSTATION):	nzonehtmlhaxx_nzone.elf nzonehtmlhaxx_dsstation.elf
	arm-eabi-objcopy -O binary nzonehtmlhaxx_dsstation.elf $(TARGET_DSSTATION)
	arm-eabi-objcopy -O binary nzonehtmlhaxx_nzone.elf $(TARGET_NZONE)
	cp $(TARGET_DSSTATION) $(WEBDIR_DSSTATION)
	cp $(TARGET_NZONE) $(WEBDIR_NZONE)

nzonehtmlhaxx_dsstation.elf nzonehtmlhaxx_nzone.elf:	$(SOURCE)
	@make clean -C exploit DS_STATION=1
	@make -C exploit DS_STATION=1
	arm-eabi-gcc -x assembler-with-cpp -DDS_STATION -nostartfiles -nostdlib $< -o nzonehtmlhaxx_dsstation.elf
	@make clean -C exploit NZONE=1
	@make -C exploit NZONE=1
	@ls
	arm-eabi-gcc -x assembler-with-cpp -DNZONE -nostartfiles -nostdlib $< -o nzonehtmlhaxx_nzone.elf

clean:
	rm -f $(TARGET_DSSTATION) $(TARGET_NZONE) nzonehtmlhaxx_dsstation.elf nzonehtmlhaxx_nzone.elf nzoneurlstacksmash.o

