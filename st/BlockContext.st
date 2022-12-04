
"evaluation"

BlockContext method!
value
	<primitive: 'block_context_value'>
	self primitiveFailed!

BlockContext method!
value: argument
	<primitive: 'block_context_value'>
	self primitiveFailed!

BlockContext method!
value: firstArgument value: secondArgument
	<primitive: 'block_context_value'>
	self primitiveFailed!

BlockContext method!
value: firstArgument value: secondArgument value: thirdArgument
	<primitive: 'block_context_value'>
	self primitiveFailed!

BlockContext method!
valueWithArguments: anArray
	<primitive: 'block_context_value_with_args'>
	self primitiveFailed!


"controlling"

BlockContext method!
whileTrue
	^ [self value] whileTrue: [nil]!

BlockContext method!
whileFalse
	^ [self value] whileFalse: [nil]!

BlockContext method!
whileTrue: aBlock
	^ [self value] whileTrue: [aBlock value]!

BlockContext method!
whileFalse: aBlock
	^ [self value] whileFalse: [aBlock value]!

BlockContext method!
repeat
	^ [self value. true] whileTrue!


"error handling"

BlockContext method!
cannotReturn: anObject
	self error: 'cannot return object to a context that no longer exists'!
