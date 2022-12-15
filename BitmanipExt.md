# Bit-Manipulation ISA Extension for RISC-V

The bit-manipulation (bitmanip) extension consists of instructions that are intended for code size reduction, performance improvement, and energy reduction. The instructions are divided into several smaller bitmanip extensions – Zba, Zbb, Zbc, and Zbs extensions. Bitmanip instructions with suffixes .b, .h, and .w only look at the least significant 8-bits, 16-bits, and 32-bits of the input respectively and generate XLEN-wide results. The results are zero-extended or sign-extended based on the specific instruction.
The Zba instruction can be used to accelerate the generation of addresses that index into arrays of basic types using both unsigned word-sized and XLEN-sized indices. These instructions contain the shift and add instructions such as sh1add, sh1add.uw, and slli.uw. Zbb instructions are the most commonly used instructions and contain basic bit-manipulation instructions. Instructions in this group are logic with negate (andn, orn, xnor), count leading/trailing zero bits (clz, ctz), count population (cpop), integer minimum/maximum (max, min), and sign and zero extension (sext, zext). Bitwise rotation also comes under the Zbb group of instructions (rol, ror, orc, rev8). Zbc supports the carry-less multiplication operations (cmul, cmulh, cmulr). Zbs supports single-bit instructions to set, clear, invert or extract a single bit in a register (bclr, bext, binv, best). 
Further details about these instruction groups and a pseudo code for its implementation can be found on the RISCV A, B, C, and S extension document. 

 
## Extension Development for Rev Core

The Rev SST component supports the creation and addition of instructions to the ISA to allow the evaluations of performance with these instructions. Each instruction and its mnemonic is stored in an instruction table and can be addressed by a map that can be addressed using the encoded instruction. Within the Rev component for SST, the source code for the ISA is within the src folder. Within the scope of this special topic, four instructions from the bitmanip extension were added to the baseline Rev ISA. The detailed steps involved in the addition of these instructions are listed below. 

## Adding the new mnemonic to the feature handlers

The first step in the addition of a new instruction is to create a new feature mnemonic. This mnemonic can be parsed by the Rev infrastructure and recognized as a supported extension. RV_B was chosen as the extension mnemonic. This then needs to be added to the FeatureType enum present in RevFeature.h header file for use by the extension parser. 

```
    typedef enum{
      RV_UNKNOWN    = 0,        ///< RevFeatureType: unknown feature
      RV_I          = 1,        ///< RevFeatureType: I-extension
      RV_M          = 2,        ///< RevFeatureType: M-extension
      RV_A          = 3,        ///< RevFeatureType: A-extension
      RV_F          = 4,        ///< RevFeatureType: F-extension
      RV_D          = 5,        ///< RevFeatureType: D-extension
      RV_C          = 6,        ///< RevFeatureType: C-extension
      RV_P          = 20        ///< RevFeatureType: PAN Extension
      RV_P          = 20,       ///< RevFeatureType: PAN Extension
      RV_B          = 30        ///< RevFeatureType: Bit Manipulation Extension
    }RevFeatureType;
```

The extension parser is present in the RevFeature.cc file within the ParseMachineModel function. The function checks the machine model that is obtained from the SST simulation option and enables the required features. This is simply a switch-case entry and the new extension feature needs to be added to this. 
```
  // -- step 2: parse all the features
  while( found < arch.length() ){
    switch( arch[found] ){
    case 'I':
      SetMachineEntry(RV_I);
      break;
    case 'M':
      SetMachineEntry(RV_M);
      break;
    ...
    case 'P':
      SetMachineEntry(RV_P);
      break;
    case 'B':
      SetMachineEntry(RV_B);
      break;
    ...
```

## Creating the extension header

The next step is to create the extension header with the declaration of the extension and a definition of its instructions. This also includes details such as the opcodes for the instruction, its encoding table, mnemonics for the instructions, its memory cost, registers used, and the details of bits in the PC. This file resides inside src/insns/ and is usually given the name of the instruction mnemonic created above. In this project, the new header file is named RV64B.h. Care must be taken to ensure that the new instruction class, RV64B resides inside SST::RevCPU and inherits from the RevExt class. The RevExt class contains definitions for the instruction table and functions to get the instruction table. Within the class, the instructions are defined statically with return-type bool. The instructions take the RevFeature, the register files, the instance of memory, and the decoded state from the instruction as parameters. RevFeature object enables the instructions code to check the device architecture – 32 or 64-bit. 

To run a comprehensive code, a complete set of instructions are required, not just the bitmanip instruction. The instructions can be added one after the other with references from other machine models. An example of the andn instruction added as a part of the bitmanip extension is shown below. 
```
  static bool andn(RevFeature *F, RevRegFile *R,RevMem *M,RevInst Inst) {
    SEXT(R->RV64[Inst.rd], (R->RV64[Inst.rs1] & (~R->RV64[Inst.rs2])), 64);
    R->RV64_PC += Inst.instSize;
    R->cost += 5;
    return true;
  }
```

Once the instruction is defined, this needs to be added to the instruction table. The instruction table is implemented as a vector of instruction entries where each entry corresponds to a single instruction entry. The instruction table is typically named <feature mnemonic>Table, RV64BTable in this case. The instruction structure holds the mnemonic value, its cost of execution, the opcode, funct3, fucnt7 bits, the registers required , the instruction format, and the pointer to the implementation function. The example for andn instruction is shown below
```
std::vector<RevInstEntry > RV64BTable = {
{RevInstEntryBuilder<RevInstDefaults>().SetMnemonic("andn %rd, %rs1, %rs2" ).SetCost(1).SetOpcode( 0b0110011).SetFunct3(0b111).SetFunct7(0b0100000).SetrdClass(RegGPR).Setrs1Class(RegGPR).Setrs2Class(RegGPR    ).Setrs3Class(RegUNKNOWN).Setimm(FUnk).SetFormat(RVTypeR).SetImplFunc(&andn ).InstEntry},
};
```

This has the mnemonic defined to andn, an operating cost of 1 cycle. The opcode for this instruction is 0110011 with funct3 and funct7 bits set to 111 and 0100000 respectively. This is an R-type instruction and uses two source registers rs1 and rs2. The implementation function is pointing to andn, which was defined earlier in the class. 

## Adding the extension to the instruction table loader

Once the header file is implemented, this needs to be added to the RevInstTables.h header file. This is a common header file to the rest of the platform. Next, the new extension’s instructions need to be loaded into the internal Rev instruction table. Within the RevProc.cc file, the SeedInstTable function needs to include the new extension. We create a new object of the class and pass the feature pointer, a pointer to the register, the memory object, and the SST output object. The inclusion of the new RV64B extension is shown below. 
```
// Bit Manipulation Extension
if( feature->IsModeEnabled(RV_B) ){
EnableExt(static_cast<RevExt *>(new RV64B(feature,RegFile,mem,output)),false);
}
```

## Modifications to instruction decode

In this project, Rev was built with gcc/g++ 7 and presented an erroneous behaviour in the instruction decode phase. Presently, the method to check if an instruction is present in the instruction table is to find the instruction from the map using the encoded value as the key and compare if the value, Entry, is greater than the size of the instruction table. This is done using the following piece of code – 

`if( Entry > (InstTable.size()-1) )`

However, this condition does not return correctly as g++ returns 0 if the entry is not found. Therefore, the condition needs to be modified to the following for the correct execution of the code.

`if( Entry > (InstTable.size()-1) || it == EncToEntry.end() )`

## Evaluation
To evaluate the performance improvements with these instructions, a binary had to be generated with these instructions. Inline assembly code was written to build the binaries for the instruction. The inline assembly code for four instructions, andn, sh1add, bclr, and sw are shown below
```
  asm ( "li	a6,7;"
        "li	a4,10;"
        "andn	a5,a4,a6;"
        "sh1add	a5,a4,a6;"
        "bclr	a5,a4,a6;"
        "sw	a5,-20(s0);"); 
```

To build these with the RISC-V toolchain, the bitmanip extension must be provided as the architecture in the -march flag. For example, 
`riscv64-unknown-elf-gcc -march=rv64imafd_zbb -b sample.exe main.c`

will prepare the binaries with Zbb extensions enabled. This will allow the compilation of the code with instructions such as andn that come under the Zbb extension. Multiple extensions can be combined with underscores to enable building multiple instructions. That is, rv64imafd_zbb_zba allows the compilation of instructions with Zba and Zbb extensions. 
Functional Testing
It is desirable to check the functional correctness of instructions to ensure that the added instructions provide the right output. To achieve this, the value of the registers can be checked at the end of every execution cycle. In the RevExt.cc file, the Execute function can be modified with the following additions at the end.  
```
  output->verbose(CALL_INFO, 6, 0, "Register Values - \n");

  for(int i = 0; i < _REV_NUM_REGS_; i ++)
  {
    output->verbose(CALL_INFO, 6, 0,
                    "%d : %ld \n", i, regFile[threadID].RV64[i]);
  }
```

This prints the register values at the end of every cycle which can be compared against the register values from the inline assembly code. 
 
![rev](documentation/imgs/rev_regs1.png)
