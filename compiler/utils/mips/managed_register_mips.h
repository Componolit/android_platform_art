/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_COMPILER_UTILS_MIPS_MANAGED_REGISTER_MIPS_H_
#define ART_COMPILER_UTILS_MIPS_MANAGED_REGISTER_MIPS_H_

#include "constants_mips.h"
#include "debug/dwarf/register.h"
#include "utils/managed_register.h"

namespace art {
namespace mips {

// Values for register pairs.
enum RegisterPair {
  V0_V1 = 0,
  A0_A1 = 1,
  A2_A3 = 2,
  T0_T1 = 3,
  T2_T3 = 4,
  T4_T5 = 5,
  T6_T7 = 6,
  S0_S1 = 7,
  S2_S3 = 8,
  S4_S5 = 9,
  S6_S7 = 10,
  A1_A2 = 11,  // Dalvik style passing
  kNumberOfRegisterPairs = 12,
  kNoRegisterPair = -1,
};

std::ostream& operator<<(std::ostream& os, const RegisterPair& reg);

const int kNumberOfCoreRegIds = kNumberOfCoreRegisters;
const int kNumberOfCoreAllocIds = kNumberOfCoreRegisters;

const int kNumberOfFRegIds = kNumberOfFRegisters;
const int kNumberOfFAllocIds = kNumberOfFRegisters;

const int kNumberOfDRegIds = kNumberOfDRegisters;
const int kNumberOfOverlappingDRegIds = kNumberOfOverlappingDRegisters;
const int kNumberOfDAllocIds = kNumberOfDRegisters;

const int kNumberOfPairRegIds = kNumberOfRegisterPairs;

const int kNumberOfRegIds = kNumberOfCoreRegIds + kNumberOfFRegIds +
    kNumberOfDRegIds + kNumberOfPairRegIds;
const int kNumberOfAllocIds =
    kNumberOfCoreAllocIds + kNumberOfFAllocIds + kNumberOfDAllocIds;

// Register ids map:
//   [0..R[  core registers (enum Register)
//   [R..F[  single precision FP registers (enum FRegister)
//   [F..D[  double precision FP registers (enum DRegister)
//   [D..P[  core register pairs (enum RegisterPair)
// where
//   R = kNumberOfCoreRegIds
//   F = R + kNumberOfFRegIds
//   D = F + kNumberOfDRegIds
//   P = D + kNumberOfRegisterPairs

// Allocation ids map:
//   [0..R[  core registers (enum Register)
//   [R..F[  single precision FP registers (enum FRegister)
// where
//   R = kNumberOfCoreRegIds
//   F = R + kNumberOfFRegIds


// An instance of class 'ManagedRegister' represents a single core register (enum
// Register), a single precision FP register (enum FRegister), a double precision
// FP register (enum DRegister), or a pair of core registers (enum RegisterPair).
// 'ManagedRegister::NoRegister()' provides an invalid register.
// There is a one-to-one mapping between ManagedRegister and register id.
class MipsManagedRegister : public ManagedRegister {
 public:
  constexpr Register AsCoreRegister() const {
    CHECK(IsCoreRegister());
    return static_cast<Register>(id_);
  }

  constexpr FRegister AsFRegister() const {
    CHECK(IsFRegister());
    return static_cast<FRegister>(id_ - kNumberOfCoreRegIds);
  }

  constexpr DRegister AsDRegister() const {
    CHECK(IsDRegister());
    return static_cast<DRegister>(id_ - kNumberOfCoreRegIds - kNumberOfFRegIds);
  }

  constexpr FRegister AsOverlappingDRegisterLow() const {
    CHECK(IsOverlappingDRegister());
    DRegister d_reg = AsDRegister();
    return static_cast<FRegister>(d_reg * 2);
  }

  constexpr FRegister AsOverlappingDRegisterHigh() const {
    CHECK(IsOverlappingDRegister());
    DRegister d_reg = AsDRegister();
    return static_cast<FRegister>(d_reg * 2 + 1);
  }

  constexpr Register AsRegisterPairLow() const {
    CHECK(IsRegisterPair());
    // Appropriate mapping of register ids allows to use AllocIdLow().
    return FromRegId(AllocIdLow()).AsCoreRegister();
  }

  constexpr Register AsRegisterPairHigh() const {
    CHECK(IsRegisterPair());
    // Appropriate mapping of register ids allows to use AllocIdHigh().
    return FromRegId(AllocIdHigh()).AsCoreRegister();
  }

  constexpr bool IsCoreRegister() const {
    CHECK(IsValidManagedRegister());
    return (0 <= id_) && (id_ < kNumberOfCoreRegIds);
  }

  constexpr bool IsFRegister() const {
    CHECK(IsValidManagedRegister());
    const int test = id_ - kNumberOfCoreRegIds;
    return (0 <= test) && (test < kNumberOfFRegIds);
  }

  constexpr bool IsDRegister() const {
    CHECK(IsValidManagedRegister());
    const int test = id_ - (kNumberOfCoreRegIds + kNumberOfFRegIds);
    return (0 <= test) && (test < kNumberOfDRegIds);
  }

  // Returns true if this DRegister overlaps FRegisters.
  constexpr bool IsOverlappingDRegister() const {
    CHECK(IsValidManagedRegister());
    const int test = id_ - (kNumberOfCoreRegIds + kNumberOfFRegIds);
    return (0 <= test) && (test < kNumberOfOverlappingDRegIds);
  }

  constexpr bool IsRegisterPair() const {
    CHECK(IsValidManagedRegister());
    const int test =
        id_ - (kNumberOfCoreRegIds + kNumberOfFRegIds + kNumberOfDRegIds);
    return (0 <= test) && (test < kNumberOfPairRegIds);
  }

  void Print(std::ostream& os) const;

  // Returns true if the two managed-registers ('this' and 'other') overlap.
  // Either managed-register may be the NoRegister. If both are the NoRegister
  // then false is returned.
  bool Overlaps(const MipsManagedRegister& other) const;

  static constexpr MipsManagedRegister FromCoreRegister(Register r) {
    //CHECK_NE(r, kNoRegister);
    return FromRegId(r);
  }

  static constexpr MipsManagedRegister FromFRegister(FRegister r) {
    //CHECK_NE(r, kNoFRegister);
    return FromRegId(r + kNumberOfCoreRegIds);
  }

  static constexpr MipsManagedRegister FromDRegister(DRegister r) {
    //CHECK_NE(r, kNoDRegister);
    return FromRegId(r + kNumberOfCoreRegIds + kNumberOfFRegIds);
  }

  static constexpr MipsManagedRegister FromRegisterPair(RegisterPair r) {
    //CHECK_NE(r, kNoRegisterPair);
    return FromRegId(r + (kNumberOfCoreRegIds + kNumberOfFRegIds + kNumberOfDRegIds));
  }

 private:
  constexpr bool IsValidManagedRegister() const {
    return (0 <= id_) && (id_ < kNumberOfRegIds);
  }

  constexpr int RegId() const {
    CHECK(!IsNoRegister());
    return id_;
  }

  int AllocId() const {
    CHECK(IsValidManagedRegister() && !IsOverlappingDRegister() && !IsRegisterPair());
    CHECK_LT(id_, kNumberOfAllocIds);
    return id_;
  }

  constexpr int AllocIdLow() const {
    CHECK(IsOverlappingDRegister() || IsRegisterPair());
    const int r = RegId() - (kNumberOfCoreRegIds + kNumberOfFRegIds);
    int low = 0;
    if (r < kNumberOfOverlappingDRegIds) {
      CHECK(IsOverlappingDRegister());
      low = (r * 2) + kNumberOfCoreRegIds;  // Return an FRegister.
    } else {
      CHECK(IsRegisterPair());
      low = (r - kNumberOfDRegIds) * 2 + 2;  // Return a Register.
      if (low >= 24) {
        // we got a pair higher than S6_S7, must be the dalvik special case
        low = 5;
      }
    }
    return low;
  }

  constexpr int AllocIdHigh() const {
    return AllocIdLow() + 1;
  }

  friend class ManagedRegister;

  explicit constexpr MipsManagedRegister(int reg_id) : ManagedRegister(reg_id) {}

  static constexpr MipsManagedRegister FromRegId(int reg_id) {
    MipsManagedRegister reg(reg_id);
    CHECK(reg.IsValidManagedRegister());
    return reg;
  }
};

std::ostream& operator<<(std::ostream& os, const MipsManagedRegister& reg);

}  // namespace mips

constexpr inline mips::MipsManagedRegister ManagedRegister::AsMips() const {
  mips::MipsManagedRegister reg(id_);
  CHECK(reg.IsNoRegister() || reg.IsValidManagedRegister());
  return reg;
}

}  // namespace art

#endif  // ART_COMPILER_UTILS_MIPS_MANAGED_REGISTER_MIPS_H_
