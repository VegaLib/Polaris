/*
 * Microsoft Public License (Ms-PL) - Copyright (c) 2020 Sean Moss
 * This file is subject to the terms and conditions of the Microsoft Public License, the text of which can be found in
 * the 'LICENSE' file at the root of this repository, or online at <https://opensource.org/licenses/MS-PL>.
 */

#pragma once

/// The main type for performing the parsing of Polaris shaders, the implementation is split into many files

#include <plsl/config.hpp>
#include <plsl/compiler.hpp>
#include "./error_listener.hpp"

#include "../../generated/PLSLBaseVisitor.h"

#define VISIT_DECL(type) antlrcpp::Any visit##type(grammar::PLSL::type##Context* ctx) override;


namespace plsl
{

// The exception type used to stop the tree walk and report an error
class ParserError final
{
public:
	ParserError(const CompilerError& error)
		: error_{ error }
	{ }
	ParserError(const string& msg, uint32 l, uint32 c)
		: error_{ CompilerStage::Parse, msg, l, c }
	{ }

	inline const CompilerError& error() const { return error_; }

private:
	const CompilerError error_;
}; // class ParserError


// The central visitor type that performs the parsing and AST walk
class Parser final :
	public grammar::PLSLBaseVisitor
{
	friend class ErrorListener;

public:
	Parser();
	virtual ~Parser();

	bool parse(const string& source, const CompilerOptions& options) noexcept;

	inline const CompilerError& lastError() const { return lastError_; }
	inline bool hasError() const { return !lastError_.message().empty(); }

	/* File-Level Rules */
	VISIT_DECL(File)
	VISIT_DECL(ShaderTypeStatement)

private:
	ErrorListener errorListener_;
	CompilerError lastError_;

	PLSL_NO_COPY(Parser)
	PLSL_NO_MOVE(Parser)
}; // class Parser

} // namespace plsl


#undef VISIT_DECL
