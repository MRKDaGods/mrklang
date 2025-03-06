#include "error_reporter.h"
#include "common/logging.h"

#include <numeric>

MRK_NS_BEGIN

ErrorReporter& ErrorReporter::instance() {
	static ErrorReporter instance;
	return instance;
}

void ErrorReporter::setCurrentFile(const SourceFile* file) {
	currentFile_ = file;
}

void ErrorReporter::lexicalError(const Str& message, const LexerPosition& position, uint32_t length) {
	addError(MakeUnique<CompilerError>(currentFile_, CompilerError::Stage::LEXICAL, message, position.line, position.column, length));
}

void ErrorReporter::parserError(const Str& message, const Token& token, CompilerError** err) {
	auto error = MakeUnique<CompilerError>(currentFile_, CompilerError::Stage::PARSER, message, token.position.line, token.position.column, token.lexeme.size());
	if (err) {
		*err = error.get();
	}

	addError(Move(error));
}

void ErrorReporter::semanticError(const Str& message, const ast::Node* node) {
	auto& token = node->startToken;
	addError(MakeUnique<CompilerError>(currentFile_, CompilerError::Stage::SEMANTIC, message, token.position.line, token.position.column, token.lexeme.size()));
}

bool ErrorReporter::hasErrors() const {
	return !errors_.empty();
}

void ErrorReporter::reportErrors() const {
	for (const auto& [file, errors] : errors_) {
		std::cerr << "Errors in file: " << file->filename << "\n";
		auto lines = file->contents.lines();

		for (const auto& err : errors) {
			if (err->line > lines.size()) continue; // Skip invalid line numbers

			// Strip leading whitespace
			Str strippedLine = lines[err->line - 1];
			strippedLine.erase(0, strippedLine.find_first_not_of(" \t"));

			// Adjust col
			size_t indentation = lines[err->line - 1].find_first_not_of(" \t");
			if (indentation == Str::npos) {
				indentation = 0;
			}

			std::cerr << "Line: " << err->line << ", Col: " << err->column << "\n";
			std::cerr << strippedLine << "\n";

			// Squiggles
			int squiggleStart = std::max(0, (int)(err->column - 1 - indentation));

			std::cerr << Str(squiggleStart, ' ')					// Leading spaces
				<< Str(err->length, '~')							// Squiggles
				<< "  // Error: " << err->message << "\n\n";		// Error message
		}
	}
}

size_t ErrorReporter::errorCount() const {
	return std::transform_reduce(
		errors_.begin(), errors_.end(),
		size_t(0),
		std::plus<>(),
		[](const auto& pair) { return pair.second.size(); }
	);
}

void ErrorReporter::addError(UniquePtr<CompilerError>&& error) {
	if (!currentFile_) {
		MRK_ERROR("Error reported without a current file");
		return;
	}

	errors_[currentFile_].push_back(Move(error));
}

MRK_NS_END