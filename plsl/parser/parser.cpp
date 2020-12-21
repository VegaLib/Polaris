/*
 * Microsoft Public License (Ms-PL) - Copyright (c) 2020 Sean Moss
 * This file is subject to the terms and conditions of the Microsoft Public License, the text of which can be found in
 * the 'LICENSE' file at the root of this repository, or online at <https://opensource.org/licenses/MS-PL>.
 */

#include "./parser.hpp"
#include "../../generated/PLSLLexer.h"
#include "../../generated/PLSL.h"

#define SET_ERROR(stage,...) lastError_ = CompilerError(CompilerStage::stage, ##__VA_ARGS__);


namespace plsl
{

// ====================================================================================================================
Parser::Parser()
	: errorListener_{ this }
	, lastError_{ }
{

}

// ====================================================================================================================
Parser::~Parser()
{

}

// ====================================================================================================================
bool Parser::parse(const string& source, const CompilerOptions& options) noexcept
{
	// Create the lexer object
	antlr4::ANTLRInputStream inStream{ source };
	grammar::PLSLLexer lexer{ &inStream };
	lexer.removeErrorListeners();
	lexer.addErrorListener(&errorListener_);

	// Create the parser object
	antlr4::CommonTokenStream tokenStream{ &lexer };
	grammar::PLSL parser{ &tokenStream };
	parser.removeErrorListeners();
	parser.addErrorListener(&errorListener_);

	// Perform the lexing and parsing
	const auto fileCtx = parser.file();
	if (hasError()) {
		return false; // The error listener in the lexer/parser picked up an error
	}

	// Visit the parsed file context
	try {
		const auto any = this->visit(fileCtx);
	}
	catch (const ParserError& pe) {
		lastError_ = pe.error();
		return false;
	}
	catch (const std::exception& ex) {
		SET_ERROR(Parse, mkstr("Unhanded error - %s", ex.what()));
		return false;
	}

	return true;
}

} // namespace plsl
