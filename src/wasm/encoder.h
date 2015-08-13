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


class WasmFunctionEncoder {
public:
  uint32_t HeaderSize(void) const;
  uint32_t BodySize(void) const;
  void Serialize(byte*, uint32_t, uint32_t) const;
private:
  WasmFunctionEncoder(uint8_t return_type,
      const ZoneVector<uint8_t>& params,
      uint16_t local_int32_count,
      uint16_t local_int64_count,
      uint16_t local_float32_count,
      uint16_t local_float64_count,
      uint8_t exported,
      uint8_t external,
      const ZoneVector<uint8_t>& body);
  friend class WasmFunctionBuilder;
  uint8_t return_type_;
  ZoneVector<uint8_t> params_;
  uint16_t local_int32_count_;
  uint16_t local_int64_count_;
  uint16_t local_float32_count_;
  uint16_t local_float64_count_;
  uint8_t exported_;
  uint8_t external_;
  ZoneVector<uint8_t> body_;
  void SerializeFunctionHeader(byte* buffer, uint32_t header_begin, uint32_t body_begin) const;
  void SerializeFunctionBody(byte* buffer, uint32_t header_begin, uint32_t body_begin) const;
};

class WasmFunctionBuilder {
public:
  WasmFunctionBuilder(Zone*);
  void AddParam(uint8_t);
  void ReturnType(uint8_t);
  void AddBody(const ZoneVector<uint8_t>&);
  void Exported(uint8_t);
  void External(uint8_t);
  WasmFunctionEncoder Build(void) const;
private:
  uint8_t return_type_;
  ZoneVector<uint8_t> params_;
  uint16_t local_int32_count_;
  uint16_t local_int64_count_;
  uint16_t local_float32_count_;
  uint16_t local_float64_count_;
  uint8_t exported_;
  uint8_t external_;
  ZoneVector<uint8_t> body_;
};

class WasmEncoder {
 public:
  const byte* Begin() const {return begin_;}
  const byte* End() const {return end_;}
private:
  friend class WasmEncoderBuilder;
  WasmEncoder(const byte* begin, const byte* end)
              :begin_(begin), end_(end){}
  const byte* begin_;
  const byte* end_;
};

class WasmEncoderBuilder {
public:
  WasmEncoderBuilder(Zone*);
  void AddFunction(const WasmFunctionEncoder&);
  WasmEncoder WriteAndBuild(Zone*) const;
private:
  ZoneVector<WasmFunctionEncoder> functions_;
};


}
}
}


#endif  // V8_WASM_ENCODER_H_
