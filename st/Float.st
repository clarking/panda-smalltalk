"
Copyright (c) 2008 Vincent Geddes
Copyright (c) 2007 Luca Bruno

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the 'Software'), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
"

"coercing"

Float method!
asFloat
    ^ self!

Float method!
unity
    ^ 1.0!

Float method!
generality
    ^ 50!

Float method!
zero
    ^ 0.0!

Float method!
coerce: aNumber
    ^ aNumber asFloat!


"testing"

Float method!
isFloat
    ^ true!


"comparing"

Float method!
< aNumber
     <primitive: 'float_lt'>
	 ^ super < aNumber!

Float method!
> aNumber
     <primitive: 'float_gt'>
	 ^ super > aNumber!

Float method!
<= aNumber
     <primitive: 'float_le'>
	 ^ super <= aNumber!

Float method!
>= aNumber
     <primitive: 'float_ge'>
	 ^ super >= aNumber!

Float method!
= aNumber
     <primitive: 'float_eq'>
	 ^ super = aNumber!

Float method!
~= aNumber
     <primitive: 'float_ne'>
	 ^ super ~= aNumber!


"arithmetic"

Float method!
+ aNumber
     <primitive: 'float_add'>
	 ^ super + aNumber!

Float method!
- aNumber
     <primitive: 'float_sub'>
	 ^ super - aNumber!

Float method!
* aNumber
     <primitive: 'float_mul'>
	 ^ super * aNumber!

Float method!
/ aNumber
	<primitive: 'float_div'>
	^ super / aNumber!

Float method!
\\ aNumber
     self notYetImplemented!

Float method!
hash
	<primitive: 'float_hash'>
	self primitiveFailed!

"printing"

Float method!
printStringBase: anInteger
	<primitive: 'float_print_string_base'>
	self primitiveFailed!

Float method!
printOn: aStream base: anInteger
	aStream nextPutAll: (self printStringBase: anInteger)!

"rounding & truncation"

Float method!
truncated
	<primitive: 'float_truncated'>
	self primitiveFailed!

Float method!
fractionPart
	<primitive: 'float_fractionPart'>
	self primitiveFailed!

Float method!
integerPart
	<primitive: 'float_integerPart'>
	self primitiveFailed!


"mathematics"

Float method!
sin
	<primitive: 'float_sin'>
	self primitiveFailed!

Float method!
cos
	<primitive: 'float_cos'>
	self primitiveFailed!

Float method!
tan
	<primitive: 'float_tan'>
	self primitiveFailed!

Float method!
arcSin
	<primitive: 'float_arc_sin'>
	self primitiveFailed!

Float method!
arcCos
	<primitive: 'float_arc_cos'>
	self primitiveFailed!

Float method!
arcTan
	<primitive: 'float_arc_tan'>
	self primitiveFailed!

Float method!
exp
	<primitive: 'float_exp'>
	self primitiveFailed!

Float method!
ln
	<primitive: 'float_ln'>
	self primitiveFailed!

Float method!
log
	<primitive: 'float_log'>
	self primitiveFailed!

Float method!
sqrt
	<primitive: 'float_sqrt'>
	self primitiveFailed!
