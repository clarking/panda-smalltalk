

"arithmetic"

LargeInteger method!
+ aNumber
	<primitive: 'large_int_add'>
	^ super + aNumber!

LargeInteger method!
- aNumber
	<primitive: 'large_int_sub'>
	^ super - aNumber!

LargeInteger method!
* aNumber
	<primitive: 'large_int_mul'>
	^ super * aNumber!

LargeInteger method!
/ aNumber
	<primitive: 'large_int_div'>
	aNumber = 0
		ifTrue: [ self error: 'cannot divide by 0' ].
	(aNumber isMemberOf: LargeInteger)
		ifTrue: [ ^ Fraction numerator: self denominator: aNumber ]
		ifFalse: [ ^ super / aNumber ]!

LargeInteger method!
// aNumber
     <primitive: 'large_int_div'>
	^ super // aNumber!

LargeInteger method!
\\ aNumber
	<primitive: 'large_int_mod'>
	^ super \\ aNumber!

"mathematics"

LargeInteger method!
gcd: anInteger
	<primitive: 'large_int_gcd'>
	(anInteger isMemberOf: SmallInteger)
		ifTrue: [^ self gcd: (self coerce: anInteger)]
	    ifFalse: [ self error: 'primitive failed']!

LargeInteger method!
lcm: anInteger
	<primitive: 'large_int_lcm'>
	^ super lcm: anInteger!


"comparing"

LargeInteger method!
< aNumber
     <primitive: 'large_int_lt'>
	 ^ super < aNumber!

LargeInteger method!
> aNumber
     <primitive: 'large_int_gt'>
	 ^ super > aNumber!

LargeInteger method!
<= aNumber
     <primitive: 'large_int_le'>
	 ^ super <= aNumber!

LargeInteger method!
>= aNumber
     <primitive: 'large_int_ge'>
	 ^ super >= aNumber!

LargeInteger method!
= aNumber
     <primitive: 'large_int_eq'>
	 ^ super = aNumber!

LargeInteger method!
~= aNumber
     <primitive: 'large_int_ne'>
	 ^ super ~= aNumber!

LargeInteger method!
hash
     <primitive: 'large_int_hash'>
	 self primitiveFailed!


"bit-wise operations"

LargeInteger method!
bitOr: aNumber
     <primitive: 'large_int_bitOr'>
	 ^ super bitOr: aNumber!

LargeInteger method!
bitAnd: aNumber
     <primitive: 'large_int_bitAnd'>
	 ^ super bitAnd: aNumber!

LargeInteger method!
bitXor: aNumber
     <primitive: 'large_int_bitAnd'>
	 ^ super bitXor: aNumber!

LargeInteger method!
bitShift: aNumber
     <primitive: 'large_int_bitShift'>
	 ^ super bitShift: aNumber!


"coercion"

LargeInteger method!
asFloat
	<primitive: 'large_int_as_float'>
	self primitiveFailed!

LargeInteger method!
asLargeInteger
    ^ self!

LargeInteger method!
asInteger
    ^ self!

LargeInteger method!
generality
    ^ 20!

LargeInteger method!
zero
    ^ 0 asLargeInteger!

LargeInteger method!
unity
    ^ 1 asLargeInteger!

LargeInteger method!
coerce: aNumber
    ^ aNumber asLargeInteger!


"printing"
LargeInteger method!
printStringBase: anInteger
	<primitive: 'large_int_print_string_base'>
	anInteger isSmallInteger
		ifTrue: [ self error: 'expected a large integer' ]!

LargeInteger method!
printOn: aStream base: anInteger
	aStream nextPutAll: (self printStringBase: anInteger)!
