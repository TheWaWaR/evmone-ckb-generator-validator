#include <evmone/evmone.h>
#include <evmc/evmc.h>

#ifdef BUILD_GENERATOR
#include "generator.h"
#else
#define CSAL_VALIDATOR_TYPE 1
#include "validator.h"
#endif

#ifdef TEST_BIN
#include "vm_test.h"
#else
#include "vm_prod.h"
#endif

#define SIGNATURE_LEN 65
#define PROGRAM_LEN 4
#define CALL_KIND_LEN 1
#define FLAGS_LEN 4
#define DEPTH_LEN 4
#define ADDRESS_LEN 20
#define CALL_KIND_OFFSET (SIGNATURE_LEN + PROGRAM_LEN)
#define FLAGS_OFFSET (CALL_KIND_OFFSET + CALL_KIND_LEN)
#define DEPTH_OFFSET (FLAGS_OFFSET + FLAGS_LEN)
#define SENDER_OFFSET (DEPTH_OFFSET + DEPTH_LEN)
#define DESTINATION_OFFSET (SENDER_OFFSET + ADDRESS_LEN)
#define CODE_OFFSET (DESTINATION_OFFSET + ADDRESS_LEN)

/// NOTE: This program must compile use g++ since evmone implemented with c++17
int execute_vm(const uint8_t *source,
               uint32_t length,
               csal_change_t *existing_values,
               csal_change_t *changes)
{
  const uint8_t call_kind = source[CALL_KIND_OFFSET];
  const uint32_t flags = *(uint32_t *)(source + FLAGS_OFFSET);
  const uint32_t depth = *(uint32_t *)(source + DEPTH_OFFSET);
  const evmc_address sender = *(evmc_address *)(source + SENDER_OFFSET);
  const evmc_address destination = *(evmc_address *)(source + DESTINATION_OFFSET);
  const uint32_t code_size = *(uint32_t *)(source + CODE_OFFSET);
  const uint8_t *code_data = source + (CODE_OFFSET + 4);
  const uint32_t input_size = *(uint32_t *)(code_data + code_size);
  const uint8_t *input_data = input_size > 0 ? code_data + (code_size + 4) : NULL;

  check_params(call_kind, flags, depth, &sender, &destination, code_size, code_data, input_size, input_data);

  struct evmc_vm *vm = evmc_create_evmone();
  struct evmc_host_interface interface = { NULL, get_storage, set_storage, NULL, NULL, NULL, NULL, NULL, NULL, get_tx_context, NULL, emit_log};
  struct evmc_host_context context;
  context_init(&context, existing_values, changes);

  struct evmc_message msg;
  msg.kind = (evmc_call_kind) call_kind;
  msg.destination = destination;
  msg.sender = sender;
  msg.flags = flags;
  msg.depth = depth;
  msg.input_data = input_data;
  msg.input_size = input_size;
  msg.gas = 10000000;

  struct evmc_result res = vm->execute(vm, &interface, &context, EVMC_MAX_REVISION, &msg, code_data, code_size);

  return_result(&msg, &res);

  return (int)res.status_code;
}
