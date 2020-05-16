BUILDER_DOCKER=nervos/ckb-riscv-gnu-toolchain@sha256:7b168b4b109a0f741078a71b7c4dddaf1d283a5244608f7851f5714fbad273ba
docker run --rm -v `pwd`:/code ${BUILDER_DOCKER} bash -c "riscv64-unknown-elf-run /code/build/generator_test $1 $2"
