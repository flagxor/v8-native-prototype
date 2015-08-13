// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdlib.h>
#include <string.h>

#include "src/wasm/encoder.h"
#include "src/wasm/wasm-macro-gen.h"
#include "src/wasm/wasm-module.h"
#include "src/wasm/wasm-opcodes.h"

#include "test/cctest/cctest.h"

#if V8_TURBOFAN_TARGET

using namespace v8::base;
using namespace v8::internal;
using namespace v8::internal::compiler;
using namespace v8::internal::wasm;

#define MODULE_HEADER(globals_count, functions_count, data_segments_count) \
  static_cast<uint8_t>(globals_count),                                     \
      static_cast<uint8_t>(globals_count >> 8),                            \
      static_cast<uint8_t>(functions_count),                               \
      static_cast<uint8_t>(functions_count >> 8),                          \
      static_cast<uint8_t>(data_segments_count),                           \
      static_cast<uint8_t>(data_segments_count >> 8)

TEST(Run_WasmModule_Return114) {
  static const byte kReturnValue = 114;
  Zone zone;
  WasmEncoderBuilder e(&zone);
  WasmFunctionBuilder f(&zone);
  f.ReturnType(kAstInt32);
  f.Exported(1);
  ZoneVector<uint8_t> body(&zone);
  body.push_back(kStmtReturn);
  body.push_back(kExprInt8Const);
  body.push_back(kReturnValue);
  f.AddBody(body);
  e.AddFunction(f.Build());
  WasmEncoder we = e.WriteAndBuild(&zone);
  Isolate* isolate = CcTest::InitIsolateOnce();
  int32_t result =
      CompileAndRunWasmModule(isolate, we.Begin(), we.End());
  CHECK_EQ(kReturnValue, result);
}

TEST(Run_WasmModule_CallAdd) {
  Zone zone;
  WasmEncoderBuilder e(&zone);
  WasmFunctionBuilder f1(&zone);
  f1.ReturnType(kAstInt32);
  f1.AddParam(kAstInt32);
  f1.AddParam(kAstInt32);
  ZoneVector<uint8_t> body(&zone);
  body.push_back(kStmtReturn);
  body.push_back(kExprInt32Add);
  body.push_back(kExprGetLocal);
  body.push_back(0);
  body.push_back(kExprGetLocal);
  body.push_back(1);
  f1.AddBody(body);
  e.AddFunction(f1.Build());
  WasmFunctionBuilder f2(&zone);
  f2.ReturnType(kAstInt32);
  f2.Exported(1);
  body.clear();
  body.push_back(kStmtReturn);
  body.push_back(kExprCallFunction);
  body.push_back(0);
  body.push_back(kExprInt8Const);
  body.push_back(77);
  body.push_back(kExprInt8Const);
  body.push_back(22);
  f2.AddBody(body);
  e.AddFunction(f2.Build());
  WasmEncoder we = e.WriteAndBuild(&zone);
  Isolate* isolate = CcTest::InitIsolateOnce();
  int32_t result =
      CompileAndRunWasmModule(isolate, we.Begin(), we.End());
  CHECK_EQ(99, result);
}

#endif  // V8_TURBOFAN_TARGET
