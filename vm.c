
#include <evmone/evmone.h>
#include <evmc/evmc.h>

#ifdef BUILD_GENERATOR
#include "generator.h"
#else
#define CSAL_VALIDATOR_TYPE 1
#include "validator.h"
#endif

struct evmc_host_context {
  evmc_address address;
  evmc_bytes32 key;
  evmc_bytes32 value;
};

struct evmc_tx_context get_tx_context(struct evmc_host_context* context) {
  struct evmc_tx_context ctx;
  return ctx;
}

evmc_bytes32 get_storage(struct evmc_host_context* context,
                         const evmc_address* address,
                         const evmc_bytes32* key) {
  evmc_bytes32 value;
  return value;
}

enum evmc_storage_status set_storage(struct evmc_host_context* context,
                                     const evmc_address* address,
                                     const evmc_bytes32* key,
                                     const evmc_bytes32* value) {

#ifdef TEST_BIN
  size_t i;
  printf("[set_storage] address: ");
  for (i = 0; i < 20; i++) {
    printf("%02x", *(address->bytes+i));
  }
  printf("\n");

  printf("[set_storage]     key: ");
  for (i = 0; i < 32; i++) {
    printf("%02x", *(key->bytes+i));
  }
  printf("\n");

  printf("[set_storage]   value: ");
  for (i = 0; i < 32; i++) {
    printf("%02x", *(value->bytes+i));
  }
  printf("\n");
#endif

  context->address = *address;
  context->key = *key;
  context->value = *value;
  return EVMC_STORAGE_ADDED;
}
void emit_log(struct evmc_host_context* context,
              const evmc_address* address,
              const uint8_t* data,
              size_t data_size,
              const evmc_bytes32 topics[],
              size_t topics_count) {

#ifdef TEST_BIN
  size_t i;

  printf("[emit_log] address: ");
  for (i = 0; i < 20; i++) {
    printf("%02x", *(address->bytes+i));
  }
  printf("\n");
  printf("[emit_log]    data: ");
  for (i = 0; i < data_size; i++) {
    printf("%02x", *(data+i));
  }
  printf("\n");

  size_t j;
  for (j = 0; j < topics_count; j++) {
    printf("[emit_log] topic[%d]: ", (int)j);
    for (i = 0; i < 32; i++) {
      printf("%02x", *(topics[j].bytes+i));
    }
    printf("\n");
  }
#else
#endif
}

#define SIGNATURE_LEN 65
#define CALL_KIND_LEN 1
#define FLAGS_LEN 4
#define DEPTH_LEN 4
#define CALL_KIND_OFFSET SIGNATURE_LEN
#define FLAGS_OFFSET (CALL_KIND_OFFSET + CALL_KIND_LEN)
#define DEPTH_OFFSET (FLAGS_OFFSET + FLAGS_LEN)
#define CODE_OFFSET (DEPTH_OFFSET + DEPTH_LEN)

/// NOTE: This program must compile use g++ since evmone implemented with c++17
int execute_vm(const uint8_t *source,
               uint32_t length,
               csal_change_t *existing_values,
               csal_change_t *changes)
{
  const uint8_t call_kind = source[CALL_KIND_OFFSET];
  const uint32_t flags = *(uint32_t *)(source + FLAGS_OFFSET);
  const uint32_t depth = *(uint32_t *)(source + DEPTH_OFFSET);
  const uint32_t code_size = *(uint32_t *)(source + CODE_OFFSET);
  const uint8_t *code_data = source + (CODE_OFFSET + 4);
  const uint32_t input_size = *(uint32_t *)(code_data + code_size);
  const uint8_t *input_data = input_size > 0 ? code_data + (code_size + 4) : NULL;

#ifdef TEST_BIN
  printf("program len: %d\n", length);
  printf("flags: %d\n", flags);
  printf("depth: %d\n", depth);
  printf("input_size: %d\n", input_size);
  printf("input_data: %p\n", (void *)input_data);
  printf("code_size: %d\n", code_size);
  printf("code_data: ");
  for (size_t i = 0; i < code_size; i++) {
    printf("%02x", *(code_data+i));
  }
  printf("\n");
#endif

  struct evmc_vm *vm = evmc_create_evmone();
  struct evmc_host_interface interface = { NULL, get_storage, set_storage, NULL, NULL, NULL, NULL, NULL, NULL, get_tx_context, NULL, emit_log};
  struct evmc_host_context context;
  struct evmc_message msg;
  msg.kind = (evmc_call_kind) call_kind;
  msg.destination.bytes[19] = 4;
  msg.sender.bytes[18] = 3;
  msg.flags = flags;
  msg.depth = depth;
  msg.input_data = input_data;
  msg.input_size = input_size;
  msg.gas = 10000000;

  struct evmc_result res = vm->execute(vm, &interface, &context, EVMC_MAX_REVISION, &msg, code_data, code_size);

#ifdef TEST_BIN
  printf("gas_left: %ld, gas_cost: %ld\n", res.gas_left, msg.gas - res.gas_left);
  printf("output_size: %ld\n", res.output_size);
  printf("output_data: ");
  for (size_t i = 0; i < res.output_size; i++) {
    printf("%02x", *(res.output_data+i));
  }
  printf("\n");
#endif

  return (int)res.status_code;
}
