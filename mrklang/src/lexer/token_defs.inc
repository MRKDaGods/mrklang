// --- Blocks ---
TOKEN(BLOCK_CSHARP)         /// C# code block
TOKEN(BLOCK_CPP)            /// C++ code block
TOKEN(BLOCK_DART)           /// Dart code block
TOKEN(BLOCK_JS)             /// JS Code block

// --- Keywords ---
TOKEN(KW_FUNC)              /// Function keyword
TOKEN(KW_CLASS)             /// Class keyword
TOKEN(KW_STRUCT)            /// Struct
TOKEN(KW_ENUM)              /// Enum
TOKEN(KW_VAR)               /// Variable keyword
TOKEN(KW_IF)                /// If keyword
TOKEN(KW_ELSE)              /// Else keyword
TOKEN(KW_FOR)               /// For keyword
TOKEN(KW_FOREACH)           /// Foreach keyword
TOKEN(KW_WHILE)             /// While keyword
TOKEN(KW_RETURN)            /// Return keyword
TOKEN(KW_NEW)               /// New keyword
TOKEN(KW_DELETE)            /// Delete
TOKEN(KW_IN)                /// In keyword
TOKEN(KW_AS)                /// As
TOKEN(KW_PARAMS)            /// params for varargs
TOKEN(KW_NAMESPACE)         /// NMS
TOKEN(KW_DECLSPEC)          /// __declspec(xxx)
TOKEN(KW_USE)               /// use xyz
TOKEN(KW_FROM)              /// from, to be used with use. use xyz from "LLL"
TOKEN(KW_GLOBAL)            /// Reserve __global namespace

// --- Access modifiers Keywords ---
TOKEN(KW_STATIC)            /// Static keyword
TOKEN(KW_PUBLIC)            /// Public
TOKEN(KW_PRIVATE)
TOKEN(KW_PROTECTED)
TOKEN(KW_VIRTUAL)
TOKEN(KW_OVERRIDE)

// --- Literals ---
TOKEN(LIT_INT)              /// Integer literal
TOKEN(LIT_FLOAT)            /// Float literal
TOKEN(LIT_HEX)              /// Hex literal
TOKEN(LIT_STRING)           /// String literal
TOKEN(LIT_CHAR)             /// Char literal
TOKEN(LIT_BOOL)             /// Boolean literal
TOKEN(LIT_LANG_BLOCK)       /// Cross lang block literal
TOKEN(LIT_NULL)             /// Null

// --- Identifiers ---
TOKEN(IDENTIFIER)           /// Identifier

// --- Arithmetic Operators ---
TOKEN(OP_PLUS)              /// Plus operator                +
TOKEN(OP_MINUS)             /// Minus operator               -
TOKEN(OP_ASTERISK)          /// Asterisk operator            *
TOKEN(OP_SLASH)             /// Slash operator               /
TOKEN(OP_MOD)               /// Modulo operator              %
TOKEN(OP_INCREMENT)         /// Increment                    ++
TOKEN(OP_DECREMENT)         /// Decrement                    --

// --- Assignment Operators ---
TOKEN(OP_EQ)                /// Assignment                   =
TOKEN(OP_PLUS_EQ)           /// Plus equals                  +=
TOKEN(OP_MINUS_EQ)          /// Minus equals                 -=
TOKEN(OP_MULT_EQ)           /// Multiply equals              *=
TOKEN(OP_DIV_EQ)            /// Divide equals                /=

// --- Comparison Operators ---
TOKEN(OP_EQ_EQ)             /// Equals                       ==
TOKEN(OP_NOT_EQ)            /// Not equal                    !=
TOKEN(OP_LT)                /// Less than                    <
TOKEN(OP_GT)                /// Greater than                 >
TOKEN(OP_LE)                /// Less than or equal           <=
TOKEN(OP_GE)                /// Greater than or equal        >=

// --- Logical Operators ---
TOKEN(OP_AND)               /// Logical AND                  &&
TOKEN(OP_OR)                /// Logical OR                   ||
TOKEN(OP_NOT)               /// Logical NOT                  !

// --- Bitwise Operators ---
TOKEN(OP_BAND)              /// Bitwise AND                  &
TOKEN(OP_BOR)               /// Bitwise OR                   |
TOKEN(OP_BNOT)              /// Bitwise NOT                  ~
TOKEN(OP_BXOR)              /// Bitwise XOR                  ^

// --- Special Operators ---
TOKEN(OP_DOUBLE_COLON)      /// Scope resolution             ::
TOKEN(OP_ARROW)             /// Arrow                        ->
TOKEN(OP_FAT_ARROW)         /// Fat arrow                    =>
TOKEN(OP_DOT)               /// Member access                .
TOKEN(OP_QUESTION)          /// Ternary conditional          ?

// --- Punctuation ---
TOKEN(SEMICOLON)            /// Semicolon                    ;
TOKEN(COMMA)                /// Comma                        ,
TOKEN(COLON)                /// Colon                        :
TOKEN(LPAREN)               /// Left parenthesis             (
TOKEN(RPAREN)               /// Right parenthesis            )
TOKEN(LBRACE)               /// Left brace                   {
TOKEN(RBRACE)               /// Right brace                  }
TOKEN(LBRACKET)             /// Left square bracket          [
TOKEN(RBRACKET)             /// Right square bracket         ]

// --- Comments ---
TOKEN(COMMENT_SINGLE)       /// Single comment               //
TOKEN(COMMENT_MULTI_START)  /// Multi comment start          /*
TOKEN(COMMENT_MULTI_END)    /// Multi comment end            */

// --- Special ---
TOKEN(INTERPOLATION)        /// Interpolation                $
TOKEN(ERROR)                /// Error
TOKEN(END_OF_FILE)          /// End of file
