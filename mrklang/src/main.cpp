#include "common/macros.h"
#include "common/logging.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>

MRK_STD string readFile() {
    MRK_STD ifstream src("examples/runtime.mrk");
    MRK_STD string content((MRK_STD istreambuf_iterator<char>(src)), MRK_STD istreambuf_iterator<char>());
    return content;
}

int main() {
	MRK_STD cout << "mrklang codedom alpha\n";

	auto source = readFile();
    MRK_STD cout << "Src:\n" << "\033[47;30m" << source << "\033[0m" << "\n\n";

    MRK_STD cout << "Lexing...\n";
    auto startTime = MRK_CHRONO high_resolution_clock::now();

	auto lexer = MRK Lexer(source);
	auto& tokens = lexer.tokenize();

    auto delta = MRK_CHRONO duration_cast<MRK_CHRONO milliseconds>(
        MRK_CHRONO high_resolution_clock::now() - startTime).count();

    MRK_STD cout << "Lexer took " << delta << " ms\n";
	MRK_STD cout << "Tokens (" << tokens.size() << "):\n";

    for (auto& tok : tokens) {
        MRK_STD cout
            << "Line: "             << MRK_STD setw(4)  << MRK_STD left     << tok.position.line
            << "\tColumn: "         << MRK_STD setw(4)  << MRK_STD left     << tok.position.column
            << "\tType: "           << MRK_STD setw(10) << MRK_STD left     << MRK toString(tok.type)
            << "\tLexeme: "                                                 << tok.lexeme << '\n';
    }

    if (!lexer.errors().empty()) {
        MRK_STD cout << "\n\033[47;30mCompilation failed due to lexer errors.\033[0m\n\n";

        lexer.reportErrors();
        return 1;
    }

    MRK_STD cout << "Parsing...\n";
    startTime = MRK_CHRONO high_resolution_clock::now();

    auto parser = MRK Parser(&lexer);
    auto program = parser.parse();

    delta = MRK_CHRONO duration_cast<MRK_CHRONO milliseconds>(
        MRK_CHRONO high_resolution_clock::now() - startTime).count();

    MRK_STD cout << "Parser took " << delta << " ms\n";
    MRK_STD cout << "Statements (" << program->statements.size() << "):\n";
    MRK_STD cout << program->toString() << '\n';

    if (!parser.errors().empty()) {
        MRK_STD cout << "\n\033[47;30mCompilation failed due to parser errors.\033[0m\n\n";
        parser.reportErrors();

        return 1;
    }

	return 0;
}