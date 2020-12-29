/*
 * Microsoft Public License (Ms-PL) - Copyright (c) 2020 Sean Moss
 * This file is subject to the terms and conditions of the Microsoft Public License, the text of which can be found in
 * the 'LICENSE' file at the root of this repository, or online at <https://opensource.org/licenses/MS-PL>.
 */

#include "./Parser.hpp"
#include "./Expr.hpp"

#define VISIT_FUNC(type) antlrcpp::Any Parser::visit##type(grammar::PLSL::type##Context* ctx)


namespace plsl
{

// ====================================================================================================================
VISIT_FUNC(Statement)
{
	// Dispatch
	if (ctx->variableDefinition()) {
		visit(ctx->variableDefinition());
	}
	else if (ctx->variableDeclaration()) {
		visit(ctx->variableDeclaration());
	}
	else if (ctx->assignment()) {
		visit(ctx->assignment());
	}

	return nullptr;
}

// ====================================================================================================================
VISIT_FUNC(VariableDefinition)
{
	// Create the variable definition
	const auto varDecl = ctx->variableDeclaration();
	auto var = parseVariableDeclaration(varDecl, false);
	if (var.arraySize != 1) {
		ERROR(varDecl->arraySize, "Function-local variables cannot be arrays");
	}
	if (!var.dataType->isNumeric() && (var.dataType->baseType != ShaderBaseType::Boolean)) {
		ERROR(varDecl->baseType, "Function-local variable must be numeric or boolean types");
	}

	// TODO: Visit and check expression

	// Add the variable
	var.type = VariableType::Private;
	scopes_.addVariable(var);

	// TODO: Emit declaration and assignment

	return nullptr;
}

// ====================================================================================================================
VISIT_FUNC(VariableDeclaration)
{
	// Create the variable definition
	auto var = parseVariableDeclaration(ctx, false);
	if (var.arraySize != 1) {
		ERROR(ctx->arraySize, "Function-local variables cannot be arrays");
	}
	if (!var.dataType->isNumeric() && (var.dataType->baseType != ShaderBaseType::Boolean)) {
		ERROR(ctx->baseType, "Function-local variables must be numeric or boolean type");
	}

	// Add variable
	var.type = VariableType::Private;
	scopes_.addVariable(var);

	// TODO: Emit declaration

	return nullptr;
}

// ====================================================================================================================
VISIT_FUNC(Assignment)
{
	// Visit the left-hand side (abuse the Variable type to accomplish this)
	const std::shared_ptr<Variable> var{ visit(ctx->lval).as<Variable*>() };

	// TODO: Visit expression

	// TODO: Validate type compatiblity and compound-assignment operators

	// TODO: Emit assignment

	return nullptr;
}

// ====================================================================================================================
VISIT_FUNC(Lvalue)
{
	// Process different lvalue options
	if (ctx->name) {
		// Find the variable
		const auto varName = ctx->name->getText();
		const auto var = scopes_.getVariable(varName);
		if (!var) {
			ERROR(ctx->name, mkstr("No variable with name '%s' found", varName.c_str()));
		}
		if (!var->canWrite(currentStage_)) {
			ERROR(ctx->name, mkstr("The variable '%s' is read-only in this context", varName.c_str()));
		}

		return new Variable({}, varName, var->dataType, var->arraySize);
	}
	else {
		// Get the lvalue
		const std::shared_ptr<Variable> var{ visit(ctx->val).as<Variable*>() };
		if (var->name.find("imageStore") == 0) {
			ERROR(ctx->val, "Image or RWTexel stores must be top-level lvalue");
		}
		
		// Switch based on the lvalue type
		if (ctx->index) {
			// Visit the index expression
			const auto index = visit(ctx->index).as<std::shared_ptr<Expr>>();
			if (!index->type()->isNumeric() || (index->type()->numeric.dims[1] != 1) || (index->arraySize() != 1)) {
				ERROR(ctx->index, "Indexer argument must by a non-array numeric scalar or vector");
			}
			if (index->type()->baseType == ShaderBaseType::Float) {
				ERROR(ctx->index, "Indexer argument must be an integer type");
			}

			// A few different types can be used as arrays
			string refStr{};
			const ShaderType* refType{};
			if (var->dataType->baseType == ShaderBaseType::Image) {
				const auto dimcount = GetImageDimsComponentCount(var->dataType->image.dims);
				if (dimcount != index->type()->numeric.dims[0]) {
					ERROR(ctx->index, mkstr("Image type expects indexer with %u components", dimcount));
				}
				refStr = mkstr("imageStore(%s, %s, {})", var->name.c_str(), index->refString().c_str());
				refType = 
					types_.getNumericType(var->dataType->image.texel.type, var->dataType->image.texel.components, 1);
			}
			else if (var->dataType->baseType == ShaderBaseType::RWBuffer) {
				if (index->type()->numeric.dims[0] != 1) {
					ERROR(ctx->index, "RWBuffer expects a scalar integer indexer");
				}
				refStr = mkstr("(%s._data_[%s])", var->name.c_str(), ctx->index->getText().c_str());
				refType = types_.getType(var->dataType->buffer.structName);
			}
			else if (var->dataType->baseType == ShaderBaseType::RWTexels) {
				if (index->type()->numeric.dims[0] != 1) {
					ERROR(ctx->index, "RWTexels expects a scalar integer indexer");
				}
				refStr = mkstr("imageStore(%s, %s, {})", var->name.c_str(), index->refString().c_str());
				refType =
					types_.getNumericType(var->dataType->image.texel.type, var->dataType->image.texel.components, 1);
			}
			else if (var->arraySize != 1) {
				refStr = var->name;
				refType = var->dataType;
			}
			else if (var->dataType->isNumeric()) {
				if (var->dataType->numeric.dims[1] != 1) { // Matrix
					refStr = var->name;
					refType = types_.getNumericType(var->dataType->baseType, var->dataType->numeric.dims[0], 1);
				}
				else if (var->dataType->numeric.dims[0] != 1) { // Vector
					refStr = var->name;
					refType = types_.getNumericType(var->dataType->baseType, 1, 1);
				}
				else {
					ERROR(ctx->index, "Cannot apply indexer to scalar type");
				}
			}
			else {
				ERROR(ctx->index, "Type cannot receive an indexer");
			}

			return new Variable({}, refStr, refType, 1);
		}
		else if (ctx->SWIZZLE()) {
			// Validate data type
			const uint32 compCount = var->dataType->numeric.dims[0];
			if (!var->dataType->isNumeric() && (var->dataType->baseType != ShaderBaseType::Boolean)) {
				ERROR(ctx->SWIZZLE(), "Swizzles can only be applied to numeric types");
			}
			if ((compCount == 1) || (var->dataType->numeric.dims[1] != 1)) {
				ERROR(ctx->SWIZZLE(), "Swizzles can only be applied to a vector type");
			}
			validateSwizzle(compCount, ctx->SWIZZLE());

			// Get the new type
			const auto stxt = ctx->SWIZZLE()->getText();
			const auto stype = types_.getNumericType(var->dataType->baseType, uint32(stxt.length()), 1);

			return new Variable({}, mkstr("(%s.%s)", var->name.c_str(), stxt.c_str()), stype, 1);
		}
		else { // ctx->IDENTIFIER()
			// Validate data type
			if (!var->dataType->isStruct()) {
				ERROR(ctx->IDENTIFIER(), "Member access operator '.' can only be applied to struct types");
			}

			// Validate member
			const auto memName = ctx->IDENTIFIER()->getText();
			const auto memType = var->dataType->getMember(memName);
			if (!memType) {
				ERROR(ctx->IDENTIFIER(), 
					mkstr("Struct type '%s' does not have member with name '%s'", 
						var->dataType->userStruct.structName.c_str(), memName.c_str()));
			}

			return new Variable({}, mkstr("(%s.%s)", var->name.c_str(), memName.c_str()), 
				types_.getNumericType(memType->baseType, memType->dims[0], memType->dims[1]), memType->arraySize);
		}
	}
}

} // namespace plsl