PRESET=Debug

build:
	@echo "Building project..."
  @cmake --build --preset Debug
flash: 
	@echo "Flashing project..."
	@cmake --build --preset Debug --target flash
