#include "core.h"
#include "profiler.h"
#include "common/logging.h"
#include "common/utils.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/symbol_table.h"
#include "codegen/code_generator.h"
#include "codegen/metadata_writer.h"

#include <fstream>

MRK_NS_BEGIN

using namespace ast;
using namespace semantic;
using namespace codegen;

Core::Core(const Vec<Str>& files)
	: errorReporter_(ErrorReporter::instance()) {
	readGlobalSymbolFile();
	readSourceFiles(files);
}

int Core::build() {
	if (sourceFiles_.empty()) {
		MRK_ERROR("No valid source files found");
		return 1;
	}

	// Process each source file
	for (const auto& src : sourceFiles_) {
		MRK_INFO("Processing {}", src->filename);
		errorReporter_.setCurrentFile(src.get());

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

		programs_.push_back(Move(program));
	}

	if (programs_.empty()) {
		MRK_ERROR("No programs were compiled");
		return 1;
	}

	// Resolve symbols
	if (!resolveSymbols()) {
		return 1;
	}

	// Print symbol table
	symbolTable_.dump();

	if (errorReporter_.hasErrors()) {
		MRK_ERROR("\033[47;30mCompilation failed due to linking errors.\033[0m");
		errorReporter_.reportErrors();
		return 1;
	}

	// Metadata
	MRK_INFO("Generating metadata...");
	MetadataWriter metadataWriter(&symbolTable_);
	auto registration = metadataWriter.writeMetadataFile("runtime_metadata.mrkmeta");
	if (!registration) {
		MRK_ERROR("Failed to generate metadata");
		return 1;
	}

	// Codegen...
	MRK_INFO("Generating code...");
	CodeGenerator generator(&symbolTable_, registration.get());
	auto code = generator.generateRuntimeCode();

	MRK_INFO("Generated code:\n{}", code);

	// Write to file
	std::ofstream out("runtime_generated.cpp");
	out << code;
	out.close();

	return 0;
}

void Core::readGlobalSymbolFile() {
	// Injected
	auto globalSyms = R"(
		// Injected global symbols
		__declspec(INJECT_GLOBAL) class __globalType {
			public static __declspec(INJECT_GLOBAL) func __globalFunction() {}
		}
	)";

	auto globalFile = MakeUnique<SourceFile>();
	globalFile->filename = "<global>";
	globalFile->contents.raw = globalSyms;
	sourceFiles_.push_back(Move(globalFile));
}

UniquePtr<SourceFile> Core::readSourceFile(const Str& filename) {
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

bool Core::lexFile(SourceFile* srcFile, Vec<Token>& tokens) {
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

	if (errorReporter_.hasErrors()) {
		MRK_ERROR("\033[47;30mCompilation failed due to lexer errors in {}.\033[0m", srcFile->filename);
		errorReporter_.reportErrors();
		return false;
	}

	return true;
}

UniquePtr<ast::Program> Core::parseFile(const SourceFile* sourceFile, Vec<Token>&& tokens) {
	MRK_INFO("Parsing...");

	Profiler::start();
	auto parser = Parser(Move(tokens));
	auto program = parser.parseProgram(sourceFile);
	auto delta = Profiler::stop();

	MRK_INFO("Parser took {} ms", delta);
	MRK_INFO("Statement count: {}", program->statements.size());
	std::cout << program->toString() << '\n';

	if (errorReporter_.hasErrors()) {
		MRK_ERROR("\033[47;30mCompilation failed due to parser errors in {}.\033[0m", sourceFile->filename);
		errorReporter_.reportErrors();
		return nullptr;
	}

	return program;
}

void Core::readSourceFiles(const Vec<Str>& files) {
	for (const auto& filename : files) {
		MRK_INFO("Reading {}", filename);

		auto sourceFile = readSourceFile(filename);
		if (!sourceFile) {
			MRK_ERROR("Failed to read source file: {}", filename);
			continue;
		}

		// sourceFile->filename = utils::concat("<file>", sourceFile->filename, "</file>");
		sourceFiles_.push_back(Move(sourceFile));
	}
}

bool Core::resolveSymbols() {
	MRK_INFO("Resolving symbols...");

	Profiler::start();
	symbolTable_ = SymbolTable(Move(programs_));
	symbolTable_.build();
	
	auto delta = Profiler::stop();
	MRK_INFO("Symbol table build took {} ms", delta);

	if (errorReporter_.hasErrors()) {
		MRK_ERROR("\033[47;30mCompilation failed due to semantic errors.\033[0m");
		errorReporter_.reportErrors();
		return false;
	}

	MRK_INFO("Symbol table built successfully");
	return true;
}

MRK_NS_END
