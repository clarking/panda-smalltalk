

WideString method!
at: anInteger
	<primitive: 'wide_string_at'>
	anInteger isInteger
		ifTrue: [self error: 'index out of bounds'].
	anInteger isNumber
		ifTrue: [^ self at: anInteger asInteger]
		ifFalse: [self error: 'index is not an integer']!

WideString method!
at: anInteger put: aCharacter
	<primitive: 'wide_string_at_put'>
	anInteger isInteger
		ifTrue: [self error: 'index out of bounds'].
	anInteger isNumber
		ifTrue: [^ self at: anInteger asInteger]
		ifFalse: [self error: 'index is not an integer'].
	aCharacter isCharacter
		ifFalse: [self error: 'object is not a character']!


