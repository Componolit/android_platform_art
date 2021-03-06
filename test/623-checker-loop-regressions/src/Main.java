/*
 * Copyright (C) 2016 The Android Open Source Project
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

/**
 * Regression tests for loop optimizations.
 */
public class Main {

  /// CHECK-START: int Main.earlyExitFirst(int) loop_optimization (before)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  //
  /// CHECK-START: int Main.earlyExitFirst(int) loop_optimization (after)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  static int earlyExitFirst(int m) {
    int k = 0;
    for (int i = 0; i < 10; i++) {
      if (i == m) {
        return k;
      }
      k++;
    }
    return k;
  }

  /// CHECK-START: int Main.earlyExitLast(int) loop_optimization (before)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  //
  /// CHECK-START: int Main.earlyExitLast(int) loop_optimization (after)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  static int earlyExitLast(int m) {
    int k = 0;
    for (int i = 0; i < 10; i++) {
      k++;
      if (i == m) {
        return k;
      }
    }
    return k;
  }

  /// CHECK-START: int Main.earlyExitNested() loop_optimization (before)
  /// CHECK-DAG: Phi loop:<<Loop1:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop1>>      outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop2:B\d+>> outer_loop:<<Loop1>>
  /// CHECK-DAG: Phi loop:<<Loop2>>      outer_loop:<<Loop1>>
  //
  /// CHECK-START: int Main.earlyExitNested() loop_optimization (after)
  /// CHECK-DAG: Phi loop:<<Loop1:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop1>>      outer_loop:none
  //
  /// CHECK-START: int Main.earlyExitNested() loop_optimization (after)
  /// CHECK-NOT: Phi loop:{{B\d+}} outer_loop:{{B\d+}}
  static int earlyExitNested() {
    int offset = 0;
    for (int i = 0; i < 2; i++) {
      int start = offset;
      // This loop can be removed.
      for (int j = 0; j < 2; j++) {
        offset++;
      }
      if (i == 1) {
        return start;
      }
    }
    return 0;
  }

  // Regression test for b/33774618: transfer operations involving
  // narrowing linear induction should be done correctly.
  //
  /// CHECK-START: int Main.transferNarrowWrap() loop_optimization (before)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  //
  /// CHECK-START: int Main.transferNarrowWrap() loop_optimization (after)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  static int transferNarrowWrap() {
    short x = 0;
    int w = 10;
    int v = 3;
    for (int i = 0; i < 10; i++) {
      v = w + 1;    // transfer on wrap-around
      w = x;   // wrap-around
      x += 2;  // narrowing linear
    }
    return v;
  }

  // Regression test for b/33774618: transfer operations involving
  // narrowing linear induction should be done correctly
  // (currently rejected, could be improved).
  //
  /// CHECK-START: int Main.polynomialShort() loop_optimization (before)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  //
  /// CHECK-START: int Main.polynomialShort() loop_optimization (after)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  static int polynomialShort() {
    int x = 0;
    for (short i = 0; i < 10; i++) {
      x = x - i;  // polynomial on narrowing linear
    }
    return x;
  }

  // Regression test for b/33774618: transfer operations involving
  // narrowing linear induction should be done correctly
  // (currently rejected, could be improved).
  //
  /// CHECK-START: int Main.polynomialIntFromLong() loop_optimization (before)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  //
  /// CHECK-START: int Main.polynomialIntFromLong() loop_optimization (after)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  static int polynomialIntFromLong() {
    int x = 0;
    for (long i = 0; i < 10; i++) {
      x = x - (int) i;  // polynomial on narrowing linear
    }
    return x;
  }

  /// CHECK-START: int Main.polynomialInt() loop_optimization (before)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  //
  /// CHECK-START: int Main.polynomialInt() loop_optimization (after)
  /// CHECK-NOT: Phi
  //
  /// CHECK-START: int Main.polynomialInt() instruction_simplifier$after_bce (after)
  /// CHECK-DAG: <<Int:i\d+>>  IntConstant -45  loop:none
  /// CHECK-DAG:               Return [<<Int>>] loop:none
  static int polynomialInt() {
    int x = 0;
    for (int i = 0; i < 10; i++) {
      x = x - i;
    }
    return x;
  }

  // Regression test for b/34779592 (found with fuzz testing): overflow for last value
  // of division truncates to zero, for multiplication it simply truncates.
  //
  /// CHECK-START: int Main.geoIntDivLastValue(int) loop_optimization (before)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  //
  /// CHECK-START: int Main.geoIntDivLastValue(int) loop_optimization (after)
  /// CHECK-NOT: Phi
  //
  /// CHECK-START: int Main.geoIntDivLastValue(int) instruction_simplifier$after_bce (after)
  /// CHECK-DAG: <<Int:i\d+>> IntConstant 0    loop:none
  /// CHECK-DAG:              Return [<<Int>>] loop:none
  static int geoIntDivLastValue(int x) {
    for (int i = 0; i < 2; i++) {
      x /= 1081788608;
    }
    return x;
  }

  /// CHECK-START: int Main.geoIntMulLastValue(int) loop_optimization (before)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  //
  /// CHECK-START: int Main.geoIntMulLastValue(int) loop_optimization (after)
  /// CHECK-NOT: Phi
  //
  /// CHECK-START: int Main.geoIntMulLastValue(int) instruction_simplifier$after_bce (after)
  /// CHECK-DAG: <<Par:i\d+>> ParameterValue         loop:none
  /// CHECK-DAG: <<Int:i\d+>> IntConstant -194211840 loop:none
  /// CHECK-DAG: <<Mul:i\d+>> Mul [<<Par>>,<<Int>>]  loop:none
  /// CHECK-DAG:              Return [<<Mul>>]       loop:none
  static int geoIntMulLastValue(int x) {
    for (int i = 0; i < 2; i++) {
      x *= 1081788608;
    }
    return x;
  }

  /// CHECK-START: long Main.geoLongDivLastValue(long) loop_optimization (before)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  //
  /// CHECK-START: long Main.geoLongDivLastValue(long) loop_optimization (after)
  /// CHECK-NOT: Phi
  //
  /// CHECK-START: long Main.geoLongDivLastValue(long) instruction_simplifier$after_bce (after)
  /// CHECK-DAG: <<Long:j\d+>> LongConstant 0    loop:none
  /// CHECK-DAG:               Return [<<Long>>] loop:none
  static long geoLongDivLastValue(long x) {
    for (int i = 0; i < 10; i++) {
      x /= 1081788608;
    }
    return x;
  }

  /// CHECK-START: long Main.geoLongMulLastValue(long) loop_optimization (before)
  /// CHECK-DAG: Phi loop:<<Loop:B\d+>> outer_loop:none
  /// CHECK-DAG: Phi loop:<<Loop>>      outer_loop:none
  //
  /// CHECK-START: long Main.geoLongMulLastValue(long) loop_optimization (after)
  /// CHECK-NOT: Phi
  //
  /// CHECK-START: long Main.geoLongMulLastValue(long) instruction_simplifier$after_bce (after)
  /// CHECK-DAG: <<Par:j\d+>>  ParameterValue                    loop:none
  /// CHECK-DAG: <<Long:j\d+>> LongConstant -8070450532247928832 loop:none
  /// CHECK-DAG: <<Mul:j\d+>>  Mul [<<Par>>,<<Long>>]            loop:none
  /// CHECK-DAG:               Return [<<Mul>>]                  loop:none
  static long geoLongMulLastValue(long x) {
    for (int i = 0; i < 10; i++) {
      x *= 1081788608;
    }
    return x;
  }

  public static void main(String[] args) {
    expectEquals(10, earlyExitFirst(-1));
    for (int i = 0; i <= 10; i++) {
      expectEquals(i, earlyExitFirst(i));
    }
    expectEquals(10, earlyExitFirst(11));

    expectEquals(10, earlyExitLast(-1));
    for (int i = 0; i < 10; i++) {
      expectEquals(i + 1, earlyExitLast(i));
    }
    expectEquals(10, earlyExitLast(10));
    expectEquals(10, earlyExitLast(11));

    expectEquals(2, earlyExitNested());

    expectEquals(17, transferNarrowWrap());
    expectEquals(-45, polynomialShort());
    expectEquals(-45, polynomialIntFromLong());
    expectEquals(-45, polynomialInt());

    expectEquals(0, geoIntDivLastValue(0));
    expectEquals(0, geoIntDivLastValue(1));
    expectEquals(0, geoIntDivLastValue(2));
    expectEquals(0, geoIntDivLastValue(1081788608));
    expectEquals(0, geoIntDivLastValue(-1081788608));
    expectEquals(0, geoIntDivLastValue(2147483647));
    expectEquals(0, geoIntDivLastValue(-2147483648));

    expectEquals(          0, geoIntMulLastValue(0));
    expectEquals( -194211840, geoIntMulLastValue(1));
    expectEquals( -388423680, geoIntMulLastValue(2));
    expectEquals(-1041498112, geoIntMulLastValue(1081788608));
    expectEquals( 1041498112, geoIntMulLastValue(-1081788608));
    expectEquals(  194211840, geoIntMulLastValue(2147483647));
    expectEquals(          0, geoIntMulLastValue(-2147483648));

    expectEquals(0L, geoLongDivLastValue(0L));
    expectEquals(0L, geoLongDivLastValue(1L));
    expectEquals(0L, geoLongDivLastValue(2L));
    expectEquals(0L, geoLongDivLastValue(1081788608L));
    expectEquals(0L, geoLongDivLastValue(-1081788608L));
    expectEquals(0L, geoLongDivLastValue(2147483647L));
    expectEquals(0L, geoLongDivLastValue(-2147483648L));
    expectEquals(0L, geoLongDivLastValue(9223372036854775807L));
    expectEquals(0L, geoLongDivLastValue(-9223372036854775808L));

    expectEquals(                   0L, geoLongMulLastValue(0L));
    expectEquals(-8070450532247928832L, geoLongMulLastValue(1L));
    expectEquals( 2305843009213693952L, geoLongMulLastValue(2L));
    expectEquals(                   0L, geoLongMulLastValue(1081788608L));
    expectEquals(                   0L, geoLongMulLastValue(-1081788608L));
    expectEquals( 8070450532247928832L, geoLongMulLastValue(2147483647L));
    expectEquals(                   0L, geoLongMulLastValue(-2147483648L));
    expectEquals( 8070450532247928832L, geoLongMulLastValue(9223372036854775807L));
    expectEquals(                   0L, geoLongMulLastValue(-9223372036854775808L));

    System.out.println("passed");
  }

  private static void expectEquals(int expected, int result) {
    if (expected != result) {
      throw new Error("Expected: " + expected + ", found: " + result);
    }
  }

  private static void expectEquals(long expected, long result) {
    if (expected != result) {
      throw new Error("Expected: " + expected + ", found: " + result);
    }
  }
}
