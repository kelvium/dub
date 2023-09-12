/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Nikita Romaniuk
 */

#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CXCursor current_function;

static enum CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData client_data)
{
	CXString name = clang_getCursorSpelling(cursor);
	enum CXCursorKind kind = clang_getCursorKind(cursor);

	switch (kind) {
	case CXCursor_FunctionDecl: {
		current_function = cursor;
	} break;
	case CXCursor_CallExpr: {
		if (!clang_Cursor_isNull(current_function)) {
			CXString current_function_name = clang_getCursorSpelling(current_function);
			printf("\"%s\" -> \"%s\"\n", clang_getCString(current_function_name), clang_getCString(name));
			clang_disposeString(current_function_name);
		}
	} break;
	default: break;
	}

	clang_disposeString(name);
	return CXChildVisit_Recurse;
}

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <compile_commands.json>", argv[0]);
		return 1;
	}
	const char* compile_commands_path = argv[1];

	CXIndex index = clang_createIndex(1, 0);
	CXCompilationDatabase compilation_db = clang_CompilationDatabase_fromDirectory(compile_commands_path, NULL);
	CXCompileCommands compile_commands = clang_CompilationDatabase_getAllCompileCommands(compilation_db);

	printf("digraph {\n");
	printf("graph [rankdir=LR ranksep=5 nodesep=0.01]\n");

	unsigned int commands_count = clang_CompileCommands_getSize(compile_commands);
	for (unsigned int i = 0; i < commands_count; ++i) {
		CXCompileCommand command = clang_CompileCommands_getCommand(compile_commands, i);

		int args_n = clang_CompileCommand_getNumArgs(command);
		const char** args = malloc(sizeof(const char*) * args_n);
		for (int j = 0; j < args_n; ++j) {
			CXString str = clang_CompileCommand_getArg(command, j);
			args[j] = strdup(clang_getCString(str));
			clang_disposeString(str);
		}

		CXTranslationUnit unit;
		enum CXErrorCode error = clang_parseTranslationUnit2(index, NULL, args, args_n, NULL, 0, CXTranslationUnit_None, &unit);
		if (error != CXError_Success) {
			fprintf(stderr, "failed to parse tu: %d\n", error);
			for (int j = 0; j < args_n; ++j)
				free((void*) args[j]);
			free(args);
			continue;
		}

		CXCursor cursor = clang_getTranslationUnitCursor(unit);
		clang_visitChildren(cursor, visitor, NULL);

		clang_disposeTranslationUnit(unit);

		for (int j = 0; j < args_n; ++j)
			free((void*) args[j]);
		free(args);
	}

	clang_CompileCommands_dispose(compile_commands);
	clang_CompilationDatabase_dispose(compilation_db);
	clang_disposeIndex(index);

	printf("}\n");
	return 0;
}
