/*
 * Microsoft Public License (Ms-PL) - Copyright (c) 2020-2021 Sean Moss
 * This file is subject to the terms and conditions of the Microsoft Public License, the text of which can be found in
 * the 'LICENSE' file at the root of this repository, or online at <https://opensource.org/licenses/MS-PL>.
 */

/// The main function entry point for the command-line VSL compiler 'vslc'

#include "../vsl/Compiler.hpp"

#include <iostream>


int main(int argc, char* argv[])
{
	using namespace vsl;

	try {
		Compiler c{ };
	}
	catch (const std::exception& ex) {
		std::cerr << "Unhandled exception: " << ex.what() << std::endl;
	}

	return 0;
}
