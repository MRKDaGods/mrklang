#include "common/macros.h"
#include "common/logging.h"
#include "common/types.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/symbol_table.h"
#include "core/source_file.h"
#include "core/error_reporter.h"
#include "core/profiler.h"

#include <iostream>
#include <iomanip>
#include <fstream>

using namespace MRK_NS;
using namespace semantic;

UniquePtr<SourceFile> readSourceFile(const Str& filename) {
    auto file = MakeUnique<SourceFile>();
    file->filename = filename;

    std::ifstream src(filename);
    if (!src.is_open()) {
        MRK_ERROR("Failed to open file: {}", filename);
        return nullptr;
    }

    file->contents.raw = std::string((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
    return file;
}

bool lexFile(SourceFile* srcFile, Vec<Token>& tokens) {
    MRK_INFO("Lexing...");

    Profiler::start();
    auto lexer = Lexer(srcFile->contents.raw);
    lexer.tokenize();
    tokens = Move(lexer.moveTokens());
    auto delta = Profiler::stop();

    MRK_INFO("Lexer took {} ms", delta);
    MRK_INFO("Token count: {}", tokens.size());

    for (auto& tok : tokens) {
        std::cout
            << "Line: " << std::setw(4) << std::left << tok.position.line
            << "\tColumn: " << std::setw(4) << std::left << tok.position.column
            << "\tType: " << std::setw(10) << std::left << toString(tok.type)
            << "\tLexeme: " << tok.lexeme << '\n';
    }

    auto& errorReporter = ErrorReporter::instance();
    if (errorReporter.hasErrors()) {
        MRK_ERROR("\033[47;30mCompilation failed due to lexer errors in {}.\033[0m", srcFile->filename);
        errorReporter.reportErrors();
        return false;
    }

    return true;
}

UniquePtr<Program> parseFile(const SourceFile* sourceFile, Vec<Token>&& tokens) {
    MRK_INFO("Parsing...");

    Profiler::start();
    auto parser = Parser(Move(tokens));
    auto program = parser.parseProgram(sourceFile);
    auto delta = Profiler::stop();

    MRK_INFO("Parser took {} ms", delta);
    MRK_INFO("Statement count: {}", program->statements.size());
    std::cout << program->toString() << '\n';

    auto& errorReporter = ErrorReporter::instance();
    if (errorReporter.hasErrors()) {
        MRK_ERROR("\033[47;30mCompilation failed due to parser errors in {}.\033[0m", sourceFile->filename);
        errorReporter.reportErrors();
        return nullptr;
    }

    return program;
}

int main() {
    std::cout << "mrklang codedom alpha\n";

    auto sourceFilenames = { "examples/hello.mrk", "examples/runtime.mrk" };
    Vec<UniquePtr<SourceFile>> sourceFiles;
    Vec<UniquePtr<Program>> programs;
    auto& errorReporter = ErrorReporter::instance();

    // Read all source files
    for (const auto& filename : sourceFilenames) {
        MRK_INFO("Reading {}", filename);
        auto sourceFile = readSourceFile(filename);
        if (!sourceFile) {
            MRK_ERROR("Failed to read source file: {}", filename);
            continue;
        }
        sourceFiles.push_back(Move(sourceFile));
    }

    if (sourceFiles.empty()) {
        MRK_ERROR("No valid source files found");
        return 1;
    }

    // Process each source file
    for (const auto& src : sourceFiles) {
        MRK_INFO("Processing {}", src->filename);
        errorReporter.setCurrentFile(src.get());

        // Lexical analysis
        Vec<Token> tokens;
        if (!lexFile(src.get(), tokens)) {
            continue;
        }

        // Parsing
        auto lexer = Lexer(src->contents.raw);
        lexer.tokenize(); // Need to tokenize again for parser to work
        auto program = parseFile(src.get(), Move(tokens));

        if (!program) {
            continue;
        }

        programs.push_back(Move(program));
    }

    if (programs.empty()) {
        MRK_ERROR("No programs successfully compiled");
        return 1;
    }

    // Build symbol table
    MRK_INFO("Building symbol table, program count={}", programs.size());
    auto symTable = SymbolTable(Move(programs));
    symTable.build();
	if (errorReporter.hasErrors()) {
		MRK_ERROR("\033[47;30mCompilation failed due to semantic errors.\033[0m");
		errorReporter.reportErrors();
		return 1;
	}

    symTable.dump();

    return 0;
}
