PRESET=Debug
PROTO=protocol/bldc.proto
PROTO_GENERATED=protocol/bldc.pb.c protocol/bldc.pb.h protocol/bldc_pb2.py
NANOPB_GENERATOR=Third_Party/nanopb/generator/nanopb_generator.py

.PHONY: build flash clean proto

build:
	@echo "Building project..."
	@cmake --preset $(PRESET)
	@cmake --build --preset $(PRESET)

flash: build
	@echo "Flashing project..."
	@cmake --build --preset $(PRESET) --target flash

proto:
	@echo "Regenerating Protocol Buffers bindings..."
	@rm -f $(PROTO_GENERATED)
	@python3 $(NANOPB_GENERATOR) $(PROTO)
	@protoc --python_out=. $(PROTO)

clean:
	@echo "Cleaning project..."
	@cmake --build --preset $(PRESET) --target clean
