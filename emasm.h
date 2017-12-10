/**
* Copyright (c) 2017
* Mikhail Baynov m.baynov@gmail.com
* Distributed under the GNU GPL v2
*/
#ifndef EMASM_H
#define EMASM_H
#ifdef __cplusplus
extern "C" {
#endif


char* emasm_compile(const char* in);
char* emasm_disasm(const char* in);

enum OPCODES {
   STOP = 0x00,		///< halts execution
   ADD,				///< addition operation
   MUL,				///< mulitplication operation
   SUB,				///< subtraction operation
   DIV,				///< integer division operation
   SDIV,				///< signed integer division operation
   MOD,				///< modulo remainder operation
   SMOD,				///< signed modulo remainder operation
   ADDMOD,				///< unsigned modular addition
   MULMOD,				///< unsigned modular multiplication
   EXP,				///< exponential operation
   SIGNEXTEND,			///< extend length of signed integer
   LT = 0x10,			///< less-than comparision
   GT,					///< greater-than comparision
   SLT,				///< signed less-than comparision
   SGT,				///< signed greater-than comparision
   EQ,					///< equality comparision
   ISZERO,				///< simple not operator
   AND,				///< bitwise AND operation
   OR,					///< bitwise OR operation
   XOR,				///< bitwise XOR operation
   NOT,				///< bitwise NOT opertation
   BYTE,				///< retrieve single byte from word
   SHA3 = 0x20,		///< compute SHA3-256 hash
   ADDRESS = 0x30,		///< get address of currently executing account
   BALANCE,			///< get balance of the given account
   ORIGIN,				///< get execution origination address
   CALLER,				///< get caller address
   CALLVALUE,			///< get deposited value by the instruction/transaction responsible for this execution
   CALLDATALOAD,		///< get input data of current environment
   CALLDATASIZE,		///< get size of input data in current environment
   CALLDATACOPY,		///< copy input data in current environment to memory
   CODESIZE,			///< get size of code running in current environment
   CODECOPY,			///< copy code running in current environment to memory
   GASPRICE,			///< get price of gas in current environment
   EXTCODESIZE,		///< get external code size (from another contract)
   EXTCODECOPY,		///< copy external code (from another contract)
   BLOCKHASH = 0x40,	///< get hash of most recent complete block
   COINBASE,			///< get the block's coinbase address
   TIMESTAMP,			///< get the block's timestamp
   NUMBER,				///< get the block's number
   DIFFICULTY,			///< get the block's difficulty
   GASLIMIT,			///< get the block's gas limit
   POP = 0x50,			///< remove item from stack
   MLOAD,				///< load word from memory
   MSTORE,				///< save word to memory
   MSTORE8,			///< save byte to memory
   SLOAD,				///< load word from storage
   SSTORE,				///< save word to storage
   JUMP,				///< alter the program counter
   JUMPI,				///< conditionally alter the program counter
   PC,					///< get the program counter
   MSIZE,				///< get the size of active memory
   GAS,				///< get the amount of available gas
   JUMPDEST,			///< 0x5b set a potential jump destination
   PUSH1 = 0x60,		///< place 1 byte item on stack
   PUSH2,				///< place 2 byte item on stack
   PUSH3,				///< place 3 byte item on stack
   PUSH4,				///< place 4 byte item on stack
   PUSH5,				///< place 5 byte item on stack
   PUSH6,				///< place 6 byte item on stack
   PUSH7,				///< place 7 byte item on stack
   PUSH8,				///< place 8 byte item on stack
   PUSH9,				///< place 9 byte item on stack
   PUSH10,				///< place 10 byte item on stack
   PUSH11,				///< place 11 byte item on stack
   PUSH12,				///< place 12 byte item on stack
   PUSH13,				///< place 13 byte item on stack
   PUSH14,				///< place 14 byte item on stack
   PUSH15,				///< place 15 byte item on stack
   PUSH16,				///< place 16 byte item on stack
   PUSH17,				///< place 17 byte item on stack
   PUSH18,				///< place 18 byte item on stack
   PUSH19,				///< place 19 byte item on stack
   PUSH20,				///< place 20 byte item on stack
   PUSH21,				///< place 21 byte item on stack
   PUSH22,				///< place 22 byte item on stack
   PUSH23,				///< place 23 byte item on stack
   PUSH24,				///< place 24 byte item on stack
   PUSH25,				///< place 25 byte item on stack
   PUSH26,				///< place 26 byte item on stack
   PUSH27,				///< place 27 byte item on stack
   PUSH28,				///< place 28 byte item on stack
   PUSH29,				///< place 29 byte item on stack
   PUSH30,				///< place 30 byte item on stack
   PUSH31,				///< place 31 byte item on stack
   PUSH32,				///< place 32 byte item on stack
   DUP1 = 0x80,		///< copies the highest item in the stack to the top of the stack
   DUP2,				///< copies the second highest item in the stack to the top of the stack
   DUP3,				///< copies the third highest item in the stack to the top of the stack
   DUP4,				///< copies the 4th highest item in the stack to the top of the stack
   DUP5,				///< copies the 5th highest item in the stack to the top of the stack
   DUP6,				///< copies the 6th highest item in the stack to the top of the stack
   DUP7,				///< copies the 7th highest item in the stack to the top of the stack
   DUP8,				///< copies the 8th highest item in the stack to the top of the stack
   DUP9,				///< copies the 9th highest item in the stack to the top of the stack
   DUP10,				///< copies the 10th highest item in the stack to the top of the stack
   DUP11,				///< copies the 11th highest item in the stack to the top of the stack
   DUP12,				///< copies the 12th highest item in the stack to the top of the stack
   DUP13,				///< copies the 13th highest item in the stack to the top of the stack
   DUP14,				///< copies the 14th highest item in the stack to the top of the stack
   DUP15,				///< copies the 15th highest item in the stack to the top of the stack
   DUP16,				///< copies the 16th highest item in the stack to the top of the stack
   SWAP1 = 0x90,		///< swaps the highest and second highest value on the stack
   SWAP2,				///< swaps the highest and third highest value on the stack
   SWAP3,				///< swaps the highest and 4th highest value on the stack
   SWAP4,				///< swaps the highest and 5th highest value on the stack
   SWAP5,				///< swaps the highest and 6th highest value on the stack
   SWAP6,				///< swaps the highest and 7th highest value on the stack
   SWAP7,				///< swaps the highest and 8th highest value on the stack
   SWAP8,				///< swaps the highest and 9th highest value on the stack
   SWAP9,				///< swaps the highest and 10th highest value on the stack
   SWAP10,				///< swaps the highest and 11th highest value on the stack
   SWAP11,				///< swaps the highest and 12th highest value on the stack
   SWAP12,				///< swaps the highest and 13th highest value on the stack
   SWAP13,				///< swaps the highest and 14th highest value on the stack
   SWAP14,				///< swaps the highest and 15th highest value on the stack
   SWAP15,				///< swaps the highest and 16th highest value on the stack
   SWAP16,				///< swaps the highest and 17th highest value on the stack
   LOG0 = 0xa0,		///< Makes a log entry; no topics.
   LOG1,				///< Makes a log entry; 1 topic.
   LOG2,				///< Makes a log entry; 2 topics.
   LOG3,				///< Makes a log entry; 3 topics.
   LOG4,				///< Makes a log entry; 4 topics.
   SLOADBYTES = 0xe1,  ///    0xe1: ['SLOADBYTES', 3, 0, 50], # to be discontinued
   SSTOREBYTES = 0xe2, ///    0xe2: ['SSTOREBYTES', 3, 0, 0], # to be discontinued
   SSIZE = 0xe3,       ///    0xe3: ['SSIZE', 1, 1, 50], # to be discontinued
   CREATE = 0xf0,		///< create a new account with associated code
   CALL,				///< message-call into an account
   CALLCODE,			///< message-call with another account's code only
   RETURN,				///< halt execution returning output data
   DELEGATECALL,  ///   0xf4: ['DELEGATECALL', 6, 1, 40], # 700 now
   CALLBLACKBOX, ///   0xf5: ['CALLBLACKBOX', 7, 1, 40],
   STATICCALL = 0xfa,   ///   0xfa: ['STATICCALL', 6, 1, 40],
   INVALID = 0xfe,   ///< 'Invalid opcode' This is about documenting ethereum/EIPs#141
   REVERT = 0xfd,    ///< new opcode, revert changes>
   SUICIDE = 0xff		///< halt execution and register account for later deletion
};


char* opcode_from_byte(const uint8_t c) {
   char *out = calloc(1, 33);
   sprintf(out, "  %02x??", c);  //default
   switch (c) {
      case STOP: return "STOP"; break;
      case ADD: return "ADD"; break;
      case MUL: return "MUL"; break;
      case SUB: return "SUB"; break;
      case DIV: return "DIV"; break;
      case SDIV: return "SDIV"; break;
      case MOD: return "MOD"; break;
      case SMOD: return "SMOD"; break;
      case ADDMOD: return "ADDMOD"; break;
      case MULMOD: return "MULMOD"; break;
      case EXP: return "EXP"; break;
      case SIGNEXTEND: return "SIGNEXTEND"; break;
      case LT: return "LT"; break;
      case GT: return "GT"; break;
      case SLT: return "SLT"; break;
      case EQ: return "EQ"; break;
      case ISZERO: return "ISZERO"; break;
      case AND: return "AND"; break;
      case OR: return "OR"; break;
      case XOR: return "XOR"; break;
      case NOT: return "NOT"; break;
      case BYTE: return "BYTE"; break;
      case SHA3: return "SHA3"; break;
      case ADDRESS: return "ADDRESS"; break;
      case BALANCE: return "BALANCE"; break;
      case ORIGIN: return "ORIGIN"; break;
      case CALLER: return "CALLER"; break;
      case CALLVALUE: return "CALLVALUE"; break;
      case CALLDATALOAD: return "CALLDATALOAD"; break;
      case CALLDATASIZE: return "CALLDATASIZE"; break;
      case CALLDATACOPY: return "CALLDATACOPY"; break;
      case CODESIZE: return "CODESIZE"; break;
      case CODECOPY: return "CODECOPY"; break;
      case GASPRICE: return "GASPRICE"; break;
      case EXTCODESIZE: return "EXTCODESIZE"; break;
      case EXTCODECOPY: return "EXTCODECOPY"; break;
      case BLOCKHASH: return "BLOCKHASH"; break;
      case COINBASE: return "COINBASE"; break;
      case TIMESTAMP: return "TIMESTAMP"; break;
      case NUMBER: return "NUMBER"; break;
      case DIFFICULTY: return "DIFFICULTY"; break;
      case GASLIMIT: return "GASLIMIT"; break;
      case POP: return "POP"; break;
      case MLOAD: return "MLOAD"; break;
      case MSTORE: return "MSTORE"; break;
      case MSTORE8: return "MSTORE8"; break;
      case SLOAD: return "SLOAD"; break;
      case SSTORE: return "SSTORE"; break;
      case JUMP: return "JUMP"; break;
      case JUMPI: return "JUMPI"; break;
      case PC: return "PC"; break;
      case MSIZE: return "MSIZE"; break;
      case GAS: return "GAS"; break;
      case JUMPDEST: return "JUMPDEST"; break;
      case PUSH1: return "PUSH"; break;
      case PUSH2: return "PUSH"; break;
      case PUSH3: return "PUSH"; break;
      case PUSH4: return "PUSH"; break;
      case PUSH5: return "PUSH"; break;
      case PUSH6: return "PUSH"; break;
      case PUSH7: return "PUSH"; break;
      case PUSH8: return "PUSH"; break;
      case PUSH9: return "PUSH"; break;
      case PUSH10: return "PUSH"; break;
      case PUSH11: return "PUSH"; break;
      case PUSH12: return "PUSH"; break;
      case PUSH13: return "PUSH"; break;
      case PUSH14: return "PUSH"; break;
      case PUSH15: return "PUSH"; break;
      case PUSH16: return "PUSH"; break;
      case PUSH17: return "PUSH"; break;
      case PUSH18: return "PUSH"; break;
      case PUSH19: return "PUSH"; break;
      case PUSH20: return "PUSH"; break;
      case PUSH21: return "PUSH"; break;
      case PUSH22: return "PUSH"; break;
      case PUSH23: return "PUSH"; break;
      case PUSH24: return "PUSH"; break;
      case PUSH25: return "PUSH"; break;
      case PUSH26: return "PUSH"; break;
      case PUSH27: return "PUSH"; break;
      case PUSH28: return "PUSH"; break;
      case PUSH29: return "PUSH"; break;
      case PUSH30: return "PUSH"; break;
      case PUSH31: return "PUSH"; break;
      case PUSH32: return "PUSH"; break;
      case DUP1: return "DUP1"; break;
      case DUP2: return "DUP2"; break;
      case DUP3: return "DUP3"; break;
      case DUP4: return "DUP4"; break;
      case DUP5: return "DUP5"; break;
      case DUP6: return "DUP6"; break;
      case DUP7: return "DUP7"; break;
      case DUP8: return "DUP8"; break;
      case DUP9: return "DUP9"; break;
      case DUP10: return "DUP10"; break;
      case DUP11: return "DUP11"; break;
      case DUP12: return "DUP12"; break;
      case DUP13: return "DUP13"; break;
      case DUP14: return "DUP14"; break;
      case DUP15: return "DUP15"; break;
      case DUP16: return "DUP16"; break;
      case SWAP1: return "SWAP1"; break;
      case SWAP2: return "SWAP2"; break;
      case SWAP3: return "SWAP3"; break;
      case SWAP4: return "SWAP4"; break;
      case SWAP5: return "SWAP5"; break;
      case SWAP6: return "SWAP6"; break;
      case SWAP7: return "SWAP7"; break;
      case SWAP8: return "SWAP8"; break;
      case SWAP9: return "SWAP9"; break;
      case SWAP10: return "SWAP10"; break;
      case SWAP11: return "SWAP11"; break;
      case SWAP12: return "SWAP12"; break;
      case SWAP13: return "SWAP13"; break;
      case SWAP14: return "SWAP14"; break;
      case SWAP15: return "SWAP15"; break;
      case SWAP16: return "SWAP16"; break;
      case LOG0: return "LOG0"; break;
      case LOG1: return "LOG1"; break;
      case LOG2: return "LOG2"; break;
      case LOG3: return "LOG3"; break;
      case LOG4: return "LOG4"; break;
      case SLOADBYTES: return "SLOADBYTES"; break;
      case SSTOREBYTES: return "SSTOREBYTES"; break;
      case SSIZE: return "SSIZE"; break;
      case CREATE: return "CREATE"; break;
      case CALL: return "CALL"; break;
      case CALLCODE: return "CALLCODE"; break;
      case RETURN: return "RETURN"; break;
      case DELEGATECALL: return "DELEGATECALL"; break;
      case CALLBLACKBOX: return "CALLBLACKBOX"; break;
      case STATICCALL: return "STATICCALL"; break;
      case INVALID: return "INVALID"; break;
      case REVERT: return "REVERT"; break;
      case SUICIDE: return "SUICIDE"; break;
      default:
         break;
   }
   return out;
}


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
#endif
