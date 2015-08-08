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


void AddUint8(byte*& b, uint8_t x) {
  *b = x;
  b++;
}

void AddUint16(byte*& b, uint16_t x) {
  AddUint8(b, x);
  AddUint8(b, x >> 8);
}

void AddUint32(byte*& b, uint32_t x) {
  AddUint16(b, x);
  AddUint16(b, x >> 16);
}

WasmFunctionBuilder::WasmFunctionBuilder() {
  return_type_ = kAstInt32;
  local_int32_count_ = 0;
  local_int64_count_ = 0;
  local_float32_count_ = 0;
  local_float64_count_ = 0;
  exported_ = 0;
  external_ = 0;
}

void WasmFunctionBuilder::AddParam(uint8_t param) {
  params_.push_back(param);
}

void WasmFunctionBuilder::ReturnType(uint8_t type) {
  return_type_ = type;
}

void WasmFunctionBuilder::AddStatement(uint8_t stmt) {
  stmts_.push_back(stmt);
}

void WasmFunctionBuilder::Exported(uint8_t flag) {
  exported_ = flag;
}

void WasmFunctionBuilder::External(uint8_t flag) {
  external_ = flag;
}

WasmFunctionEncoder WasmFunctionBuilder::Build() {
  return WasmFunctionEncoder::WasmFunctionEncoder(return_type_,
                          params_,
                          local_int32_count_,
                          local_int64_count_,
                          local_float32_count_,
                          local_float64_count_,
                          exported_,
                          external_,
                          stmts_);
}

WasmFunctionEncoder::WasmFunctionEncoder(uint8_t return_type,
                    std::vector<uint8_t> params,
                    uint16_t local_int32_count,
                    uint16_t local_int64_count,
                    uint16_t local_float32_count,
                    uint16_t local_float64_count,
                    uint8_t exported,
                    uint8_t external,
                    std::vector<uint8_t> stmts)
  : return_type_(return_type),
    params_(params),
    local_int32_count_(local_int32_count),
    local_int64_count_(local_int64_count),
    local_float32_count_(local_float32_count),
    local_float64_count_(local_float64_count),
    exported_(exported),
    external_(external),
    stmts_(stmts){}

uint32_t WasmFunctionEncoder::HeaderSize() {
  static const int kMinFunctionSize = 24;
  return kMinFunctionSize + (uint32_t)params_.size();
}

uint32_t WasmFunctionEncoder::BodySize(void) {
  return (uint32_t)stmts_.size();
}

void WasmFunctionEncoder::Serialize(byte* buffer, uint32_t header_begin, uint32_t body_begin) {
  SerializeFunctionHeader(buffer, header_begin, body_begin);
  SerializeFunctionBody(buffer, header_begin, body_begin);
}

void WasmFunctionEncoder::SerializeFunctionHeader(byte* buffer, uint32_t header_begin, uint32_t body_begin) {
  byte* header = buffer + header_begin;
  //signature
  AddUint8(header, (uint32_t)params_.size());
  for (int i = 0; i < (uint32_t)params_.size(); i++) {
    AddUint8(header, params_[i]);
  }
  AddUint8(header, return_type_);
  AddUint32(header, 0);                          //name offset
  AddUint32(header, body_begin);
  AddUint32(header, body_begin + (uint32_t)stmts_.size());
  AddUint16(header, local_int32_count_);
  AddUint16(header, local_int64_count_);
  AddUint16(header, local_float32_count_);
  AddUint16(header, local_float64_count_);
  AddUint8(header, exported_);
  AddUint8(header, external_);
}

void WasmFunctionEncoder::SerializeFunctionBody(byte* buffer, uint32_t header_begin, uint32_t body_begin) {
  byte* body = buffer + body_begin;
  std::memcpy(body, stmts_.data(), stmts_.size());
}

void WasmEncoderBuilder::AddFunction(WasmFunctionEncoder f) {
  functions_.push_back(f);
}

WasmEncoder WasmEncoderBuilder::WriteAndBuild(Zone* z) {
  uint32_t total_size = 3;
  uint32_t body_begin = 3;
  for (int i = 0; i < (uint32_t)functions_.size(); i++) {
    total_size += functions_[i].HeaderSize();
    total_size += functions_[i].BodySize();
    body_begin += functions_[i].HeaderSize();
  }
  ZoneVector<uint8_t> buffer_vector(total_size, z);
  byte* buffer = buffer_vector.data();
  byte* temp = buffer;
  AddUint8(temp, 0);                 //globals
  AddUint8(temp, (uint8_t)functions_.size());
  AddUint8(temp, 0);                 //data segments
  uint32_t header_begin = 3;
  for (int i = 0; i < (uint32_t)functions_.size(); i++) {
    functions_[i].Serialize(buffer, header_begin, body_begin);
    header_begin += functions_[i].HeaderSize();
    body_begin += functions_[i].BodySize();
  }
  return WasmEncoder::WasmEncoder(buffer, buffer + total_size);
}

}
}
}
