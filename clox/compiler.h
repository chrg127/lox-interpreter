#ifndef COMPILER_H_INCLUDED
#define COMPILER_H_INCLUDED

#include <stdbool.h>
#include "object.h"

ObjFunction *compile(const char *src, ObjString *filename);
void compiler_mark_roots();
void compile_vm_end();
void compiler_enable_print_code();

#endif
