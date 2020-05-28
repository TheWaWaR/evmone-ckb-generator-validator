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
  evmc_address tx_origin;
  bool destructed;
};

struct evmc_tx_context get_tx_context(struct evmc_host_context* context) {
  printf("[get_tx_context]");
  struct evmc_tx_context ctx{};
  ctx.tx_origin = context->tx_origin;
  return ctx;
}

bool account_exists(struct evmc_host_context* context,
                    const evmc_address* address) {
  printf("[account_exists] address: ");
  for (size_t i = 0; i < 20; i++) {
    printf("%02x", *(address->bytes+i));
  }
  printf("\n");
  return true;
}
evmc_bytes32 get_storage(struct evmc_host_context* context,
                         const evmc_address* address,
                         const evmc_bytes32* key) {
  printf("[get_storage] key: ");
  for (size_t i = 0; i < 32; i++) {
    printf("%02x", *(key->bytes+i));
  }
  printf("\n");
  evmc_bytes32 value{};
  return value;
}

enum evmc_storage_status set_storage(struct evmc_host_context* context,
                                     const evmc_address* address,
                                     const evmc_bytes32* key,
                                     const evmc_bytes32* value) {
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
  context->address = *address;
  context->key = *key;
  context->value = *value;

  return EVMC_STORAGE_ADDED;
}

size_t get_code_size(struct evmc_host_context* context,
                     const evmc_address* address) {
  printf("[get_code_size] address: ");
  for (size_t i = 0; i < 20; i++) {
    printf("%02x", *(address->bytes+i));
  }
  printf("\n");
  return 0;
}

evmc_bytes32 get_code_hash(struct evmc_host_context* context,
                           const evmc_address* address) {
  printf("[get_code_hash] address: ");
  for (size_t i = 0; i < 20; i++) {
    printf("%02x", *(address->bytes+i));
  }
  printf("\n");
  evmc_bytes32 hash{};
  return hash;
}

size_t copy_code(struct evmc_host_context* context,
                 const evmc_address* address,
                 size_t code_offset,
                 uint8_t* buffer_data,
                 size_t buffer_size) {
  printf("[copy_code] address: ");
  for (size_t i = 0; i < 20; i++) {
    printf("%02x", *(address->bytes+i));
  }
  printf("\n");
  printf("  > code_offset: %ld\n", code_offset);
  printf("  > buffer_data: 0x");
  for (size_t i = 0; i < buffer_size; i++) {
    printf("%02x", *(buffer_data+i));
  }
  printf("\n");
  return 0;
}

evmc_uint256be get_balance(struct evmc_host_context* context,
                           const evmc_address* address) {
  printf("[get_balance] address: ");
  for (size_t i = 0; i < 20; i++) {
    printf("%02x", *(address->bytes+i));
  }
  printf("\n");
  evmc_uint256be balance{};
  return balance;
}

void selfdestruct(struct evmc_host_context* context,
                       const evmc_address* address,
                       const evmc_address* beneficiary) {
  printf("[selfdestruct] beneficiary: ");
  for (size_t i = 0; i < 20; i++) {
    printf("%02x", *(beneficiary->bytes+i));
  }
  printf("\n");
}
void emit_log(struct evmc_host_context* context,
              const evmc_address* address,
              const uint8_t* data,
              size_t data_size,
              const evmc_bytes32 topics[],
              size_t topics_count) {

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
}

inline void check_params(const uint8_t call_kind,
                         const uint32_t flags,
                         const uint32_t depth,
                         const evmc_address *sender,
                         const evmc_address *destination,
                         const uint32_t code_size,
                         const uint8_t *code_data,
                         const uint32_t input_size,
                         const uint8_t *input_data) {
  printf("call_kind: %d\n", call_kind);
  printf("flags: %d\n", flags);
  printf("depth: %d\n", depth);
  printf("sender: ");
  for (size_t i = 0; i < 20; i++) {
    printf("%02x", *(sender->bytes+i));
  }
  printf("\n");
  printf("destination: ");
  for (size_t i = 0; i < 20; i++) {
    printf("%02x", *(destination->bytes+i));
  }
  printf("\n");
  printf("input_size: %d\n", input_size);
  printf("input_data_ptr: %p\n", (void *)input_data);
  printf("input_data: ");
  for (size_t i = 0; i < input_size; i++) {
    printf("%02x", *(input_data+i));
  }
  printf("\n");
  printf("code_size: %d\n", code_size);
  printf("code_data: ");
  for (size_t i = 0; i < code_size; i++) {
    printf("%02x", *(code_data+i));
  }
  printf("\n");
}

inline void context_init(struct evmc_host_context* context,
                         csal_change_t *_existing_values,
                         csal_change_t *changes,
                         evmc_address tx_origin) {
  /* Do nothing */
  context->destructed = false;
}

inline void return_result(const struct evmc_message *msg, const struct evmc_result *res) {
  printf("gas_left: %ld, gas_cost: %ld\n", res->gas_left, msg->gas - res->gas_left);
  printf("output_size: %ld\n", res->output_size);
  printf("output_data: ");
  for (size_t i = 0; i < res->output_size; i++) {
    printf("%02x", *(res->output_data+i));
  }
  printf("\n");
}
