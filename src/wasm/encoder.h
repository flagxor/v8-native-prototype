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


class WasmEncoder {
 public:
  WasmEncoder(Zone* zone)
      : zone_(zone),
        buffer_(zone_) {}

  const void* GetModuleData() { return buffer_.data(); }
  size_t GetModuleSize() { return buffer_.size(); }

 private:
  Zone *zone_;
  ZoneVector<byte> buffer_;
};


}
}
}


#endif  // V8_WASM_ENCODER_H_
