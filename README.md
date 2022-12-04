## Introduction

Panda is a new Smalltalk implementation, written from scratch in C99.

### Implemented features:

- All Smalltalk-80 syntax supported.
- Fast bytecode interpreter (optionally uses gcc's computed goto)
- Mark-Compact garbage collector
- A small class library, with language core, data structures, and streams
- Basic support for reading code from command line

### Next on the TODO

- Image support
- better script file support

## Building

```bash
    mkdir build && cd build
    cmake .. && cmake --build .
```

Panda isn't installable right now. The `panda' executable runs in the
directory in which it was built ("build/"), and expects the kernel library files
to be in "../st".

## Usage

Use '-h' option to see available parameters.
The main executable can read files and output the result, as well as enter in a repl loop to evaluate expressions.

*This is in an eraly stage and isnt't intended for for everyday use.*

## Examples

Here are some short examples. Make sure to browse through the class library in "st/" to see all the implemented classes.

1. Sort a constant array of SmallIntegers

```bash
   echo "#(3 7 5 7 12 1) sort" | ./panda
```

2. Bignum arithmetic

```bash
   echo "1000000000000000000000000 + 1" | ./panda
```

3. Data Structures: Put a pair #key -> 'foo' into a dictionary and attempt retrieval

```smalltalk
   echo "|map| 
         map := Dictionary new. 
         map at: #key put: 'foo'.
         map at: #key." | ./panda
```

4. Fun with blocks: Use block to select elements of array which are greater than

```bash
    echo "#(3 7 5 7 12 1) select: [ :x | x > 5 ]" | ./panda
```

5. Errors: Panda prints a useful traceback if there is an runtime error

```bash
    echo "#(3 7 5 7 12 1) reverse" | ./panda
    An error occurred during program execution
    message: reverse
    
    Traceback:
    Array(Object)>>error:
    Array(Object)>>doesNotUnderstand:
    UndefinedObject>>doIt[]
    UndefinedObject>>doIt
    System>>startupSystem
```
