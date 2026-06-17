BOARD = stm32g431 
TARGET = stm32fgx # openocd target config file check /usr/share/openocd/scripts/target/ for target config
ELF_FILE = build/Debug/$(BOARD).elf


build: dsdl_gen_build
	cmake --preset Debug 
	cmake --build --preset Debug

bins:
		arm-none-eabi-objcopy -O binary $(ELF_FILE) build/firmware.bin
		arm-none-eabi-objcopy -O ihex  $(ELF_FILE) build/firmware.hex

flash: build bins
	if command -v openocd >/dev/null 2>&1; then \
		cd build && \
		openocd -f interface/stlink.cfg -f target/$(TARGET).cfg \
		  -c "program Debug/$(BOARD).elf verify reset exit"; \
	else \
		echo "No suitable flashing tool found. Please install openocd"; \
		exit 1; \
	fi


# =========================
# DSDL GEN DRONCAN
# =========================
TP_BASE=Middlewares/Third_Party
GEN_DIR=$(TP_BASE)/dsdl_generated

dsdl_gen:
	@test -d $(TP_BASE)/DSDL || git clone --depth 1 https://github.com/DroneCAN/DSDL $(TP_BASE)/DSDL

dsdlc:
	@test -d $(TP_BASE)/dronecan_dsdlc || git clone --depth 1 https://github.com/DroneCAN/dronecan_dsdlc $(TP_BASE)/dronecan_dsdlc

libcanard:
	@test -d $(TP_BASE)/libcanard || git clone https://github.com/dronecan/libcanard $(TP_BASE)/libcanard	

dsdl_gen_build: libcanard dsdl_gen dsdlc
	python3 $(TP_BASE)/dronecan_dsdlc/dronecan_dsdlc.py \
		-O $(GEN_DIR) \
		$(TP_BASE)/DSDL/uavcan \
		$(TP_BASE)/DSDL/dronecan

clean:
	rm -rf build

distclean: clean
	rm -rf $(TP_BASE)/libcanard $(TP_BASE)/DSDL $(TP_BASE)/dronecan_dsdlc $(GEN_DIR)

.PHONY: bin_create flash clean distclean dsdl_gen dsdlc dsdl_gen_build