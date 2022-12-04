
"accessing"

ByteArray method!
at: anInteger
	<primitive: 'byte_array_at'>
	anInteger isInteger
		ifTrue: [self error: 'out of bounds array access'].
	anInteger isNumber
		ifTrue: [ self at: anInteger asInteger ]
		ifFalse: [self error: 'non-integer index']!

ByteArray method!
at: anInteger put: anObject
	<primitive: 'byte_array_at_put'>
	anInteger isInteger
		ifTrue: [self error: 'out of bounds array access'].
	anInteger isNumber
		ifTrue: [ self at: anInteger asInteger ]
		ifFalse: [self error: 'non-integer index']!

ByteArray method!
hash
	<primitive: 'byte_array_hash'>
	self primitiveFailed!