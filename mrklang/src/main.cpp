#include "common/macros.h"
#include "common/logging.h"
#include "common/types.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/symbol_table.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>

using namespace MRK_NS;

std::string readFile(const Str& filename) {
    std::ifstream src(filename);
    std::string content((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
    return content;
}

int main() {
	std::cout << "mrklang codedom alpha\n";

    auto files = { "examples/hello.mrk", "examples/runtime.mrk" };

    Vec<UniquePtr<Program>> programs;

    for (const auto& filename : files) {
        MRK_INFO("Processing {}", filename);

        auto getSource = readFile(filename);
        // std::cout << "Src:\n" << "\033[47;30m" << source << "\033[0m" << "\n\n";

        MRK_INFO("Lexing...");
        auto startTime = std::chrono::high_resolution_clock::now();

        auto lexer = Lexer(getSource);
        auto& getTokens = lexer.tokenize();

        auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();

        MRK_INFO("Lexer took {} ms", delta);
        MRK_INFO("Token({}):", getTokens.size());

        for (auto& tok : getTokens) {
            std::cout
                << "Line: " << std::setw(4) << std::left << tok.getPosition.line
                << "\tColumn: " << std::setw(4) << std::left << tok.getPosition.column
                << "\tType: " << std::setw(10) << std::left << toString(tok.type)
                << "\tLexeme: " << tok.lexeme << '\n';
        }

        if (!lexer.getErrors().empty()) {
            MRK_ERROR("\033[47;30mCompilation failed due to lexer getErrors in {}.\033[0m\n", filename);

            lexer.reportErrors();
            return 1;
        }

        MRK_INFO("Parsing...");
        startTime = std::chrono::high_resolution_clock::now();

        auto parser = Parser(filename, &lexer);
        auto program = parser.parseProgram();

        delta = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();

        MRK_INFO("Parser took {} ms", delta);
        MRK_INFO("Statements({}):", program->statements.size());
        std::cout << program->toString() << '\n';

        if (!parser.getErrors().empty()) {
            MRK_ERROR("\033[47;30mCompilation failed due to parser getErrors in {}.\033[0m\n", filename);
            parser.reportErrors();

            return 1;
        }

        programs.push_back(Move(program));
    }

    // build symbol table
    MRK_INFO("Building symbol table, program count={}", programs.size());
    auto symTable = SymbolTable(Move(programs));
    symTable.build();
    symTable.dump();

	return 0;
}