PRESET=Debug

.PHONY: build flash

build:
	@echo "Building project..."
	@cmake --preset $(PRESET)
	@cmake --build --preset $(PRESET)

flash: build
	@echo "Flashing project..."
	@cmake --build --preset $(PRESET) --target flash

clean:
	@echo "Cleaning project..."
	@cmake --build --preset $(PRESET) --target clean