// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/assembler.h"
#include "src/macro-assembler.h"

#include "src/wasm/wasm-module.h"

#include "src/compiler/linkage.h"

#include "src/zone.h"

namespace v8 {
namespace internal {
namespace wasm {

using compiler::MachineType;
using compiler::MachineSignature;
using compiler::LocationSignature;
using compiler::CallDescriptor;
using compiler::LinkageLocation;

namespace {
MachineType MachineTypeFor(LocalType type) {
  switch (type) {
    case kAstInt32:
      return compiler::kMachInt32;
    case kAstInt64:
      return compiler::kMachInt64;
    case kAstFloat64:
      return compiler::kMachFloat64;
    case kAstFloat32:
      return compiler::kMachFloat32;
    default:
      UNREACHABLE();
      return compiler::kMachAnyTagged;
  }
}


// Platform-specific configuration for C calling convention.
LinkageLocation regloc(Register reg) {
  return LinkageLocation::ForRegister(Register::ToAllocationIndex(reg));
}


LinkageLocation regloc(DoubleRegister reg) {
  return LinkageLocation::ForRegister(DoubleRegister::ToAllocationIndex(reg));
}


LinkageLocation stackloc(int i) {
  return LinkageLocation::ForCallerFrameSlot(i);
}


#if V8_TARGET_ARCH_IA32
// ===========================================================================
// == ia32 ===================================================================
// ===========================================================================
#define GP_PARAM_REGISTERS eax, edx, ecx, edx, esi, edi
#define GP_RETURN_REGISTERS eax, edx
#define FP_PARAM_REGISTERS xmm1, xmm2, xmm3, xmm4, xmm5, xmm6
#define FP_RETURN_REGISTERS xmm1, xmm2

#elif V8_TARGET_ARCH_X64
// ===========================================================================
// == x64 ====================================================================
// ===========================================================================
#define GP_PARAM_REGISTERS rax, rdx, rcx, rbx, rsi, rdi
#define GP_RETURN_REGISTERS rax, rdx
#define FP_PARAM_REGISTERS xmm1, xmm2, xmm3, xmm4, xmm5, xmm6

#elif V8_TARGET_ARCH_X87
// ===========================================================================
// == x87 ====================================================================
// ===========================================================================
#define GP_PARAM_REGISTERS eax, edx, ecx, edx, esi, edi
#define GP_RETURN_REGISTERS eax, edx

#elif V8_TARGET_ARCH_ARM
// ===========================================================================
// == arm ====================================================================
// ===========================================================================
#define GP_PARAM_REGISTERS r0, r1, r2, r3
#define GP_RETURN_REGISTERS r0, r1


#elif V8_TARGET_ARCH_ARM64
// ===========================================================================
// == arm64 ====================================================================
// ===========================================================================
#define GP_PARAM_REGISTERS x0, x1, x2, x3, x4, x5, x6, x7
#define GP_RETURN_REGISTERS x0, x1

#elif V8_TARGET_ARCH_MIPS
// ===========================================================================
// == mips ===================================================================
// ===========================================================================
#define GP_PARAM_REGISTERS a0, a1, a2, a3
#define GP_RETURN_REGISTERS v0, v1

#elif V8_TARGET_ARCH_MIPS64
// ===========================================================================
// == mips64 =================================================================
// ===========================================================================
#define GP_PARAM_REGISTERS a0, a1, a2, a3, a4, a5, a6, a7
#define GP_RETURN_REGISTERS v0, v1

#elif V8_TARGET_ARCH_PPC || V8_TARGET_ARCH_PPC64
// ===========================================================================
// == ppc & ppc64 ============================================================
// ===========================================================================
#define GP_PARAM_REGISTERS r3, r4, r5, r6, r7, r8, r9, r10
#define GP_RETURN_REGISTERS r3, r4

#else
// ===========================================================================
// == unknown ================================================================
// ===========================================================================
// Don't define anything. We'll just always use the stack.
#endif

// Helper for allocating either an GP or FP reg, or the next stack slot.
struct Allocator {
  Allocator(const Register* gp, int gpc, const DoubleRegister* fp, int fpc)
      : gp_count(gpc),
        gp_offset(0),
        gp_regs(gp),
        fp_count(fpc),
        fp_offset(0),
        fp_regs(fp),
        stack_offset(0) {}

  int gp_count;
  int gp_offset;
  const Register* gp_regs;

  int fp_count;
  int fp_offset;
  const DoubleRegister* fp_regs;

  int stack_offset;

  LinkageLocation Next(LocalType type) {
    if (IsFloatingPoint(type)) {
      // Allocate a floating point register/stack location.
      if (fp_offset < fp_count) {
        return regloc(fp_regs[fp_offset++]);
      } else {
        int offset = -1 - stack_offset;
        stack_offset += Words(type);
        return stackloc(offset);
      }
    } else {
      // Allocate a general purpose register/stack location.
      if (gp_offset < gp_count) {
        return regloc(gp_regs[gp_offset++]);
      } else {
        int offset = -1 - stack_offset;
        stack_offset += Words(type);
        return stackloc(offset);
      }
    }
  }
  bool IsFloatingPoint(LocalType type) {
    return type == kAstFloat32 || type == kAstFloat64;
  }
  int Words(LocalType type) {
    if (kPointerSize < 8 && (type == kAstInt64 || type == kAstFloat64)) {
      return 2;
    }
    return 1;
  }
};
}  // namespace


// General code uses the above configuration data.
CallDescriptor* ModuleEnv::GetWasmCallDescriptor(Zone* zone,
                                                 FunctionSig* fsig) {
  MachineSignature::Builder msig(zone, fsig->return_count(),
                                 fsig->parameter_count());
  LocationSignature::Builder locations(zone, fsig->return_count(),
                                       fsig->parameter_count());

#ifdef GP_RETURN_REGISTERS
  static const Register kGPReturnRegisters[] = {GP_RETURN_REGISTERS};
  static const int kGPReturnRegistersCount =
      static_cast<int>(arraysize(kGPReturnRegisters));
#else
  static const Register* kGPReturnRegisters = nullptr;
  static const int kGPReturnRegistersCount = 0;
#endif

#ifdef FP_RETURN_REGISTERS
  static const DoubleRegister kFPReturnRegisters[] = {FP_RETURN_REGISTERS};
  static const int kFPReturnRegistersCount =
      static_cast<int>(arraysize(kFPReturnRegisters));
#else
  static const DoubleRegister* kFPReturnRegisters = nullptr;
  static const int kFPReturnRegistersCount = 0;
#endif

  Allocator rets(kGPReturnRegisters, kGPReturnRegistersCount,
                 kFPReturnRegisters, kFPReturnRegistersCount);

  // Add return location(s).
  const int return_count = static_cast<int>(locations.return_count_);
  for (int i = 0; i < return_count; i++) {
    LocalType ret = fsig->GetReturn(i);
    msig.AddReturn(MachineTypeFor(ret));
    locations.AddReturn(rets.Next(ret));
  }

#ifdef GP_PARAM_REGISTERS
  static const Register kGPParamRegisters[] = {GP_PARAM_REGISTERS};
  static const int kGPParamRegistersCount =
      static_cast<int>(arraysize(kGPParamRegisters));
#else
  static const Register* kGPParamRegisters = nullptr;
  static const int kGPParamRegistersCount = 0;
#endif

#ifdef FP_PARAM_REGISTERS
  static const DoubleRegister kFPParamRegisters[] = {FP_PARAM_REGISTERS};
  static const int kFPParamRegistersCount =
      static_cast<int>(arraysize(kFPParamRegisters));
#else
  static const DoubleRegister* kFPParamRegisters = nullptr;
  static const int kFPParamRegistersCount = 0;
#endif

  Allocator params(kGPParamRegisters, kGPParamRegistersCount, kFPParamRegisters,
                   kFPParamRegistersCount);

  // Add register and/or stack parameter(s).
  const int parameter_count = static_cast<int>(fsig->parameter_count());
  for (int i = 0; i < parameter_count; i++) {
    LocalType param = fsig->GetParam(i);
    msig.AddParam(MachineTypeFor(param));
    locations.AddParam(params.Next(param));
  }

  const RegList kCalleeSaveRegisters = 0;
  const RegList kCalleeSaveFPRegisters = 0;

  // The target for WASM calls is always a code object.
  MachineType target_type = compiler::kMachAnyTagged;
  LinkageLocation target_loc = LinkageLocation::ForAnyRegister();
  return new (zone) CallDescriptor(       // --
      CallDescriptor::kCallCodeObject,    // kind
      target_type,                        // target MachineType
      target_loc,                         // target location
      msig.Build(),                       // machine_sig
      locations.Build(),                  // location_sig
      0,                                  // js_parameter_count
      compiler::Operator::kNoProperties,  // properties
      kCalleeSaveRegisters,               // callee-saved registers
      kCalleeSaveFPRegisters,             // callee-saved fp regs
      CallDescriptor::kNoFlags,           // flags
      "c-call");
}
}
}
}
