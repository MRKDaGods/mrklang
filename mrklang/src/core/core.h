#include "common/types.h"
#include "source_file.h"
#include "parser/ast.h"
#include "error_reporter.h"
#include "semantic/symbol_table.h"

#define MRKLANG_COMPILER
#include "mrk-metadata.h"

MRK_NS_BEGIN

class Core {
public:
	Core(const Vec<Str>& files);
	int build();

private:
	Vec<UniquePtr<SourceFile>> sourceFiles_;
	Vec<UniquePtr<ast::Program>> programs_;
	ErrorReporter& errorReporter_;
	semantic::SymbolTable symbolTable_;

	UniquePtr<SourceFile> readSourceFile(const Str& filename);
	bool lexFile(SourceFile* srcFile, Vec<Token>& tokens);
	UniquePtr<ast::Program> parseFile(const SourceFile* sourceFile, Vec<Token>&& tokens);
	void readSourceFiles(const Vec<Str>& files);
	bool resolveSymbols();
	void buildMetadata();
};

MRK_NS_END