.PHONY: update_deps

include deps.mk

OLIMEX_JTAG = ftdi/olimex-arm-usb-tiny-h
JLINK_JTAG = jlink

JTAG ?= $(JLINK_JTAG)

all: bootloader app_mt

make_prog = $(MAKE) -f src/$(1)/$(1).mk

app_mt:
	$(call make_prog,app_mt) autogen
	$(call make_prog,app_mt)

bootloader:
	$(call make_prog,bootloader)

prog_download = @openocd \
	-f interface/$(JTAG).cfg \
	-f target/stm32f2x.cfg \
	-f stm32f2x-setup.cfg \
	-c "flash write_image erase build/$(1)/$(1).elf" \
	-c "reset init" \
	-c "reset run" \
	-c shutdown download.log 2>&1 && \
	echo Download complete
	
clear_app_cfg:
	@openocd \
	-f interface/$(JTAG).cfg \
	-f target/stm32f2x.cfg \
	-f stm32f2x-setup.cfg \
	-c "flash erase_sector 0 1 1" \
	-c "reset init" \
	-c "reset run" \
	-c shutdown download.log 2>&1 && \
	echo App config section has been erased

clear_app_hdr:
	@openocd \
	-f interface/$(JTAG).cfg \
	-f target/stm32f2x.cfg \
	-f stm32f2x-setup.cfg \
	-c "flash erase_sector 0 2 2" \
	-c "reset init" \
	-c "reset run" \
	-c shutdown download.log 2>&1 && \
	echo App config section has been erased

upgrade_image: app_mt
	arm-none-eabi-objcopy -O binary --only-section header build/app_mt/app_mt.elf build/app_mt/app_mt_hdr.bin
	arm-none-eabi-objcopy -O binary --remove-section cfg --remove-section header build/app_mt/app_mt.elf build/app_mt/app_mt_app.bin
	python scripts/build_app_image.py build/app_mt/app_mt_hdr.bin build/app_mt/app_mt_app.bin build/app_mt/app_mt_update.bin

download_app_mt: upgrade_image
	@openocd \
	-f interface/$(JTAG).cfg \
	-f target/stm32f2x.cfg \
	-f stm32f2x-setup.cfg \
	-c "flash erase_sector 0 2 last" \
	-c "flash write_bank 0 build/app_mt/app_mt_app.bin 0x8200" \
	-c "flash write_bank 0 build/app_mt/app_mt_hdr.bin 0x8000" \
	-c "reset init" \
	-c "reset run" \
	-c shutdown download.log 2>&1 && \
	echo Download complete

download_bootloader: bootloader
	$(call prog_download,bootloader)

download: download_app_mt download_bootloader 

build/app_mt/app_mt.dfu: upgrade_image
	python scripts/dfu.py \
		-b 0x08008000:build/app_mt/app_mt_hdr.bin \
		-b 0x08008200:build/app_mt/app_mt_app.bin \
		build/app_mt/app_mt.dfu

build/bootloader/bootloader.dfu: bootloader
	python scripts/dfu.py \
		-b 0x08000000:build/bootloader/bootloader.bin \
		build/bootloader/bootloader.dfu

download_dfu_app_mt: build/app_mt/app_mt.dfu
	dfu-util -a 0 -t 2048 -D build/app_mt/app_mt.dfu

download_dfu_bootloader: build/bootloader/bootloader.dfu
	dfu-util -a 0 -t 2048 -D build/bootloader/bootloader.dfu

build/all.dfu: upgrade_image bootloader
	python scripts/dfu.py \
		-b 0x08000000:build/bootloader/bootloader.bin \
		-b 0x08008000:build/app_mt/app_mt_hdr.bin \
		-b 0x08008200:build/app_mt/app_mt_app.bin \
		build/all.dfu

download_dfu: build/all.dfu
	dfu-util -a 0 -t 2048 -D build/all.dfu
	
autoload_dfu: build/all.dfu
	python scripts/autoload.py

clean:
	rm -rf .dep build

