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
  WasmEncoderBuilder e;
  WasmFunctionBuilder f;
  f.ReturnType(kAstInt32);
  f.Exported(1);
  f.AddStatement(kStmtReturn);
  f.AddStatement(kExprInt8Const);
  f.AddStatement(kReturnValue);
  e.AddFunction(f.Build());
  WasmEncoder we = e.WriteAndBuild(&zone);
  Isolate* isolate = CcTest::InitIsolateOnce();
  int32_t result =
      CompileAndRunWasmModule(isolate, we.Begin(), we.End());
  CHECK_EQ(kReturnValue, result);
}


/*TEST(Run_WasmModule_CallAdd) {
  static const int kModuleHeaderSize = 6;
  static const int kFunctionSize = 24;
  static const byte kCodeStartOffset0 =
      kModuleHeaderSize + 2 + kFunctionSize * 2;
  static const byte kCodeEndOffset0 = kCodeStartOffset0 + 6;
  static const byte kCodeStartOffset1 = kCodeEndOffset0;
  static const byte kCodeEndOffset1 = kCodeEndOffset0 + 7;
  static const byte data[] = {
      MODULE_HEADER(0, 2, 0),  // globals, functions, data segments
      // func#0 -----------------------------------------
      2, kAstInt32, kAstInt32, kAstInt32,  // signature: int,int -> int
      0, 0, 0, 0,                          // name offset
      kCodeStartOffset0, 0, 0, 0,          // code start offset
      kCodeEndOffset0, 0, 0, 0,            // code end offset
      0, 0,                                // local int32 count
      0, 0,                                // local int64 count
      0, 0,                                // local float32 count
      0, 0,                                // local float64 count
      0,                                   // exported
      0,                                   // external
      // func#1 -----------------------------------------
      0, kAstInt32,                // signature: void -> int
      0, 0, 0, 0,                  // name offset
      kCodeStartOffset1, 0, 0, 0,  // code start offset
      kCodeEndOffset1, 0, 0, 0,    // code end offset
      0, 0,                        // local int32 count
      0, 0,                        // local int64 count
      0, 0,                        // local float32 count
      0, 0,                        // local float64 count
      1,                           // exported
      0,                           // external
      // body#0 -----------------------------------------
      kStmtReturn,       // --
      kExprInt32Add,     // --
      kExprGetLocal, 0,  // --
      kExprGetLocal, 1,  // --
      // body#1 -----------------------------------------
      kStmtReturn,           // --
      kExprCallFunction, 0,  // --
      kExprInt8Const, 77,    // --
      kExprInt8Const, 22     // --
  };

  Isolate* isolate = CcTest::InitIsolateOnce();
  int32_t result =
      CompileAndRunWasmModule(isolate, data, data + arraysize(data));
  CHECK_EQ(99, result);
}*/

TEST(Run_WasmModule_CallAdd) {
  Zone zone;
  WasmEncoderBuilder e;
  WasmFunctionBuilder f1;
  f1.ReturnType(kAstInt32);
  f1.AddParam(kAstInt32);
  f1.AddParam(kAstInt32);
  f1.AddStatement(kStmtReturn);
  f1.AddStatement(kExprInt32Add);
  f1.AddStatement(kExprGetLocal);
  f1.AddStatement(0);
  f1.AddStatement(kExprGetLocal);
  f1.AddStatement(1);
  e.AddFunction(f1.Build());
  WasmFunctionBuilder f2;
  f2.ReturnType(kAstInt32);
  f2.Exported(1);
  f2.AddStatement(kStmtReturn);
  f2.AddStatement(kExprCallFunction);
  f2.AddStatement(0);
  f2.AddStatement(kExprInt8Const);
  f2.AddStatement(77);
  f2.AddStatement(kExprInt8Const);
  f2.AddStatement(22);
  e.AddFunction(f2.Build());
  WasmEncoder we = e.WriteAndBuild(&zone);
  Isolate* isolate = CcTest::InitIsolateOnce();
  int32_t result =
      CompileAndRunWasmModule(isolate, we.Begin(), we.End());
  CHECK_EQ(99, result);
}

#endif  // V8_TURBOFAN_TARGET
