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
#include "hash.h"

struct callmap_callee {
	int count;
	const char* whom;
};

struct callmap_caller {
	struct callmap_callee* list;
	int capacity;
	int size;
};

static CXCursor current_function;
static struct hashmap callmap;

static enum CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData client_data)
{
	CXString name = clang_getCursorSpelling(cursor);
	const char* name_cstr = clang_getCString(name);
	enum CXCursorKind kind = clang_getCursorKind(cursor);

	switch (kind) {
	case CXCursor_FunctionDecl: {
		current_function = cursor;
	} break;
	case CXCursor_CallExpr: {
		if (!clang_Cursor_isNull(current_function)) {
			CXString current_function_name = clang_getCursorSpelling(current_function);
			const char* current_function_name_cstr = clang_getCString(current_function_name);
			struct callmap_caller* caller_entry = hashmap_get(&callmap, current_function_name_cstr);
			if (caller_entry == NULL) {
				struct callmap_caller* new_caller_entry = malloc(sizeof(*new_caller_entry));
				new_caller_entry->capacity = 16;
				new_caller_entry->size = 0;
				new_caller_entry->list = calloc(new_caller_entry->capacity, sizeof(struct callmap_callee));
				hashmap_insert(&callmap, current_function_name_cstr, new_caller_entry);
				caller_entry = new_caller_entry;
			}
			bool fn_was_present = false;
			for (int i = 0; i < caller_entry->size; ++i) {
				struct callmap_callee* callee = caller_entry->list + i;
				if (strcmp(callee->whom, name_cstr) == 0) {
					++callee->count;
					fn_was_present = true;
					break;
				}
			}
			if (!fn_was_present) {
				if (caller_entry->capacity == caller_entry->size) {
					int new_capacity = caller_entry->capacity * 2;
					caller_entry->list = realloc(caller_entry->list, sizeof(struct callmap_callee) * new_capacity);
					caller_entry->capacity = new_capacity;
				}
				struct callmap_callee* callee_entry = caller_entry->list + caller_entry->size;
				callee_entry->count = 1;
				callee_entry->whom = strdup(name_cstr);
				++caller_entry->size;
			}
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

	hashmap_create(&callmap);

	CXIndex index = clang_createIndex(1, 0);
	CXCompilationDatabase compilation_db = clang_CompilationDatabase_fromDirectory(compile_commands_path, NULL);
	CXCompileCommands compile_commands = clang_CompilationDatabase_getAllCompileCommands(compilation_db);

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

	for (int i = 0; i < callmap.capacity; ++i) {
		struct hashmap_entry* entry = callmap.table + i;
		if (entry->exists) {
			const struct callmap_caller* caller = entry->value;
			for (int j = 0; j < caller->size; ++j) {
				const struct callmap_callee* callee = caller->list + j;
				printf("%s -> %s (%d)\n", entry->key, callee->whom, callee->count);
				free((void*) callee->whom);
			}
			free((void*) caller->list);
			free((void*) caller);
		}
	}

	hashmap_free(&callmap);
	return 0;
}
