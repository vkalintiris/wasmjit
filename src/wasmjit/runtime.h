/* -*-mode:c; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
  Copyright (c) 2018 Rian Hunter

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 */

#ifndef __WASMJIT__RUNTIME_H__
#define __WASMJIT__RUNTIME_H__

#include <wasmjit/ast.h>
#include <wasmjit/vector.h>

#include <stddef.h>
#include <stdint.h>

typedef size_t wasmjit_addr_t;
#define INVALID_ADDR ((wasmjit_addr_t) -1)

struct Addrs {
	size_t n_elts;
	wasmjit_addr_t *elts;
};

DECLARE_VECTOR_GROW(addrs, struct Addrs);

struct ModuleInst {
	struct FuncTypeVector {
		size_t n_elts;
		struct FuncType *elts;
	} types;
	struct Addrs funcaddrs;
	struct Addrs tableaddrs;
	struct Addrs memaddrs;
	struct Addrs globaladdrs;
};

DECLARE_VECTOR_GROW(func_types, struct FuncTypeVector);

struct Value {
	unsigned type;
	union ValueUnion {
		uint32_t i32;
		uint64_t i64;
		float f32;
		double f64;
	} data;
};

#define IS_HOST(funcinst) (!(funcinst)->module_inst)

struct Store {
	struct ModuleInstances {
		size_t n_elts;
		struct ModuleInst *elts;
	} modules;
	struct Namespace {
		size_t n_elts;
		struct NamespaceEntry {
			char *module_name;
			char *name;
			unsigned type;
			wasmjit_addr_t addr;
		} *elts;
	} names;
	struct StoreFuncs {
		wasmjit_addr_t n_elts;
		struct FuncInst {
			struct ModuleInst *module_inst;
			size_t code_length;
			struct Instr *code;
			size_t n_locals;
			struct {
				uint32_t count;
				uint8_t valtype;
			} *locals;
			void *compiled_code;
			struct FuncType type;
		} *elts;
	} funcs;
	struct TableFuncs {
		wasmjit_addr_t n_elts;
		struct TableInst {
			struct FuncInst **data;
			unsigned elemtype;
			size_t length;
			size_t max;
		} *elts;
	} tables;
	struct StoreMems {
		wasmjit_addr_t n_elts;
		struct MemInst {
			char *data;
			size_t size;
			size_t max; /* max of 0 means no max */
		} *elts;
	} mems;
	struct StoreGlobals {
		wasmjit_addr_t n_elts;
		struct GlobalInst {
			struct Value value;
			unsigned mut;
		} *elts;
	} globals;
	struct Addrs startfuncs;
};

DECLARE_VECTOR_GROW(store_module_insts, struct ModuleInstances);
DECLARE_VECTOR_GROW(store_names, struct Namespace);
DECLARE_VECTOR_GROW(store_funcs, struct StoreFuncs);
DECLARE_VECTOR_GROW(store_tables, struct TableFuncs);
DECLARE_VECTOR_GROW(store_mems, struct StoreMems);
DECLARE_VECTOR_GROW(store_globals, struct StoreGlobals);

#define WASM_PAGE_SIZE ((size_t) (64 * 1024))

int _wasmjit_create_func_type(struct FuncType *ft,
			      size_t n_inputs,
			      wasmjit_valtype_t *input_types,
			      size_t n_outputs, wasmjit_valtype_t *output_types);

wasmjit_addr_t _wasmjit_add_memory_to_store(struct Store *store,
					    size_t size, size_t max);
wasmjit_addr_t _wasmjit_add_function_to_store(struct Store *store,
					      struct ModuleInst *module_inst,
					      void *code,
					      size_t n_inputs,
					      wasmjit_valtype_t *input_types,
					      size_t n_outputs,
					      wasmjit_valtype_t *output_types);
wasmjit_addr_t _wasmjit_add_table_to_store(struct Store *store,
					   unsigned elemtype,
					   size_t length,
					   size_t max);
wasmjit_addr_t _wasmjit_add_global_to_store(struct Store *store,
					    struct Value value,
					    unsigned mut);
int _wasmjit_add_to_namespace(struct Store *store,
			      const char *module_name,
			      const char *name,
			      unsigned type,
			      wasmjit_addr_t addr);

int wasmjit_import_function(struct Store *store,
			    const char *module_name,
			    const char *name,
			    void *funcaddr,
			    size_t n_inputs,
			    wasmjit_valtype_t *input_types,
			    size_t n_outputs, wasmjit_valtype_t *output_types);

wasmjit_addr_t wasmjit_import_memory(struct Store *store,
				     const char *module_name,
				     const char *name,
				     size_t size, size_t max);

int wasmjit_import_table(struct Store *store,
			 const char *module_name,
			 const char *name,
			 unsigned elemtype,
			 size_t length,
			 size_t max);

int wasmjit_import_global(struct Store *store,
			  const char *module_name,
			  const char *name,
			  struct Value value,
			  unsigned mut);

int wasmjit_typecheck_func(struct FuncType *expected_type,
			   struct FuncInst *func);

int wasmjit_typecheck_table(struct TableType *expected_type,
			    struct TableInst *table);

int wasmjit_typecheck_memory(struct MemoryType *expected_type,
			     struct MemInst *mem);

int wasmjit_typecheck_global(struct GlobalType *expected_type,
			     struct GlobalInst *mem);

__attribute__ ((unused))
static int wasmjit_typelist_equal(size_t nelts, wasmjit_valtype_t *elts,
				  size_t onelts, wasmjit_valtype_t *oelts)
{
	size_t i;
	if (nelts != onelts) return 0;
	for (i = 0; i < nelts; ++i) {
		if (elts[i] != oelts[i]) return 0;
	}
	return 1;
}

#endif
