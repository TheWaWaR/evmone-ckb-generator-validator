#include <evmc/evmc.h>

#ifdef BUILD_GENERATOR
#include "generator.h"
#else
#define CSAL_VALIDATOR_TYPE 1
#include "validator.h"
#include "secp256k1_helper.h"
#endif

/* Validator */
#ifndef BUILD_GENERATOR
int csal_return(const uint8_t *data, uint32_t data_length) {
  // TODO: check return data
  return 0;
}
int csal_log(const uint8_t *data, uint32_t data_length) {
  return 0;
}
int csal_selfdestruct(const uint8_t *data, uint32_t data_length) {
  // TODO: check selfdestruct
  return 0;
}

int get_script_args(uint8_t *script_data, size_t script_size, mol_seg_t *args_bytes_seg) {
  mol_seg_t script_seg;
  script_seg.ptr = (uint8_t *)script_data;
  script_seg.size = script_size;
  if (MolReader_Script_verify(&script_seg, false) != MOL_OK) {
    return ERROR_INVALID_DATA;
  }
  mol_seg_t args_seg = MolReader_Script_get_args(&script_seg);
  *args_bytes_seg = MolReader_Bytes_raw_bytes(&args_seg);
  if (args_bytes_seg->size != CSAL_SCRIPT_ARGS_LEN) {
    return ERROR_INVALID_DATA;
  }
  return 0;
}
int load_type_script_args(mol_seg_t *args_bytes_seg, size_t index, size_t source) {
  int ret;
  uint8_t type_script[SCRIPT_SIZE];
  uint64_t len = SCRIPT_SIZE;
  ret = ckb_load_cell_by_field(type_script, &len, 0, index, source, CKB_CELL_FIELD_TYPE);
  if (ret != CKB_SUCCESS) {
    return ret;
  }
  return get_script_args(type_script, len, args_bytes_seg);
}
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
  int ret;
  uint64_t len;
  blake2b_state blake2b_ctx;

  uint8_t witness[WITNESS_SIZE];
  size_t cell_source = 0;
  len = WITNESS_SIZE;
  // TODO: support contract call contract
  ret = ckb_load_actual_type_witness(witness, &len, 0, &cell_source);
  if (ret != CKB_SUCCESS) {
    return ret;
  }
  mol_seg_t witness_seg;
  witness_seg.ptr = (uint8_t *)witness;
  witness_seg.size = len;
  debug_print_int("load witness:", (int) witness_seg.size);
  if (MolReader_WitnessArgs_verify(&witness_seg, false) != MOL_OK) {
    return ERROR_INVALID_DATA;
  }
  mol_seg_t content_seg;
  if (cell_source == CKB_SOURCE_GROUP_OUTPUT) {
    content_seg = MolReader_WitnessArgs_get_output_type(&witness_seg);
  } else {
    content_seg = MolReader_WitnessArgs_get_input_type(&witness_seg);
  }
  if (MolReader_BytesOpt_is_none(&content_seg)) {
    return ERROR_INVALID_DATA;
  }
  const mol_seg_t content_bytes_seg = MolReader_Bytes_raw_bytes(&content_seg);

  uint8_t zero_signature[65];
  memset(zero_signature, 0, 65);
  uint8_t signature_data[65];
  memcpy(signature_data, content_bytes_seg.ptr + 4, 65);
  if (memcmp(signature_data, zero_signature, 65) == 0) {
    ckb_debug("verify contract sender signature");
    // contract call contract
    uint8_t script[SCRIPT_SIZE];
    len = SCRIPT_SIZE;
    ret = ckb_checked_load_script(script, &len, 0);
    if (ret != CKB_SUCCESS) {
      return ret;
    }
    mol_seg_t current_args_bytes_seg;
    ret = get_script_args(script, len, &current_args_bytes_seg);
    if (ret != CKB_SUCCESS) {
      return ret;
    }

    bool found_other_contract = false;
    size_t index = 0;
    mol_seg_t args_bytes_seg;
    while(1) {
      load_type_script_args(&args_bytes_seg, index, CKB_SOURCE_INPUT);
      if (memcmp(args_bytes_seg.ptr, current_args_bytes_seg.ptr, CSAL_SCRIPT_ARGS_LEN) != 0
          && memcmp(args_bytes_seg.ptr, sender->bytes, CSAL_SCRIPT_ARGS_LEN) == 0) {
        found_other_contract = true;
        break;
      }
      index += 1;
    }
    if (!found_other_contract) {
      index = 0;
      while(1) {
        load_type_script_args(&args_bytes_seg, index, CKB_SOURCE_OUTPUT);
        if (memcmp(args_bytes_seg.ptr, current_args_bytes_seg.ptr, CSAL_SCRIPT_ARGS_LEN) != 0
            && memcmp(args_bytes_seg.ptr, sender->bytes, CSAL_SCRIPT_ARGS_LEN) == 0) {
          found_other_contract = true;
          break;
        }
        index += 1;
      }
    }
    if (!found_other_contract) {
      // Not a contract sender but with zero signature
      return -90;
    }
  } else {
    // Verify EoA account call contract
    ckb_debug("Verify EoA sender signature");
    memset(content_bytes_seg.ptr + 4, 0, 65);
    uint8_t sign_message[32];
    uint8_t tx_hash[32];
    len = 32;
    ret = ckb_load_tx_hash(tx_hash, &len, 0);
    if (ret != CKB_SUCCESS) {
      return ret;
    }
    blake2b_init(&blake2b_ctx, 32);
    blake2b_update(&blake2b_ctx, tx_hash, 32);
    blake2b_update(&blake2b_ctx, content_bytes_seg.ptr, content_bytes_seg.size);
    blake2b_final(&blake2b_ctx, sign_message, 32);

    /* Load signature */
    secp256k1_context context;
    uint8_t secp_data[CKB_SECP256K1_DATA_SIZE];
    ret = ckb_secp256k1_custom_verify_only_initialize(&context, secp_data);
    if (ret != 0) {
      return ret;
    }

    int recid = (int)signature_data[64];
    secp256k1_ecdsa_recoverable_signature signature;
    if (secp256k1_ecdsa_recoverable_signature_parse_compact(&context, &signature, signature_data, recid) == 0) {
      return -91;
    }

    /* Recover pubkey */
    secp256k1_pubkey pubkey;
    if (secp256k1_ecdsa_recover(&context, &pubkey, &signature, sign_message) != 1) {
      return -92;
    }

    /* Check pubkey hash */
    uint8_t temp[32768];
    size_t pubkey_size = 33;
    if (secp256k1_ec_pubkey_serialize(&context, temp,
                                      &pubkey_size, &pubkey,
                                      SECP256K1_EC_COMPRESSED) != 1) {
      return -93;
    }
    blake2b_init(&blake2b_ctx, 32);
    blake2b_update(&blake2b_ctx, temp, pubkey_size);
    blake2b_final(&blake2b_ctx, temp, 32);

    if (memcmp(sender->bytes, temp, 20) != 0) {
      return -94;
    }
  }

  /*
   * - verify code_hash not changed
   * - verify code_hash in data filed match the blake2b_h256(code_data)
   */
  if (call_kind != EVMC_CREATE) {
    uint8_t code_hash[32];
    blake2b_init(&blake2b_ctx, 32);
    blake2b_update(&blake2b_ctx, code_data, code_size);
    blake2b_final(&blake2b_ctx, code_hash, 32);
    debug_print_data("code: ", code_data, code_size);
    debug_print_data("code_hash: ", code_hash, 32);

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
