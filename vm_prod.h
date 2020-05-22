#include <evmc/evmc.h>

#ifdef BUILD_GENERATOR
#include "generator.h"
#else
#define CSAL_VALIDATOR_TYPE 1
#include "validator.h"
#endif

struct evmc_host_context {
  csal_change_t *existing_values;
  csal_change_t *changes;
};

struct evmc_tx_context get_tx_context(struct evmc_host_context* context) {
  struct evmc_tx_context ctx;
  return ctx;
}

evmc_bytes32 get_storage(struct evmc_host_context* context,
                         const evmc_address* address,
                         const evmc_bytes32* key) {
  evmc_bytes32 value;
  int ret;
  ret = csal_change_fetch(context->changes, key->bytes, value.bytes);
  if (ret != 0) {
    ret = csal_change_fetch(context->existing_values, key->bytes, value.bytes);
  }
  return value;
}

enum evmc_storage_status set_storage(struct evmc_host_context* context,
                                     const evmc_address* address,
                                     const evmc_bytes32* key,
                                     const evmc_bytes32* value) {
  /* int _ret; */
  csal_change_insert(context->existing_values, key->bytes, value->bytes);
  csal_change_insert(context->changes, key->bytes, value->bytes);
  return EVMC_STORAGE_ADDED;
}
void emit_log(struct evmc_host_context* context,
              const evmc_address* address,
              const uint8_t* data,
              size_t data_size,
              const evmc_bytes32 topics[],
              size_t topics_count) {
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
  /* Do noting */
}

inline void context_init(struct evmc_host_context* context,
                  csal_change_t *existing_values,
                  csal_change_t *changes) {
  context->existing_values = existing_values;
  context->changes = changes;
}

inline void return_result(const struct evmc_message *_msg, const struct evmc_result *res) {
  if (res->status_code == EVMC_SUCCESS) {
    csal_return(res->output_data, res->output_size);
  }
}
