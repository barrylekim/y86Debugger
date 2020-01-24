#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>

#include "instruction.h"
#include "printRoutines.h"

/* Reads one byte from memory, at the specified address. Stores the
   read value into *value. Returns 1 in case of success, or 0 in case
   of failure (e.g., if the address is beyond the limit of the memory
   size). */
int memReadByte(machine_state_t *state,	uint64_t address, uint8_t *value) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  if (address > state->programSize) {
    printf("memReadByte failed because address %"PRIx64" is larger than memory size\n", address);
    printf("print memory size %"PRIx64"\n", state->programSize);
    return 0;
  }
  uint8_t byteAtAddress = state->programMap[address];
  *value = byteAtAddress;
  return 1;
}

/* Reads one quad-word (64-bit number) from memory in little-endian
   format, at the specified starting address. Stores the read value
   into *value. Returns 1 in case of success, or 0 in case of failure
   (e.g., if the address is beyond the limit of the memory size). */
int memReadQuadLE(machine_state_t *state, uint64_t address, uint64_t *value) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  if (address + 7 > state -> programSize){
    printf("memReadQuadLE failed because address %"PRIx64" + 7 is larger than memory size\n", address);
    printf("print memory size %"PRIx64"\n", state->programSize);
    return 0;
  }
  uint64_t temp = 0x00000000000000;
      for (int i = 0; i<= 7; i++){
        uint64_t pcincr = address + i;
        uint64_t valBytes = state->programMap[pcincr];
        temp = temp | valBytes << (8*i);
      };
  *value = temp;
  return 1;
}

/* Stores the specified one-byte value into memory, at the specified
   address. Returns 1 in case of success, or 0 in case of failure
   (e.g., if the address is beyond the limit of the memory size). */
int memWriteByte(machine_state_t *state,  uint64_t address, uint8_t value) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  if (address > state->programSize){
    printf("memWriteByte failed because address %"PRIx64" is larger than memory size\n", address);
    printf("print memory size %"PRIx64"\n", state->programSize);
    return 0;
  }
  state->programMap[address] = value;
  return 1;
}

/* Stores the specified quad-word (64-bit) value into memory, at the
   specified start address, using little-endian format. Returns 1 in
   case of success, or 0 in case of failure (e.g., if the address is
   beyond the limit of the memory size). */
int memWriteQuadLE(machine_state_t *state, uint64_t address, uint64_t value) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  if (address + 7 >= state->programSize){
    printf("memWriteQuadLE failed because address %"PRIx64" is larger than memory size\n", address);
    printf("print memory size %"PRIx64"\n", state->programSize);
    return 0;
  }
  for (int i = 7; i>= 0; i--){
    uint8_t eachByte = (value >> (i*8)) & 0xFF;
    memWriteByte(state, address + i,  eachByte);
  }
  return 1;
}

/* Fetches one instruction from memory, at the address specified by
   the program counter. Does not modify the machine's state. The
   resulting instruction is stored in *instr. Returns 1 if the
   instruction is a valid non-halt instruction, or 0 (zero)
   otherwise. */
int fetchInstruction(machine_state_t *state, y86_instruction_t *instr) {
  // Look at the first byte and see what type of instruction that is. Depending on the instruction, we
  // know how big the instruction is and only take and update the program counter with the correct number
  // of bytes.
  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  uint64_t pc = state->programCounter;
  // uint64_t instrAtPc = state->programMap[pc];
  uint8_t instrAtPc;
  if (!memReadByte(state, pc, &instrAtPc)){
    // Address bigger than memory size
    return 0;
  };
  printf("Start of Fetch!!!!!!!!!!!!!!!!!!\n");
  printf("PC is now at %"PRIx64"\n", pc);
  printf("At that address the map points to  %"PRIx64"\n", instrAtPc);
  uint8_t icode = instrAtPc >> 4;
  uint8_t ifun = instrAtPc & 0x0F;
  switch (icode){
    case I_HALT: 
    {
      if (ifun != 0){
        printf("This is invalid");
        printErrorInvalidInstruction(stdout,instr);
        instr->icode = I_INVALID;
      }
      instr->icode = icode;
      instr->ifun = ifun;
      instr->location = pc;
      return 0;
      break;
    }
    case I_NOP: 
    case I_RET:
    {
      if (ifun != 0){
        printf("This is invalid");
        printErrorInvalidInstruction(stdout,instr);
        instr->icode = I_INVALID;
        return 0;
      }
      instr->icode = icode;
      instr->ifun = ifun;
      instr->valP = pc + 1;
      printf("next instruction address valP is %"PRIx64"\n", instr->valP);
      break;
    }
    case I_RRMVXX: 
    case I_OPQ:
    {
      if (ifun > 0x6 || ifun < 0x0){
        printf("This is invalid");
        printErrorInvalidInstruction(stdout,instr);
        instr->icode = I_INVALID;
        return 0;
      }
      instr->icode = icode;
      instr->ifun = ifun;
      uint64_t newpc = pc + 1;
      uint8_t instrAtNewPc;
      if (!memReadByte(state, newpc, &instrAtNewPc)){
        instr->icode= I_TOO_SHORT;
        printErrorShortInstruction(stdout, instr);
        return 0;
      };
      uint8_t rA= instrAtNewPc >> 4;
      uint8_t rB= instrAtNewPc & 0x0F;
      if (rA == 0xF || rB == 0xF){
        instr->icode= I_INVALID;
        printErrorInvalidInstruction(stdout, instr);
        return 0;
      }
      instr->rA = rA;
      instr->rB = rB;
      instr->valP = pc + 2;
      printf("next instruction address valP is %"PRIx64"\n", instr->valP);
      break;
    }
    case I_IRMOVQ:
    {
      uint64_t registerpc = pc + 1;
      uint8_t registerByte;
      if(!memReadByte(state, registerpc, &registerByte)){
        instr->icode= I_TOO_SHORT;
        printErrorShortInstruction(stdout, instr);
        return 0;
      };
      uint8_t nullrA = registerByte >> 4;
      if (ifun != 0 || nullrA != 0xF){
        printf("This is invalid");
        printErrorInvalidInstruction(stdout,instr);
        instr->icode = I_INVALID;
        return 0;
      }
      instr->icode = icode;
      instr->ifun = ifun;
      uint8_t rB= registerByte & 0x0F;
      if(rB == 0xF){
        instr->icode= I_INVALID;
        printErrorInvalidInstruction(stdout, instr);
        return 0;
      }
      uint64_t * value;
      value = &instr->valC;
      if(!memReadQuadLE(state, pc + 2, value)){
        instr->icode= I_TOO_SHORT;
        printErrorShortInstruction(stdout, instr);
        return 0;
      };
      instr->rB = rB;
      instr->valP=pc+10;
      printf("Final value of valC after puting it in instruction->valC  %"PRIx64"\n\n", instr->valC);
      printf("next instruction address valP is %"PRIx64"\n", instr->valP);
      break;
    }
    case I_RMMOVQ: 
    case I_MRMOVQ:
    {
      if (ifun != 0){
        printf("This is invalid");
        printErrorInvalidInstruction(stdout,instr);
        instr->icode = I_INVALID;
        return 0;
      }
      instr->icode = icode;
      instr->ifun = ifun;
      uint64_t registerpc = pc + 1;
      uint8_t registerByte;
      if(!memReadByte(state, registerpc, &registerByte)){
        instr->icode= I_TOO_SHORT;
        printErrorShortInstruction(stdout, instr);
        return 0;
      };
      uint8_t rA= registerByte >> 4;
      uint8_t rB= registerByte & 0x0F;
      if (rA == 0xF || rB == 0xF){
        instr->icode= I_INVALID;
        printErrorInvalidInstruction(stdout, instr);
        return 0;
      }
      instr->rA = rA;
      instr->rB = rB;
      uint64_t * value;
      value = &instr->valC;
      if(!memReadQuadLE(state, pc + 2, value)){
        instr->icode= I_TOO_SHORT;
        printErrorShortInstruction(stdout, instr);
        return 0;
      };
      instr->valP=pc+10;
      printf("Final value of valC after puting it in instruction->valC  %"PRIx64"\n\n", instr->valC);
      printf("next instruction address valP is %"PRIx64"\n", instr->valP);
      break;
    }
    case I_JXX:
    case I_CALL:
    {
      if ((icode == I_CALL && ifun != 0) || (icode == I_JXX && (ifun > 0x6 || ifun < 0x0))){ //Two invalid cases when icode = 8 and when icode = 7
        printf("This is invalid");
        printErrorInvalidInstruction(stdout,instr);
        instr->icode = I_INVALID;
        return 0;
      };
      instr->icode = icode;
      instr->ifun = ifun;
      uint64_t * value;
      value = &instr->valC;
      if(!memReadQuadLE(state, pc + 1, value)){
        instr->icode= I_TOO_SHORT;
        printErrorShortInstruction(stdout, instr);
        return 0;
      };
      instr->valP=pc+9;
      printf("Final value of valC after puting it in instruction->valC  %"PRIx64"\n\n", instr->valC);
      printf("next instruction address valP is %"PRIx64"\n", instr->valP);
      break;
    }
    case I_PUSHQ:
    case I_POPQ:
    {
      uint64_t registerpc = pc + 1;
      uint8_t registerByte; 
      if(!memReadByte(state, registerpc, &registerByte)){
        instr->icode= I_TOO_SHORT;
        printErrorShortInstruction(stdout, instr);
        return 0;
      };
      uint8_t nullrB = registerByte & 0x0F;
      uint8_t rA = registerByte >> 4;
      if (ifun != 0 || nullrB != 0xF || rA == 0xF){
        printf("This is invalid");
        printErrorInvalidInstruction(stdout,instr);
        instr->icode = I_INVALID;
        return 0;
      }
      instr->icode = icode;
      instr->ifun = ifun;
      instr->rA= rA;
      instr->valP=pc+2;
      printf("next instruction address valP is %"PRIx64"\n", instr->valP);
      break;
    }
    default: 
      printErrorInvalidInstruction(stdout,instr);
      instr->icode = I_INVALID;
      return 0;
      break;
  }
  instr->location = pc;
  return 1;
  
}

/* Executes the instruction specified by *instr, modifying the
   machine's state (memory, registers, condition codes, program
   counter) in the process. Returns 1 if the instruction was executed
   successfully, or 0 if there was an error. Typical errors include an
   invalid instruction or a memory access to an invalid address. */
int executeInstruction(machine_state_t *state, y86_instruction_t *instr) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  // Need to test for invalid instructions or memory access to invalid address
  printf("Start of execution!!!!!!!!!!!!!!!!!!!!!!!\n");
  switch (instr->icode){
    case I_HALT:
    {
      // What to do with halt?
      return 1;
      break;
    }
    case I_NOP:
    {
      state->programCounter = instr->valP;
      break;
    }
    case I_RRMVXX:
    case I_JXX:
    {
      int Cnd = 0;
      uint8_t conditioncode = state->conditionCodes & 0x0F; //Condition code is now SF/ZF
      switch (instr->ifun){
        case C_NC:
        {
          Cnd = 1;
          break;
        }
        case C_LE:
        {
          if((conditioncode == 0x2) || (conditioncode == 0x1)){ //10 or 01 because 11 will never happen (can raise sign and zero flag at the same time)
            Cnd = 1;
          }
          break;
        }
        case C_L:
        {
          if((conditioncode == 0x3) || (conditioncode == 0x2)){ //11 or 10
            Cnd = 1;
          }
          break;
        }
        case C_E:
        {
          if((conditioncode == 0x1) || (conditioncode == 0x3)){ //01 or 11
            Cnd = 1;
          }
          break;
        }
        case C_NE:
        {
          if((conditioncode == 0x0) || (conditioncode == 0x2)){ //00 or 10
            Cnd = 1;
          }
          break;
        }
        case C_GE:
        {
          if((conditioncode == 0x0) || (conditioncode == 0x1)){ //00 or 01
            Cnd = 1;
          }
          break;
        }
        case C_G:
        {
          if(conditioncode == 0x0){ //00
            Cnd = 1;
          }
          break;
        }
        default:
          printf("Switching through cases in jump should not lead to this line. Ifun is wrong");
          printErrorInvalidInstruction(stdout,instr);
          instr->icode = I_INVALID;
          return 0;
          break;
      }
      if (instr->icode == I_JXX){
        if (Cnd){
          state->programCounter = instr->valC; //Let program counter jump to new address
        } else {
          state->programCounter = instr->valP; //Let program counter continue to next instruction
        }
      } else if (instr->icode == I_RRMVXX){
        if (Cnd){
          uint64_t valA = state->registerFile[instr->rA]; // valA is the content of register rA 
          uint64_t valE = valA + 0;
          state->registerFile[instr->rB] = valE;
        }
        state->programCounter = instr->valP;
      }
      break;
    }
    case I_IRMOVQ:
    {
      uint64_t valE = instr->valC + 0;
      state->registerFile[instr->rB] = valE; // put number valE into the contents of register rB
      state->programCounter = instr->valP; // update program counter
      break;
    }
    case I_RMMOVQ:
    {
      uint64_t valA = state->registerFile[instr->rA]; // valA is the content of register rA in this case is what we want to put in memory
      uint64_t valB = state->registerFile[instr->rB]; // valB is the content of register rB in this case is an address in memory
      uint64_t valE = valB + instr->valC; // valE is the address in memory to put valA in. valC is the offset from valB
      if(!memWriteQuadLE(state, valE, valA)){
        printErrorInvalidMemoryLocation(stdout, instr, valE);
        return 0;
      };
      state->programCounter = instr->valP;
      break;
    }
    case I_MRMOVQ:
    {
      uint64_t valB = state->registerFile[instr->rB]; // valB is the content of register rB in this case is an address in memory
      uint64_t valE = valB + instr->valC; // valE is the address in memory to read from and put into rA. valC is the offset from valB
      // state->registerFile[instr->rA] = state->programMap[valE];
      uint64_t tempVal;
      if(!memReadQuadLE(state, valE, &tempVal)){
        printErrorInvalidMemoryLocation(stdout, instr, valE);
        return 0;
      };
      state->registerFile[instr->rA] = tempVal;
      state->programCounter = instr->valP;
      break;
    }
    case I_OPQ:
    {
      int64_t valE;
      uint64_t valA = state->registerFile[instr->rA]; // valA is the content of register rA 
      uint64_t valB = state->registerFile[instr->rB]; // valB is the content of register rB 
      if (instr->ifun == A_ADDQ){
        valE = valA + valB;
      }
      if (instr->ifun == A_SUBQ){
        valE = valB - valA;
      }
      if (instr->ifun == A_ANDQ){
        valE = valA & valB;
      }
      if (instr->ifun == A_XORQ){
        valE = valA ^ valB;
      }
      if (instr->ifun == A_MULQ){
        valE = valA * valB;
      }
      if (instr->ifun == A_DIVQ){
        valE = valB/valA;
      }
      if (instr->ifun == A_MODQ){
        valE = valB % valA;
      }

      // Setting condition codes
      if (valE == 0){
        state->conditionCodes = state->conditionCodes | CC_ZERO_MASK;
      } else {
        state->conditionCodes = state->conditionCodes & 0xFE;
      }

      if (valE < 0){
        state->conditionCodes = state->conditionCodes | CC_SIGN_MASK;
      } else {
        state->conditionCodes = state->conditionCodes & 0xFD;
      }
      
      state->registerFile[instr->rB] = valE;
      state->programCounter = instr->valP;
      break;
    }
    case I_CALL:
    {
      uint64_t valRsp = state->registerFile[4]; //Address that the stack pointer is pointing at 
      uint64_t valE = valRsp - 8;
      if(!memWriteQuadLE(state, valE, instr->valP)){
        printErrorInvalidMemoryLocation(stdout, instr, valE);
        return 0;
      }; //Pushes the address of the next instruction after call onto the stack so we have sth to return to
      state->registerFile[4] = valRsp - 8; //Moving the stack pointer down because there is something new on the stack
      state->programCounter = instr->valC; // Program counter moves to the next instruction specified at the input address
      break;
    }
    case I_RET:
    {
      uint64_t valRsp = state->registerFile[4]; //Decode
      uint64_t tempVal;
      if(!memReadQuadLE(state, valRsp, &tempVal)){
        printErrorInvalidMemoryLocation(stdout, instr, valRsp);
        return 0;
      };
      state->programCounter = tempVal; //Update program counter, moving it back to the instruction after the call
      state->registerFile[4] = valRsp + 8; 
      break;
    }
    case I_PUSHQ:
    {
      /*
       (*state).registerFile[4] = (*state).registerFile[4] - 8;
      if (memWriteQuadLE(state, (*state).registerFile[4], (*state).registerFile[instr->rA]) == 0) {
        printErrorInvalidMemoryLocation(stdout, instr, (*state).registerFile[4]);
        state->programCounter = instr->valP;
        return 0;
      } else {
        state->programCounter = instr->valP;
        return 1;
      }
      */
      uint64_t valA = state->registerFile[instr->rA]; //Content of register rA to be pushed on the stack
      uint64_t valRsp = state->registerFile[4]; //Address that the stack pointer is pointing at
      if(!memWriteQuadLE(state, valRsp - 8, valA)){
        printErrorInvalidMemoryLocation(stdout, instr, valRsp - 8);
        return 0;
      };
      state->registerFile[4] = valRsp - 8; //Updating the stack pointer
      state->programCounter = instr->valP;
      break;
    }
    case I_POPQ:
    {
      uint64_t valRsp = state->registerFile[4]; //Address that the stack pointer is pointing at
      uint64_t tempVal;
      if(!memReadQuadLE(state, valRsp, &tempVal)){
        printErrorInvalidMemoryLocation(stdout, instr, valRsp);
        return 0;
      }; //Putting the value read at the stack pointer back into rA
      state->registerFile[instr->rA] = tempVal; //Putting the value read at the stack pointer back into rA
      state->registerFile[4] = valRsp + 8; //Updating stack pointer
      state->programCounter = instr->valP;
      break;
    }
    default:
      printf("Invalid icode!!!");
      instr->icode = I_INVALID;
      printErrorInvalidInstruction(stdout, instr);
      return 0;
      break;
  }
  return 1;
}