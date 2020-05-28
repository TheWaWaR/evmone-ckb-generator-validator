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
  evmc_address tx_origin;
  bool destructed;
};

struct evmc_tx_context get_tx_context(struct evmc_host_context* context) {
  struct evmc_tx_context ctx{};
  ctx.tx_origin = context->tx_origin;
  return ctx;
}

bool account_exists(struct evmc_host_context* context,
                    const evmc_address* address) {
  return true;
}

evmc_bytes32 get_storage(struct evmc_host_context* context,
                         const evmc_address* address,
                         const evmc_bytes32* key) {
  evmc_bytes32 value{};
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

size_t get_code_size(struct evmc_host_context* context,
                     const evmc_address* address) {
  return 0;
}

evmc_bytes32 get_code_hash(struct evmc_host_context* context,
                           const evmc_address* address) {
  evmc_bytes32 hash{};
  return hash;
}

size_t copy_code(struct evmc_host_context* context,
                 const evmc_address* address,
                 size_t code_offset,
                 uint8_t* buffer_data,
                 size_t buffer_size) {
  return 0;
}

evmc_uint256be get_balance(struct evmc_host_context* context,
                           const evmc_address* address) {
  // TODO: how to return balance?
  evmc_uint256be balance{};
  return balance;
}

void selfdestruct(struct evmc_host_context* context,
                  const evmc_address* address,
                  const evmc_address* beneficiary) {
  context->destructed = true;
  csal_selfdestruct(beneficiary->bytes, 20);
}

void emit_log(struct evmc_host_context* context,
              const evmc_address* address,
              const uint8_t* data,
              size_t data_size,
              const evmc_bytes32 topics[],
              size_t topics_count) {
  uint8_t buffer[2048];
  uint32_t offset = 0;
  uint32_t the_data_size = (uint32_t)data_size;
  uint32_t the_topics_count = (uint32_t)topics_count;
  size_t i;
  for (i = 0; i < sizeof(uint32_t); i++) {
    buffer[offset++] = *((uint8_t *)(&the_data_size) + i);
  }
  for (i = 0; i < data_size; i++) {
    buffer[offset++] = data[i];
  }
  for (i = 0; i < sizeof(uint32_t); i++) {
    buffer[offset++] = *((uint8_t *)(&the_topics_count) + i);
  }
  for (i = 0; i < topics_count; i++) {
    const evmc_bytes32 *topic = topics + i;
    for (size_t j = 0; j < 32; j++) {
      buffer[offset++] = topic->bytes[j];
    }
  }
  csal_log(buffer, offset);
}

inline int verify_params(const uint8_t call_kind,
                        const uint32_t flags,
                        const uint32_t depth,
                        const evmc_address *sender,
                        const evmc_address *destination,
                        const uint32_t code_size,
                        const uint8_t *code_data,
                        const uint32_t input_size,
                        const uint8_t *input_data) {
#ifdef BUILD_GENERATOR
  /* Do nothing */
  return 0;
#else
  // TODO:
  //   * verify the sender recovery from signature match the sender from program

  /*
   * - verify code_hash not changed
   * - verify code_hash in data filed match the blake2b_h256(code_data)
   */
  if (call_kind != EVMC_CREATE) {
    uint8_t code_hash[32];
    blake2b_state blake2b_ctx;
    blake2b_init(&blake2b_ctx, 32);
    blake2b_update(&blake2b_ctx, code_data, code_size);
    blake2b_final(&blake2b_ctx, code_hash, 32);
    debug_print_data("code: ", code_data, code_size);
    debug_print_data("code_hash: ", code_hash, 32);

    int ret;
    uint64_t len;
    uint8_t hash[32];
    len = 32;
    ret = ckb_load_cell_data(hash, &len, 32, 0, CKB_SOURCE_GROUP_INPUT);
    if (ret != CKB_SUCCESS) {
      return ret;
    }
    if (len != 32) {
      return -100;
    }
    debug_print_data("input code hash: ", hash, 32);
    if (memcmp(code_hash, hash, 32) != 0) {
      return -101;
    }

    len = 32;
    ret = ckb_load_cell_data(hash, &len, 32, 0, CKB_SOURCE_GROUP_OUTPUT);
    if (ret == CKB_SUCCESS) {
      if (len != 32) {
        return -102;
      }
      debug_print_data("output code hash: ", hash, 32);
      if (memcmp(code_hash, hash, 32) != 0) {
        return -103;
      }
    } else if (ret != CKB_INDEX_OUT_OF_BOUND) {
      // Not destroy
      return ret;
    }
  }
  return 0;
#endif
}

inline void context_init(struct evmc_host_context* context,
                         csal_change_t *existing_values,
                         csal_change_t *changes,
                         evmc_address tx_origin) {
  context->existing_values = existing_values;
  context->changes = changes;
  context->tx_origin = tx_origin;
  context->destructed = false;
}

inline void return_result(const struct evmc_message *_msg, const struct evmc_result *res) {
  if (res->status_code == EVMC_SUCCESS) {
    csal_return(res->output_data, res->output_size);
  }
}

inline int verify_result(const struct evmc_message *msg, const struct evmc_result *res) {
#ifdef BUILD_GENERATOR
  /* Do nothing */
  return 0;
#else
  if (msg->kind != EVMC_CREATE) {
    return 0;
  }

  /*
   * verify code_hash in output data filed match the blake2b_h256(res.output_data)
   */
  uint8_t code_hash[32];
  blake2b_state blake2b_ctx;
  blake2b_init(&blake2b_ctx, 32);
  blake2b_update(&blake2b_ctx, res->output_data, res->output_size);
  blake2b_final(&blake2b_ctx, code_hash, 32);
  debug_print_data("code: ", res->output_data, res->output_size);
  debug_print_data("code_hash: ", code_hash, 32);

  int ret;
  uint8_t hash[32];
  uint64_t len = 32;
  ret = ckb_load_cell_data(hash, &len, 32, 0, CKB_SOURCE_GROUP_OUTPUT);
  if (ret != CKB_SUCCESS) {
    return ret;
  }
  if (len != 32) {
    return -110;
  }
  debug_print_data("output code hash: ", hash, 32);
  if (memcmp(code_hash, hash, 32) != 0) {
    return -111;
  }
  return 0;
#endif
}
