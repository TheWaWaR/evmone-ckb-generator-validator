#include <evmc/evmc.h>

#define CSAL_VALIDATOR_TYPE 1
#include "validator.h"
#include "secp256k1_helper.h"

#define MAX_ADDRS 128

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

int check_script_code(const uint8_t *script_data_a,
                      const size_t script_size_a,
                      const uint8_t *script_data_b,
                      const size_t script_size_b,
                      bool *matched) {
  mol_seg_t script_seg_a;
  mol_seg_t script_seg_b;

  script_seg_a.ptr = (uint8_t *)script_data_a;
  script_seg_a.size = script_size_a;
  if (MolReader_Script_verify(&script_seg_a, false) != MOL_OK) {
    return ERROR_INVALID_DATA;
  }
  script_seg_b.ptr = (uint8_t *)script_data_b;
  script_seg_b.size = script_size_b;
  if (MolReader_Script_verify(&script_seg_b, false) != MOL_OK) {
    return ERROR_INVALID_DATA;
  }
  mol_seg_t code_hash_seg_a = MolReader_Script_get_code_hash(&script_seg_a);
  mol_seg_t code_hash_seg_b = MolReader_Script_get_code_hash(&script_seg_b);
  if (code_hash_seg_a.size != code_hash_seg_b.size ||
      memcmp(code_hash_seg_a.ptr, code_hash_seg_b.ptr, code_hash_seg_a.size) != 0) {
    *matched = false;
    return 0;
  }
  mol_seg_t hash_type_seg_a = MolReader_Script_get_hash_type(&script_seg_a);
  mol_seg_t hash_type_seg_b = MolReader_Script_get_hash_type(&script_seg_b);
  if (hash_type_seg_a.size != hash_type_seg_b.size ||
      memcmp(hash_type_seg_a.ptr, hash_type_seg_b.ptr, hash_type_seg_a.size) != 0) {
    *matched = false;
    return 0;
  }
  *matched = true;
  return 0;
}

int load_type_script_args(uint8_t *buf,
                          size_t *script_size,
                          mol_seg_t *args_bytes_seg,
                          size_t index,
                          size_t source) {
  int ret;
  uint64_t len = SCRIPT_SIZE;
  ret = ckb_load_cell_by_field(buf, &len, 0, index, source, CKB_CELL_FIELD_TYPE);
  if (ret != CKB_SUCCESS) {
    return ret;
  }
  *script_size = (size_t)len;
  return get_script_args(buf, len, args_bytes_seg);
}

struct evmc_host_context {
  csal_change_t *existing_values;
  csal_change_t *changes;
  evmc_address tx_origin;
  /* selfdestruct beneficiary */
  evmc_address beneficiary;
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
  memcpy(context->beneficiary.bytes, beneficiary->bytes, 20);
  context->destructed = true;
}

struct evmc_result call(struct evmc_host_context* context,
                        const struct evmc_message* msg) {
  /* TODO: verify CALL (how to fill create_address field) */
  struct evmc_result res{};
  return res;
}

void emit_log(struct evmc_host_context* context,
              const evmc_address* address,
              const uint8_t* data,
              size_t data_size,
              const evmc_bytes32 topics[],
              size_t topics_count) {
  /* Do nothing */
}


static bool touched = false;
static evmc_address the_tx_origin{};
inline int verify_params(const uint8_t *signature_data,
                         const uint8_t call_kind,
                         const uint32_t flags,
                         const uint32_t depth,
                         const evmc_address *tx_origin,
                         const evmc_address *sender,
                         const evmc_address *destination,
                         const uint32_t code_size,
                         const uint8_t *code_data,
                         const uint32_t input_size,
                         const uint8_t *input_data) {
  int ret;
  uint64_t len;
  blake2b_state blake2b_ctx;
  uint8_t zero_signature[65];
  memset(zero_signature, 0, 65);

  uint8_t witness_buf[WITNESS_SIZE];
  /* Verify there is one and only one non-zero signature */
  if (!touched) {
    bool has_entrance_signature = false;
    size_t witness_index = 0;
    len = WITNESS_SIZE;
    while(1) {
      ret = ckb_load_witness(witness_buf, &len, 0, witness_index, CKB_SOURCE_OUTPUT);
      if (ret == CKB_INDEX_OUT_OF_BOUND) {
        break;
      }
      if (ret != CKB_SUCCESS) {
        return ret;
      }
      mol_seg_t witness_seg;
      witness_seg.ptr = (uint8_t *)witness_buf;
      witness_seg.size = len;
      debug_print_int("load witness:", (int) witness_seg.size);
      if (MolReader_WitnessArgs_verify(&witness_seg, false) != MOL_OK) {
        return ERROR_INVALID_DATA;
      }
      mol_seg_t content_seg;
      content_seg = MolReader_WitnessArgs_get_input_type(&witness_seg);
      if (MolReader_BytesOpt_is_none(&content_seg)) {
        content_seg = MolReader_WitnessArgs_get_output_type(&witness_seg);
        if (MolReader_BytesOpt_is_none(&content_seg)) {
          /* Witness without type script */
          continue;
        }
      }
      const mol_seg_t content_bytes_seg = MolReader_Bytes_raw_bytes(&content_seg);
      if (memcmp(content_bytes_seg.ptr+4, zero_signature, 65) != 0) {
        if (has_entrance_signature) {
          /* multiple entrance signature */
          return -80;
        } else {
          has_entrance_signature = true;
        }
      }
      witness_index += 1;
    }
  }

  /* TODO: load once */
  uint8_t script[SCRIPT_SIZE];
  len = SCRIPT_SIZE;
  ret = ckb_checked_load_script(script, &len, 0);
  if (ret != CKB_SUCCESS) {
    return ret;
  }
  size_t script_size = (size_t)len;
  mol_seg_t current_args_bytes_seg;
  ret = get_script_args(script, len, &current_args_bytes_seg);
  if (ret != CKB_SUCCESS) {
    return ret;
  }

  /* Verify sender by signature field */
  if (memcmp(signature_data, zero_signature, 65) == 0) {
    ckb_debug("verify contract sender signature");
    // contract call contract

    bool found_other_contract = false;
    uint8_t type_script[SCRIPT_SIZE];
    size_t type_script_size = 0;
    size_t index = 0;
    mol_seg_t args_bytes_seg{};
    /* Search all inputs type script */
    while(1) {
      ret = load_type_script_args(type_script, &type_script_size, &args_bytes_seg, index, CKB_SOURCE_INPUT);
      if (ret != CKB_SUCCESS) {
        return ret;
      }
      bool code_matched = false;
      ret = check_script_code(script, script_size, type_script, type_script_size, &code_matched);
      if (ret != CKB_SUCCESS) {
        return ret;
      }
      if (code_matched
          && memcmp(args_bytes_seg.ptr, current_args_bytes_seg.ptr, CSAL_SCRIPT_ARGS_LEN) != 0
          && memcmp(args_bytes_seg.ptr, sender->bytes, CSAL_SCRIPT_ARGS_LEN) == 0) {
        found_other_contract = true;
        break;
      }
      index += 1;
    }
    /* Search all outputs type script */
    if (!found_other_contract) {
      index = 0;
      while(1) {
        ret = load_type_script_args(type_script, &type_script_size, &args_bytes_seg, index, CKB_SOURCE_OUTPUT);
        if (ret != CKB_SUCCESS) {
          return ret;
        }
        bool code_matched = false;
        ret = check_script_code(script, script_size, type_script, type_script_size, &code_matched);
        if (ret != CKB_SUCCESS) {
          return ret;
        }
        if (code_matched
            && memcmp(args_bytes_seg.ptr, current_args_bytes_seg.ptr, CSAL_SCRIPT_ARGS_LEN) != 0
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
    if (touched) {
      ckb_debug("ERROR: the non-zero signature is not in the first program");
      return -91;
    }

    ckb_debug("Verify EoA sender signature");
    uint8_t tx_hash[32];
    len = 32;
    ret = ckb_load_tx_hash(tx_hash, &len, 0);
    if (ret != CKB_SUCCESS) {
      return ret;
    }
    blake2b_init(&blake2b_ctx, 32);
    blake2b_update(&blake2b_ctx, tx_hash, 32);

    evmc_address input_addrs[MAX_ADDRS];
    size_t input_addrs_size = 0;
    uint8_t type_script[SCRIPT_SIZE];
    size_t type_script_size = 0;
    size_t input_index = 0;
    while (1) {
      len = SCRIPT_SIZE;
      ret = ckb_load_cell_by_field(type_script, &len, 0, input_index, CKB_SOURCE_INPUT, CKB_CELL_FIELD_TYPE);
      if (ret != CKB_SUCCESS) {
        return ret;
      }
      bool code_matched = false;
      ret = check_script_code(script, script_size, type_script, type_script_size, &code_matched);
      if (ret != CKB_SUCCESS) {
        return ret;
      }
      if (code_matched) {
        mol_seg_t script_seg;
        script_seg.ptr = type_script;
        script_seg.size = len;
        mol_seg_t args_seg = MolReader_Script_get_args(&script_seg);
        mol_seg_t args_bytes_seg = MolReader_Bytes_raw_bytes(&args_seg);
        if (args_bytes_seg.size != CSAL_SCRIPT_ARGS_LEN) {
          return ERROR_INVALID_DATA;
        }
        evmc_address tmp_addr{};
        memcpy(tmp_addr.bytes, args_bytes_seg.ptr, 20);
        input_addrs[input_addrs_size] = tmp_addr;
        input_addrs_size += 1;
        if (input_addrs_size >= MAX_ADDRS) {
          /* too many input addrs */
          return ERROR_INVALID_DATA;
        }
        ret = ckb_load_witness(witness_buf, &len, 0, input_index, CKB_SOURCE_INPUT);
        if (memcmp(args_bytes_seg.ptr, tx_origin->bytes, 20) == 0) {
          memset(witness_buf+4, 0, 65);
        }
        blake2b_update(&blake2b_ctx, witness_buf, 32);
      }
      input_index += 1;
    }

    size_t output_index = 0;
    while (1) {
      len = SCRIPT_SIZE;
      ret = ckb_load_cell_by_field(type_script, &len, 0, output_index, CKB_SOURCE_OUTPUT, CKB_CELL_FIELD_TYPE);
      if (ret != CKB_SUCCESS) {
        return ret;
      }
      bool code_matched = false;
      ret = check_script_code(script, script_size, type_script, type_script_size, &code_matched);
      if (ret != CKB_SUCCESS) {
        return ret;
      }
      if (code_matched) {
        mol_seg_t script_seg;
        script_seg.ptr = type_script;
        script_seg.size = len;
        mol_seg_t args_seg = MolReader_Script_get_args(&script_seg);
        mol_seg_t args_bytes_seg = MolReader_Bytes_raw_bytes(&args_seg);
        if (args_bytes_seg.size != CSAL_SCRIPT_ARGS_LEN) {
          return ERROR_INVALID_DATA;
        }
        bool has_input = false;
        for (size_t addr_idx = 0; addr_idx < input_addrs_size; addr_idx++) {
          if (memcmp(args_bytes_seg.ptr, input_addrs[addr_idx].bytes, 32) == 0) {
            has_input = true;
            break;
          }
        }
        if (!has_input) {
          ret = ckb_load_witness(witness_buf, &len, 0, output_index, CKB_SOURCE_OUTPUT);
          if (memcmp(args_bytes_seg.ptr, tx_origin->bytes, 20) == 0) {
            memset(witness_buf+4, 0, 65);
          }
          blake2b_update(&blake2b_ctx, witness_buf, 32);
        }
      }
      output_index += 1;
    }

    // Verify EoA account call contract
    uint8_t sign_message[32];
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
      return -92;
    }

    /* Recover pubkey */
    secp256k1_pubkey pubkey;
    if (secp256k1_ecdsa_recover(&context, &pubkey, &signature, sign_message) != 1) {
      return -93;
    }

    /* Check pubkey hash */
    uint8_t temp[32768];
    size_t pubkey_size = 33;
    if (secp256k1_ec_pubkey_serialize(&context, temp,
                                      &pubkey_size, &pubkey,
                                      SECP256K1_EC_COMPRESSED) != 1) {
      return -94;
    }
    blake2b_init(&blake2b_ctx, 32);
    blake2b_update(&blake2b_ctx, temp, pubkey_size);
    blake2b_final(&blake2b_ctx, temp, 32);

    /* Verify entrance program sender */
    if (memcmp(sender->bytes, temp, 20) != 0) {
      return -95;
    }
    /* Verify tx_origin */
    if (memcmp(sender->bytes, tx_origin->bytes, 20) != 0) {
      return -96;
    }
  }

  /* Verify tx_origin all the same */
  if (!touched) {
    /* TODO: tx_origin should be the same with next contract */
    memcpy(the_tx_origin.bytes, tx_origin->bytes, 20);
  } else {
    if (memcmp(the_tx_origin.bytes, tx_origin->bytes, 20) != 0) {
      /* tx_origin not the same */
      return -97;
    }
  }

  /* Verify destination match current script args */
  if (memcmp(destination->bytes, current_args_bytes_seg.ptr, CSAL_SCRIPT_ARGS_LEN) != 0) {
    ckb_debug("ERROR: destination not match current script args");
    return -98;
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

  if (!touched) {
    touched = true;
  }
  return 0;
}

inline void context_init(struct evmc_host_context* context,
                         struct evmc_vm *_vm,
                         struct evmc_host_interface *_interface,
                         evmc_address tx_origin,
                         csal_change_t *existing_values,
                         csal_change_t *changes) {
  context->existing_values = existing_values;
  context->changes = changes;
  context->tx_origin = tx_origin;
  context->destructed = false;
  memset(context->beneficiary.bytes, 0, 20);
}

inline void return_result(const struct evmc_message *_msg, const struct evmc_result *res) {
  /* Do nothing */
}

inline int verify_result(struct evmc_host_context* context,
                         const struct evmc_message *msg,
                         const struct evmc_result *res,
                         const uint8_t *return_data,
                         const size_t return_data_size,
                         const evmc_address *beneficiary) {
  if (msg->kind == EVMC_CREATE) {
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
  }

  /* Verify return data */
  if (return_data_size != res->output_size) {
    return -112;
  }
  if (memcmp(return_data, res->output_data, return_data_size) != 0) {
    return -113;
  }
  /* verify selfdestruct */
  if (memcmp(beneficiary->bytes, context->beneficiary.bytes, 20) != 0) {
    return -114;
  }

  /* TODO: Verify res.output_data match the program.return_data */
  return 0;
}