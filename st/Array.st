
"accessing"

Array method!
at: anInteger
	<primitive: 'array_at'>
	anInteger isInteger
		ifTrue: [self error: 'out of bounds array access'].
	anInteger isNumber
		ifTrue: [ self at: anInteger asInteger ]
		ifFalse: [self error: 'non-integer index']!

Array method!
at: anInteger put: anObject
	<primitive: 'array_at_put'>
	anInteger isInteger
		ifTrue: [self error: 'out of bounds array access'].
	anInteger isNumber
		ifTrue: [ self at: anInteger asInteger put: anObject]
		ifFalse: [self error: 'non-integer index']!

Array method!
asArray
	^ self!