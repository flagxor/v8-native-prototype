// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/signature.h"

#include "src/v8.h"
#include "src/zone-containers.h"

#include "src/wasm/encoder.h"
#include "src/wasm/wasm-module.h"
#include "src/wasm/wasm-opcodes.h"

namespace v8 {
namespace internal {
namespace wasm {


void WasmEncoder::AddUint8(uint8_t x) {
  buffer_.push_back(x);
}

void WasmEncoder::AddUint16(uint16_t x) {
  AddUint8(x);
  AddUint8(x >> 8);
}

void WasmEncoder::AddUint32(uint32_t x) {
  AddUint16(x);
  AddUint16(x >> 16);
}

void WasmEncoder::AddModuleHeader(
    uint16_t global_count, uint16_t functions_count,
    uint16_t data_segments_count) {
  AddUint16(global_count);
  AddUint16(functions_count);
  AddUint16(data_segments_count);
}

void WasmEncoder::AddFunction(uint8_t return_type) {
  WasmFunctionEntry e;
  e.return_type = return_type;
  e.name_offset = 0;
  e.code_start_offset = 0;
  e.code_end_offset = 0;
  e.local_int32_count = 0;
  e.local_int64_count = 0;
  e.local_float32_count = 0;
  e.local_float64_count = 0;
  e.exported = 0;
  e.external = 0;
  functions_.push_back(e);
}

void WasmEncoder::Assemble() {
  final_buffer_.clear();
}


}
}
}
