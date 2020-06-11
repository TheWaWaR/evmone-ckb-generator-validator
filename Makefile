TARGET := riscv64-unknown-elf
CC := $(TARGET)-gcc
CXX := $(TARGET)-g++
OBJCOPY := $(TARGET)-objcopy

CFLAGS_CKB_STD = -Ideps/ckb-c-stdlib
CFLAGS_INTX := -Ideps/intx/lib/intx -Ideps/intx/include
CFLAGS_ETHASH := -Ideps/ethash/include -Ideps/ethash/lib/ethash -Ideps/ethash/lib/keccak -Ideps/ethash/lib/support
CFLAGS_EVMONE := -Ideps/evmone/lib/evmone -Ideps/evmone/include -Ideps/evmone/evmc/include
CFLAGS_SECP := -isystem deps/secp256k1/src -isystem deps/secp256k1
CFLAGS := -O3 $(CFLAGS_CKB_STD) $(CFLAGS_EVMONE) $(CFLAGS_INTX) $(CFLAGS_ETHASH) $(CFLAGS_SECP) -Wall
CXXFLAGS := $(CFLAGS) -std=c++1z
LDFLAGS := -fdata-sections -ffunction-sections -Wl,--gc-sections
SECP256K1_SRC := deps/secp256k1/src/ecmult_static_pre_context.h

ALL_OBJS := build/evmone.o build/analysis.o build/execution.o build/instructions.o build/div.o build/keccak.o build/keccakf800.o build/keccakf1600.o

BUILDER_DOCKER := nervos/ckb-riscv-gnu-toolchain@sha256:7b168b4b109a0f741078a71b7c4dddaf1d283a5244608f7851f5714fbad273ba

all: build/generator build/generator_test build/validator

all-via-docker:
	mkdir -p build
	docker run --rm -v `pwd`:/code ${BUILDER_DOCKER} bash -c "cd /code && make"

build/validator: vm.c vm_validator.h build/secp256k1_data_info.h $(SECP256K1_SRC) $(ALL_OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) -Ibuild -o $@ vm.c $(ALL_OBJS)
	$(OBJCOPY) --strip-debug --strip-all $@

build/generator: vm.c vm_generator.h $(ALL_OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ vm.c $(ALL_OBJS) -DBUILD_GENERATOR
	$(OBJCOPY) --strip-debug --strip-all $@

build/generator_test: vm.c vm_test.h $(ALL_OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ vm.c $(ALL_OBJS) -DBUILD_GENERATOR -DTEST_BIN

	# riscv64-unknown-elf-run $@ 83010000 00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000003a010000030000000000000000111111111111111111111111111111111111111122222222222222222222222222222222222222220101000060806040525b607b60006000508190909055505b610018565b60db806100266000396000f3fe60806040526004361060295760003560e01c806360fe47b114602f5780636d4ce63c14605b576029565b60006000fd5b60596004803603602081101560445760006000fd5b81019080803590602001909291905050506084565b005b34801560675760006000fd5b50606e6094565b6040518082815260200191505060405180910390f35b8060006000508190909055505b50565b6000600060005054905060a2565b9056fea26469706673582212204e58804e375d4a732a7b67cce8d8ffa904fa534d4555e655a433ce0a5e0d339f64736f6c634300060600330000000000000000

	# riscv64-unknown-elf-run $@ 47010000 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000ea000000000000000000000000c8328aabcd9b9e8e64fbc566c4385c3bdeb219d751742da3eaab29d070de724c185d78a00926912ead000000608060405234801560105760006000fd5b5060043610602c5760003560e01c8063ae8421e114603257602c565b60006000fd5b6038603a565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16ff5b56fea2646970667358221220ead2c0723dcc5bc6fe1848ffcc748528c4f0638575fdee75e2c972c60fa1ea2d64736f6c6343000606003304000000ae8421e1000000000000000000000000000000000000000000000000

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

# secp256k1
build/secp256k1_data_info.h: build/dump_secp256k1_data
	$<

build/dump_secp256k1_data: dump_secp256k1_data.c $(SECP256K1_SRC)
	mkdir -p build
	gcc $(CFLAGS_SECP) $(CFLAGS_CKB_STD) -o $@ $<

$(SECP256K1_SRC):
	cd deps/secp256k1 && \
		./autogen.sh && \
		CC=$(CC) LD=$(LD) ./configure --with-bignum=no --enable-ecmult-static-precomputation --enable-endomorphism --enable-module-recovery --host=$(TARGET) && \
		make src/ecmult_static_pre_context.h src/ecmult_static_context.h

clean:
	rm -rf build/*
	cd deps/secp256k1 && [ -f "Makefile" ] && make clean

clean-bin:
	rm -rf build/generator_test build/generator build/validator
