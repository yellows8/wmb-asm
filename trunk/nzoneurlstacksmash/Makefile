# The assembled html is written to dsds_menu_up_x.html by default, where x is dsstation or nzone for the DS Station and Nintendo Zone builds.

TARGET_DSSTATION	:=	dsds_menu_up_dsstation.html
TARGET_NZONE	:=	dsds_menu_up_nzone.html
WEBDIR_DSSTATION	:=	../website/ds/data
WEBDIR_NZONE	:=	../website/ds/data

$(TARGET_NZONE) $(TARGET_DSSTATION):	nzoneurlstacksmash_nzone.elf nzoneurlstacksmash_dsstation.elf
	arm-eabi-objcopy -O binary nzoneurlstacksmash_dsstation.elf $(TARGET_DSSTATION)
	arm-eabi-objcopy -O binary nzoneurlstacksmash_nzone.elf $(TARGET_NZONE)

nzoneurlstacksmash_dsstation.elf nzoneurlstacksmash_nzone.elf:	nzoneurlstacksmash.s
	arm-eabi-gcc -x assembler-with-cpp -DDS_STATION -nostartfiles -nostdlib $< -o nzoneurlstacksmash_dsstation.elf
	arm-eabi-gcc -x assembler-with-cpp -DNZONE -nostartfiles -nostdlib $< -o nzoneurlstacksmash_nzone.elf

clean:
	rm -f $(TARGET_DSSTATION) $(TARGET_NZONE) nzoneurlstacksmash_dsstation.elf nzoneurlstacksmash_nzone.elf nzoneurlstacksmash.o

