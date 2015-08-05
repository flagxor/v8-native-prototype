// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_WASM_ENCODER_H_
#define V8_WASM_ENCODER_H_

#include "src/signature.h"
#include "src/zone-containers.h"

#include "src/base/smart-pointers.h"

#include "src/wasm/wasm-opcodes.h"
#include "src/wasm/wasm-result.h"

namespace v8 {
namespace internal {
namespace wasm {


struct WasmFunctionEntry {
  WasmFunctionEntry(Zone* zone)
    : arguments(zone) {}

  ZoneVector<byte> arguments;
  byte return_type;
  uint32_t name_offset;
  uint32_t code_start_offset;
  uint32_t code_end_offset;
  uint16_t local_int32_count;
  uint16_t local_int64_count;
  uint16_t local_float32_count;
  uint16_t local_float4_count;
  uint8_t exported;
  uint8_t external;
};

class WasmEncoder {
 public:
  WasmEncoder(Zone* zone)
      : zone_(zone),
        buffer_(zone_) {}

  const byte* ModuleBegin() { return final_buffer_.data(); }
  const byte* ModuleEnd() {
    return final_buffer_.data() + final_buffer_.size();
  }

  void AddUint8(uint8_t x);
  void AddUint16(uint16_t x);
  void AddUint32(uint32_t x);

  void AddModuleHeader(uint16_t global_count, uint16_t functions_count,
                       uint16_t data_segments_count);

  void AddFunction(uint8_t return_type);
  void Assemble();

 private:
  Zone *zone_;
  ZoneVector<byte> final_buffer_;
  ZoneVector<byte> buffer_;
  ZoneVector<WasmFunctionEntry> functions_;
};


}
}
}


#endif  // V8_WASM_ENCODER_H_
