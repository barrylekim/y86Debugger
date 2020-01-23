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
    return 0;
  }
  printf("value of address memReadByte %"PRIx64"\n\n",address);
  printf("value of memory size %"PRIx64"\n\n",state->programSize);
  uint8_t byteAtAddress = state->programMap[address];
  printf("value of byteAtAddress memReadByte %"PRIx64"\n\n",byteAtAddress);
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
    return 0;
  }
  uint64_t temp = 0x00000000000000;
      for (int i = 0; i<= 7; i++){
        uint64_t pcincr = address + i;
        uint64_t valBytes = state->programMap[pcincr];
        temp = temp | valBytes << (8*i);
        //printf("valBytes value  %"PRIx64"\n\n", valBytes);
        printf("Little endian value after each concatenation %"PRIx64"\n\n", temp);
      };
  *value = temp;
  return 1;
}

/* Stores the specified one-byte value into memory, at the specified
   address. Returns 1 in case of success, or 0 in case of failure
   (e.g., if the address is beyond the limit of the memory size). */
int memWriteByte(machine_state_t *state,  uint64_t address, uint8_t value) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  return 0;
}

/* Stores the specified quad-word (64-bit) value into memory, at the
   specified start address, using little-endian format. Returns 1 in
   case of success, or 0 in case of failure (e.g., if the address is
   beyond the limit of the memory size). */
int memWriteQuadLE(machine_state_t *state, uint64_t address, uint64_t value) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  return 0;
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
  printf("PC is now at %"PRIx64"\n", pc);
  printf("At that address the map points to  %"PRIx64"\n", instrAtPc);
  uint8_t icode = instrAtPc >> 4;
  uint8_t ifun = instrAtPc & 0x0F;
  printf("icode value %"PRIx64"\n", icode);
  printf("ifun value %"PRIx64"\n", ifun);
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
      printf("rA value %"PRIx64"\n", instr->rA);
      printf("rB value  %"PRIx64"\n", instr->rB);
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
      printf("rA value %"PRIx64"\n", instr->rA);
      printf("rB value  %"PRIx64"\n", instr->rB);
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
      printf("rA value  %"PRIx64"\n", instr->rA);
      printf("next instruction address valP is %"PRIx64"\n", instr->valP);
      break;
    }
    default: 
      printf("This shouldn't be here");
      printErrorInvalidInstruction(stdout,instr);
      instr->icode = I_INVALID;
      return 0;
      break;
  }
  return 1;
  
}

/* Executes the instruction specified by *instr, modifying the
   machine's state (memory, registers, condition codes, program
   counter) in the process. Returns 1 if the instruction was executed
   successfully, or 0 if there was an error. Typical errors include an
   invalid instruction or a memory access to an invalid address. */
int executeInstruction(machine_state_t *state, y86_instruction_t *instr) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  switch (instr->icode){
    case I_HALT:
    {
      // What to do with halt?
      // What does it mean if address is beyond the limit of memory size for memReadQuadLE() helper function up there?
      // Where is instr->valP updated??
      break;
    }
    case I_NOP:
    {
      break;
    }
    case I_RRMVXX:
    {
      break;
    }
    case I_IRMOVQ:
    {
      break;
    }
    case I_RMMOVQ:
    {
      break;
    }
    case I_MRMOVQ:
    {
      break;
    }
    case I_OPQ:
    {
      break;
    }
    case I_JXX:
    {
      break;
    }
    case I_CALL:
    {
      break;
    }
    case I_RET:
    {
      break;
    }
    case I_PUSHQ:
    {
      break;
    }
    case I_POPQ:
    {
      break;
    }
    default:
      break;
  }

  return 0;
}