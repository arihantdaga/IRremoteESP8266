// Quick and dirty tool to decode Raw Codes, GlobalCache (GC) codes
// and ProntoHex codes
// Copyright 2017 Jorge Cisneros

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "IRac.h"
#include "IRsend.h"
#include "IRsend_test.h"
#include "IRutils.h"

const uint16_t kMaxGcCodeLength = 10000;

void str_to_uint16(char *str, uint16_t *res, uint8_t base) {
  char *end;
  errno = 0;
  intmax_t val = strtoimax(str, &end, base);
  if (errno == ERANGE || val < 0 || val > UINT16_MAX || end == str ||
      *end != '\0')
    return;
  *res = (uint16_t)val;
}

void usage_error(char *name) {
  std::cerr << "Usage: " << name << " [-gc] [-rawdump] <global_code>"
            << std::endl
            << "Usage: " << name << " -prontohex [-rawdump] <prontohex_code>"
            << std::endl
            << "Usage: " << name << " -raw [-rawdump] <raw_code>" << std::endl;
}

int main(int argc, char *argv[]) {
  int argv_offset = 1;
  bool dumpraw = false;
  enum decode_type_t input_type = GLOBALCACHE;
  const uint16_t raw_freq = 38;
  // Check the invocation/calling usage.
  if (argc < 2 || argc > 4) {
    usage_error(argv[0]);
    return 1;
  }
  if (strncmp("-gc", argv[argv_offset], 3) == 0) {
    argv_offset++;
  } else if (strncmp("-prontohex", argv[argv_offset], 10) == 0) {
    input_type = PRONTO;
    argv_offset++;
  } else if (strncmp("-raw", argv[argv_offset], 4) == 0) {
    input_type = RAW;
    argv_offset++;
  }

  if (strncmp("-rawdump", argv[argv_offset], 7) == 0) {
    dumpraw = true;
    argv_offset++;
  }

  if (argc - argv_offset != 1) {
    usage_error(argv[0]);
    return 1;
  }

  uint16_t gc_test[kMaxGcCodeLength];
  int index = 0;
  char *pch;
  char *saveptr1;
  char *sep = const_cast<char *>(",");
  int codebase = 10;
  if (input_type == PRONTO) {
    sep = const_cast<char *>(" ");
    codebase = 16;
  }

  pch = strtok_r(argv[argv_offset], sep, &saveptr1);
  while (pch != NULL && index < kMaxGcCodeLength) {
    str_to_uint16(pch, &gc_test[index], codebase);
    pch = strtok_r(NULL, sep, &saveptr1);
    index++;
  }

  IRsendTest irsend(4);
  IRrecv irrecv(4);
  irsend.begin();
  irsend.reset();

  switch (input_type) {
    case GLOBALCACHE:
      irsend.sendGC(gc_test, index);
      break;
    case PRONTO:
      irsend.sendPronto(gc_test, index);
      break;
    case RAW:
      irsend.sendRaw(gc_test, index, raw_freq);
      break;
    default:
      break;
  }
  irsend.makeDecodeResult();
  irrecv.decode(&irsend.capture);

  char output[500] = "";
  String dummy = "2";
  uint16_t code_len = index;
  uint16_t code_type = irsend.capture.decode_type;
  uint16_t code_bits = irsend.capture.bits;
  bool is_ac = hasACState(irsend.capture.decode_type);
  char state_value[128] = "";
  String description = "";
  char code_value[32] = "";
  char code_addres[32] = "";
  char code_command[32] = "";

  if (is_ac) {
    for (uint16_t i = 0; i < irsend.capture.bits / 8; i++) {
      snprintf(state_value, sizeof(state_value), "%s%02X", state_value,
               irsend.capture.state[i]);
    }
    description = IRAcUtils::resultAcToString(&irsend.capture);
  } else {
    // snprintf(code_value, sizeof(code_value), "%s",
    //          String(irsend.capture.value, 16).c_str());
    // snprintf(code_addres, sizeof(code_addres), "%s",
    //          String(irsend.capture.address, 16).c_str());
    // snprintf(code_command, sizeof(code_command), "%s",
    //          String(irsend.capture.command, 16).c_str());
  }

  sprintf(output,
          "{\"code_length\":\"%d\", \"code_type\":\"%d\", "
          "\"code_bits\":\"%d\", \"is_ac\":\"%d\", \"state_value\":\"%s\", "
          "\"description\":\"%s\", \"code_value\":\"%llu\", "
          "\"code_addres\":\"%lu\", \"code_command\":\"%lu\"}",
          code_len, code_type, code_bits, (int)is_ac, state_value,
          description.c_str(), irsend.capture.value, irsend.capture.address,
          irsend.capture.command);

  std::cout << output << std::endl;
  if (dumpraw || irsend.capture.decode_type == UNKNOWN) irsend.dumpRawResult();
  return 0;
}
