/*
 * Microsoft Public License (Ms-PL) - Copyright (c) 2020 Sean Moss
 * This file is subject to the terms and conditions of the Microsoft Public License, the text of which can be found in
 * the 'LICENSE' file at the root of this repository, or online at <https://opensource.org/licenses/MS-PL>.
 */

#include "./Generator.hpp"

#include <fstream>

#define MAKE_ERROR(str) CompilerError(CompilerStage::Generate, str, 0, 0)


namespace plsl
{

// ====================================================================================================================
Generator::Generator(CompilerError* error)
	: error_{ error }
	, globals_{ }
	, stageHeaders_{ }
	, stageFunctions_{ }
	, currentFunc_{ nullptr }
{
	// Setup initial globals content
	globals_
		<< "/// This file generated by plslc\n"
		<< "#version 450 core\n";
}

// ====================================================================================================================
Generator::~Generator()
{

}

// ====================================================================================================================
void Generator::setCurrentStage(ShaderStages stage)
{
	if (stage == ShaderStages::None) {
		currentFunc_ = nullptr;
	}
	else {
		const auto name = ShaderStageToStr(stage);
		stageHeaders_[name] = {};
		currentFunc_ = &(stageFunctions_[name] = {});
	}
}

// ====================================================================================================================
void Generator::saveOutput() const
{
	// Open the file
	std::ofstream file{ "./plsl.output", std::ofstream::out | std::ofstream::trunc };
	if (!file.is_open()) {
		*error_ = MAKE_ERROR("failed to open output file");
		return;
	}

	// Write the globals
	file 
		<< "GLOBALS\n"
		<< "=======\n"
		<< globals_.str() << "\n"
		<< "\n" 
		<< std::endl;

	// Write the functions
	for (const auto& fnpair : stageFunctions_) {
		file
			<< "Shader Stage: " << fnpair.first << "\n"
			<< "==================\n"
			<< stageHeaders_.at(fnpair.first).str() << "\n"
			<< fnpair.second.str() << "\n"
			<< "\n"
			<< std::endl;
	}
}

} // namespace plsl
