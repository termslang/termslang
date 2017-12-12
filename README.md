# TERMS language specification


## Philisophy behind [#termslang](https://twitter.com/hashtag/termslang)
The idea of TERMS language is to provide opportunity to write EVM code in a human readable language, subset of [legal English](https://en.wikipedia.org/wiki/Legal_English). It does not try to follow a particular programming concept or paradigm and compiles directly into EVM instructions.


## Using terms compiler
### Compile and run terms compiler
Terms compiler source code is written in C99 and can be compiled on Linux systems using GCC with no dependencies.
```
gcc terms.c -Wno-incompatible-pointer-types-discards-qualifiers -std=c99 -o terms
```

Compiler executable file takes filename as argument. Terms language files are expected to have .tt extension.
For example:
```
./terms examples/token.tt
```
This would compile token.tt and produce a range of output files:

| filename | purpose  |
| :---:        | - |
| **token.js**  |  to deploy a contract using geth, just copy and paste contains of this file into geth console |
| **token.asm** |  raw assembly version of a contract. Can be useful for low-level code debugging |
| **token.abi** |  contract abi |
| **token.hex** |  contract bytecode |


A project in terms language can only contain one .tt file. Optionally, it can contain .ttp file that is concatenated to the end of .tt file during compiler preprocessor stage. Normally, .ttp file is used for terms language "procedures".


## Contract structure
TERMS contract source code consists of a single .tt file and, optionally, a .ttp file that is concatenated to the end of .tt file during compiler preprocessor stage. Normally, .ttp file is used for terms language "procedures".

Contract constructor section begins with word "Contract", optionally followed by the contract's name in double quotes. The name doesn't affect the bytecode in any way (unlike i.e. in Solidity).

Contract main section begins with word "Conditions" followed by separator. Separators in TERMS divide sentences of the contract.
Separators are the following: dot(.), colon(:), semicolon(;). All separators are equal and have to be used according to context.
All the following sequences are equal:
```
If not REVENUE CONSTANT, stop.
Increment BALANCE by REVENUE CONSTANT.
```

```
If not REVENUE CONSTANT, stop. Increment BALANCE by REVENUE CONSTANT.
```

```
If not REVENUE CONSTANT,
stop;
Increment BALANCE
by REVENUE CONSTANT.
```

Comma(,) is not a separator and can be used in macros as a word.

The point of entry of a TERMS program is the next sentence after the "Conditions:" sentence. In contrast with Solidity, TERMS invents the concept of "fallback dispatcher". It is the piece of code that is silently inserted before the point of entry. Its purpose is to detect method signature coming with the call and dispatch the call directly to the right method. If no known method signature detected, fallback function is executed. This is how this part of the code may look like:
```
Conditions:
Payable fallback:
Return.

transfer(address to, uint256 value);
```
There are plans to make the "payable fallback" section optional, but the rule is that it may only appear right after "Conditions:" sentence. There is currently no way in TERMS compiler to make a method payable unless it is the fallback method. This restriction fits most contracts. One can change abi manually to make a method payable.


## TERMS LANGUAGE PRIMITIVES
### Variables
Variable is one or more words starting with capital letter or underscore symbol. The rest of letters are converted to uppercase on preprocessor stage. For instance, BALANCE and Balance are the same variable. USER BALANCE and User Balance are the same, variable, too, but one that is different from BALANCE. It is up to contract writer to decide whether to use all-uppercase variables or not.

### Ethereum environment constants
There are some variables that are predefined by Ethereum environment. The all end with CONSTANT word, are directly compiled into EVM commands according to rules set in file [terms.develop.txt]:
```
>> CALLER CALLER
>> REVENUE CALLVALUE
>> STACKCOPY DUP1
>> STACK
>> RECORD SLOAD
>> BLOCKNUMBER NUMBER
>> CONTRACT ADDRESS
>> BALANCE BALANCE
>> ORIGIN ORIGIN
>> CALLDATASIZE CALLDATASIZE
>> GASPRICE GASPRICE
>> COINBASE COINBASE
>> TIMESTAMP TIMESTAMP
>> DIFFICULTY DIFFICULTY
>> GASLIMIT GASLIMIT
>> PC PC
>> MSIZE MSIZE
>> GAS GAS
>> GASLIMIT GASLIMIT
```
This means that to get message caller (CALLER opcode), we need to use CALLER CONSTANT variable. To get gas value of a call, we call GAS CONSTANT. To get current block number, we use BLOCKNUMBER CONSTANT (which is translated to opcode NUMBER).


#### Variable initialisation and scope
Variables are implicitly initialised in TERMS language. All variables are initialised with 0. That makes a shorter code but very error prone in hands of inaccurate developer with a wrong IDE. To avoid errors, we recommend using an IDE with autocomplete option and double check spelling of every variable.

TERMS language is dynamically typed. The only way to know the type of a variable is to infer it.

TERMS language does not use negative numbers. Most legal documents do not contain negatives, i.e. one cannot be sent -100 ether.


### Array members
One or more words followed by "#" and a decimal number is an array member. Those all are valid array members:
```
USER #1
USER#1
PARTY MEMBER #1
```
Convention in TERMS language is that count starts from #1.

### Sequences
Sequences, such as strings, are subset of arrays. This special kind of array has the following contents:
STRING PIECE #0   - contains length of STRING PIECE SEQUENCE
STRING PIECE #1   - contains first 32 bytes of the Unicode-encoded string.
STRING PIECE #2   - more 32 bytes
...

Note:  STRING PIECE SEQUENCE is an alias to STRING PIECE #0

### Events
Events are declared in constructor part of the code and follow the syntax rules of Solidity
```
event Transfer(address indexed _from, address indexed _to, uint256 _value);
```

Events are called by a set of log macros. Full set of all macros can be found in
[terms.develop.txt](https://github.com/termslang/termslang/blob/master/terms.develop.txt)
```
Log TRANSFER EVENT with topics FROM, TO, data VALUE.
```

## Methods
Example of a method name is:
```
constant balanceOf(address owner);
```
Here, variable OWNER is passed to the method. The name of the variable OWNER is inferred from balanceOf(address owner) method name by capitalising the parameter's name.

A method name must take exactly one line. Method body starts from the next line. Every method has one and only one return statement.

This is what identity method would look like:
```
constant identity(uint256 input);
Return unit256 at INPUT.
```

NOTE: The "constant" modifier is to be applied to methods that don't change the state. It is brought from Solidity and will likely be removed in later versions.


## Clauses
Clauses are numbers optionally divided by dots, used exactly like in legal contracts. The depth of dots is unlimited, i.e. clause 1.1.1.1.1.1.1.1.1.1.1.2.100.2.4  is totally valid. Clause can be addressed by "see" macro. For example, the following would produce an infinite loop:
```
1.2.
See 1.2.
```

## If-else statements
There are kinds of if-else statements. For example, here is the method trim() that returns its input and guards that output is not greater than 100. There are few ways to write it.

If-else statement can be written in one line, as far as every branch is a single sentence.
```
constant trim(uint256 input);
If INPUT > 100, let OUTPUT = 100, else let OUTPUT = INPUT.
Return uint256 at OUTPUT.
```

More complex logic can be fit in the extended form of if-else statement.
```
constant trim(uint256 input);
If INPUT > 100:
Let OUTPUT = 100.
Else:
Let OUTPUT = INPUT
End.
Return uint256 at OUTPUT.
```

TERMS language allows using if without else. It is processed as if else branch was empty.
```
constant trim(uint256 input);
If INPUT <= 100, see 1.1.
Let INPUT = 100.
1.1. Return uint256 at INPUT.
```

There is a special kind if-else:  if not - else. Due to decisions made in EVM, it spends less gas and produce shorter bytecode than the if-else version.
```
constant trim(uint256 input);
If not INPUT > 100, see 1.1.
Let INPUT = 100.
1.1. Return uint256 at INPUT.
```

## Procedures
Procedures provide the necessary abstraction for repeated sequences of calculations. They can also help producing smaller bytecode. Procedures take all available variables as their input and may change any of the variables, change state and to whatever is possible in the contract main section. Procedures are kept outside method bodies. It is recommended to keep procedures in a separate file. By convention, it has the same name as the main contract file, with extension .ttp instead of .tt
Here is a sample procedure:
```
Procedure "get allowance".
Let TMP = FROM - TO.
Let REMAINING read record TMP.
Procedure end.
```
A procedure starts with the word procedure followed by it's name in double quotes and ends with sentence "Procedure end." One is not allowed to call a procedure from another procedure.

Procedures can be used to replace Solidity modifiers. The following procedure only returns when called by contract owners.
```
Procedure "owners only".
Grab record CEO.
Grab record CFO.
If not CALLER CONSTANT == CEO or CALLER CONSTANT == CFO, stop.
Procedure end.
```

Let's say we want to only let those two to destroy the contract.
```
suicide();
Apply procedure "owners only".
Suicide.
Return.
```

## Taste of the language: records, grab
TERMS language introduces notion of "record" that is widely used in macros. Record in TERMS is a state variable sized 32 bytes. "Record string" is a sequence of state variables, 32 bytes each. Macros that operate with records, contain the word "record".
For example, if you have variable OWNER, you can let OWNDER be equal CALLER CONSTANT.
```
Let OWNER = CALLER CONSTANT.
```
In this case, OWNER is a memory variable that contains CALLER CONSTANT (provided by EVM as CALLER opcode is copied into offset of OWNER variable). TERMS statically assigns offsets to variables as they appear in code, so if this is the first variable we see, it is assigned offset 0x20.

We can write it into state variable with the same offset.
```
Write record OWNER.
```

Now we have both state and memory variables at offset 0x20 set to CALLER CONSTANT.
```
Write record OWNER.
```

Now we can change memory variable owner, let's set it to 0x00000000...
```
Let OWNER = 0.
```

Every time a contract is called, its every memory variable is set to 0. State variables are not like that, they are supposed to store a value. So let's imagine we want to check if CALLER CONSTANT is equal to OWNER and stop further execution if it is not. Now let's see what an owner-only method suicide() would look like.
```
suicide();
Grab record OWNER.
If not OWNER == CALLER CONSTANT, stop.
Suicide.
Return.
```
Return here is put to comply with the rule "one method - one return" of TERMS language. First, we use "grab" to copy state variable to memory variable with the same offset. Then we compare the memory variable with EVM constant that corresponds to contract caller (msg.sender, in terms of Solidity). If those addresses don't match, execution stops. Otherwise, we continue with the next sentence which destroys the contract and sends all money to the caller. If we want the method to take less space, we can write it like that:
```
suicide();
Grab record OWNER. If not OWNER == CALLER CONSTANT, stop, else suicide. Return.
```

## Macro language used in Solidity
TERMS macros are kept in a separate file and use a special kind of EVM assembly. A macro definition starts with "## " continued by lowercase words combined in a sentence. Beside words, macros can include variable arguments like ^1, ^2 etc. Variable argument numbers start from 1 and continues by incrementing the latest number by one. Macro definitions that don't follow the rule are invalid. Here is an example of a single macro.
```
## see ^1
JUMP ^1
```

TERMS macros don't don't use PUSH1, PUSH2, PUSH3 etc EVM instructions replacing them with a single instruction PUSH. There is a special kind of PUSH used in TERMS macros, PUSH* (push star). It is used to show that its operand can be contents of a variable. Let's see "## let ^1 = ^2" macro
```
## let ^1 = ^2
PUSH* ^2
PUSH ^1
MSTORE
```

```
Let NUMBER = 200.
```
Here we provide arguments: ^1 is a variable NUMBER with offset 0x20, ^2 is integer 200 that is translated to hex 0xC8. Here is the assembly output:
```
PUSH 0xC8
PUSH 0x20
MSTORE
```

Now we want to have a copy of NUMBER variable in another variable, NUMBER COPY.
```
Let NUMBER COPY = NUMBER.
```
Now ^1 is a variable NUMBER COPY with offset 0x40, ^2 the old variable NUMBER. The assembly output now is different:
```
PUSH 0x20
MLOAD
PUSH 0x40
MSTORE
```
It is different because this time we have to use MLOAD instruction to access contents of NUMBER variable.

Macros are allowed to use internal jumps and loops which allows an advanced class of macros. A good example of that kind of macros is a macro to copy a string from the contract state into memory. It would loop until SLOAD returns zero.
```
## grab string ^1
PUSH ^1
JUMPDEST loop
DUP1
DUP1
SLOAD
SWAP1
MSTORE
PUSH 0x20
ADD
DUP1
SLOAD
JUMPI loop
```

TERMS macro assembly language uses a special instruction set.
| instruction      | purpose  |
| :---:            | - |
| **INIT**         |  signifies end of contract constructor section and provides fallback dispatcher |
| **FALLBACK**     |  declares beginning of fallback section |
| **CONDITIONNOT** |  if-else condition, goes first, optional; executed when stack is 0 |
| **CONDITIONYES** |  if-else condition, goes next, optional; executed when stack is not 0 |
| **CONDITIONEND** |  if-else condition, ends the condition, requires CONDITIONNOT, CONDITIONYES or both |
| **REFJUMP**      |  saves backjump offset to memory offset 0x00, jumps to the procedure |
| **BACKJUMP**     |  jumps back to previously saved memory offset |
| **MSTORESEQ**    |  stores provided byte sequence in memory starting with PUSH address provided above |
| **SSTORESEQ**    |  stores provided byte sequence in state  starting with PUSH address provided above |
Those instructions are deeply baked into TERMS internal assembly language and should be used with care. In particular, gas costs for MSTORESEQ and SSTORESEQ is undefined. Use third party disassemblers to see why.

More info on current TERMS language implementation macros can be found in file
[terms.develop.txt](https://github.com/termslang/termslang/blob/master/terms.develop.txt)


## Contacts
Feel free to contact me on matters of the TERMS language project.


Mikhail Baynov  

e-mail: m.baynov@gmail.com

projet twitter: @termslang

donate ETH:  [0x701bc9829738a3a3feefe9e74294baa96b487d63](https://etherscan.io/address/0x701bc9829738a3a3feefe9e74294baa96b487d63)
