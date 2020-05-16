TARGET := riscv64-unknown-elf
CC := $(TARGET)-gcc
CXX := $(TARGET)-g++
OBJCOPY := $(TARGET)-objcopy

CFLAGS_CKB_STD = -Ideps/ckb-c-stdlib
CFLAGS_INTX := -Ideps/intx/lib/intx -Ideps/intx/include
CFLAGS_ETHASH := -Ideps/ethash/include -Ideps/ethash/lib/ethash -Ideps/ethash/lib/keccak -Ideps/ethash/lib/support
CFLAGS_EVMONE := -Ideps/evmone/lib/evmone -Ideps/evmone/include -Ideps/evmone/evmc/include
CFLAGS := -O3 $(CFLAGS_CKB_STD) $(CFLAGS_EVMONE) $(CFLAGS_INTX) $(CFLAGS_ETHASH) -Wall
CXXFLAGS := $(CFLAGS) -std=c++1z
LDFLAGS := -fdata-sections -ffunction-sections -Wl,--gc-sections

ALL_OBJS := build/evmone.o build/analysis.o build/execution.o build/instructions.o build/div.o build/keccak.o build/keccakf800.o build/keccakf1600.o

BUILDER_DOCKER := nervos/ckb-riscv-gnu-toolchain@sha256:7b168b4b109a0f741078a71b7c4dddaf1d283a5244608f7851f5714fbad273ba

all: build/generator build/generator_test

all-via-docker:
	mkdir -p build
	docker run --rm -v `pwd`:/code ${BUILDER_DOCKER} bash -c "cd /code && make"

build/generator: vm.c $(ALL_OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ vm.c $(ALL_OBJS) -DBUILD_GENERATOR
	$(OBJCOPY) --strip-debug --strip-all $@

build/generator_test: vm.c $(ALL_OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ vm.c $(ALL_OBJS) -DBUILD_GENERATOR -DTEST_BIN
	riscv64-unknown-elf-run $@ 4a010000 00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000101000060806040525b607b60006000508190909055505b610018565b60db806100266000396000f3fe60806040526004361060295760003560e01c806360fe47b114602f5780636d4ce63c14605b576029565b60006000fd5b60596004803603602081101560445760006000fd5b81019080803590602001909291905050506084565b005b34801560675760006000fd5b50606e6094565b6040518082815260200191505060405180910390f35b8060006000508190909055505b50565b6000600060005054905060a2565b9056fea26469706673582212204e58804e375d4a732a7b67cce8d8ffa904fa534d4555e655a433ce0a5e0d339f64736f6c6343000606003300000000

build/evmone.o: deps/evmone/lib/evmone/evmone.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $< -DPROJECT_VERSION=\"0.5.0-dev\"
build/analysis.o: deps/evmone/lib/evmone/analysis.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $<
build/execution.o: deps/evmone/lib/evmone/execution.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $<
build/instructions.o: deps/evmone/lib/evmone/instructions.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $<

build/keccak.o: deps/ethash/lib/keccak/keccak.c build/keccakf800.o build/keccakf1600.o
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $<
build/keccakf1600.o: deps/ethash/lib/keccak/keccakf1600.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $<
build/keccakf800.o: deps/ethash/lib/keccak/keccakf800.c
	$(CC) $(CFLAGS) $(LDFLAGS)  -c -o $@ $<

build/div.o: deps/intx/lib/intx/div.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -c -o $@ $<

clean:
	rm -rf build/*

clean-main:
	rm -rf build/evmone
