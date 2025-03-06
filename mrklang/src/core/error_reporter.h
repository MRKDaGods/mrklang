#pragma once

#include "common/types.h"
#include "lexer/token.h"
#include "parser/ast.h"
#include "source_file.h"

MRK_NS_BEGIN

/// Base class for all compiler errors
struct CompilerError {
    enum class Stage {
        LEXICAL,
        PARSER,
		SEMANTIC,
        CODEGEN
    };

    Stage stage;
    const SourceFile* file;
    Str message;
    uint32_t line;
    uint32_t column;
    uint32_t length;

    CompilerError(const SourceFile* file, Stage stage, Str message, uint32_t line, uint32_t column, uint32_t length = 1)
        : file(file), stage(stage), message(Move(message)), line(line), column(column), length(length) {}
};

/// Global error reporter class - singleton
class ErrorReporter {
public:
    static ErrorReporter& instance();

    // Prevent copying and moving
    ErrorReporter(const ErrorReporter&) = delete;
    ErrorReporter& operator=(const ErrorReporter&) = delete;

    void setCurrentFile(const SourceFile* file);
    void lexicalError(const Str& message, const LexerPosition& position, uint32_t length = 1);
    void parserError(const Str& message, const Token& token, CompilerError** err);
    void semanticError(const Str& message, const ast::Node* node);
    bool hasErrors() const;
    void reportErrors() const;
    size_t errorCount() const;

private:
    const SourceFile* currentFile_;
	Dict<const SourceFile*, Vec<UniquePtr<CompilerError>>> errors_;

    ErrorReporter() = default;
    void addError(UniquePtr<CompilerError>&& error);
};

MRK_NS_END