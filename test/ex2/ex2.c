/*
 * ex2.c
 *
 * RISC-V ISA: RV64I
 *
 * Copyright (C) 2017-2020 Tactical Computing Laboratories, LLC
 * All Rights Reserved
 * contact@tactcomplabs.com
 *
 * See LICENSE in the top level directory for licensing details
 *
 */

#include <stdlib.h>

int main(int argc, char **argv){
  int i = 9;
  i = i + 25;
  i = ~i;

  // asm ( "lw	a5,-20(s0);"
  //       "mv	a4,a5;"
  //       "lw	a5,-36(s0);"
  //       "andn	a5,a4,a5;"
  //       "sw	a5,-20(s0);");

  asm ( "li	a6,7;"
        "li	a4,10;"
        // "andn	a5,a4,a6;"
        // "sh1add	a5,a4,a6;"
        "bclr	a5,a4,a6;"
        "sw	a5,-20(s0);");
  return i;
}
