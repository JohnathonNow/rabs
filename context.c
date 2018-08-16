#include "context.h"
#include "rabs.h"
#include "util.h"
#include "cache.h"
#include "target.h"
#include "stringmap.h"
#include <gc.h>
#include <unistd.h>

static stringmap_t ContextCache[1] = {STRINGMAP_INIT};
static ml_value_t *DefaultString;
static ml_type_t *ContextT;

context_t *context_find(const char *Path) {
	return stringmap_search(ContextCache, Path);
}

context_t *context_push(const char *Path) {
	context_t *Context = new(context_t);
	Context->Type = ContextT;
	Context->Parent = CurrentContext;
	Context->Path = Path;
	Context->Name = Path;
	Context->FullPath = concat(RootPath, Path, NULL);
	CurrentContext = Context;
	Context->Locals[0] = (stringmap_t)STRINGMAP_INIT;
	target_t *Default = Context->Default = (target_t *)target_meta_new(0, 1, &DefaultString);
	stringmap_insert(Context->Locals, "DEFAULT", Default);
	target_t *BuildDir = target_file_check(Path[0] == '/' ? Path + 1 : Path, 0);
	stringmap_insert(Context->Locals, "BUILDDIR", BuildDir);
	stringmap_insert(Context->Locals, "PATH", BuildDir);
	stringmap_insert(Context->Locals, "_", Context);
	stringmap_insert(ContextCache, Context->Name, Context);
	return Context;
}

context_t *context_scope(const char *Name) {
	context_t *Context = new(context_t);
	Context->Type = ContextT;
	Context->Parent = CurrentContext;
	Context->Path = CurrentContext->Path;
	Context->Name = concat(CurrentContext->Name, ":", Name, NULL);
	Context->FullPath = CurrentContext->FullPath;
	CurrentContext = Context;
	Context->Default = Context->Parent->Default;
	Context->Locals[0] = (stringmap_t)STRINGMAP_INIT;
	stringmap_insert(ContextCache, Context->Name, Context);
	return Context;
}

void context_pop() {
	CurrentContext = CurrentContext->Parent;
	chdir(concat(RootPath, CurrentContext->Path, NULL));
}

ml_value_t *context_symb_get(context_t *Context, const char *Name) {
	while (Context) {
		ml_value_t *Value = stringmap_search(Context->Locals, Name);
		if (Value) return Value;
		Context = Context->Parent;
	}
	return 0;
}

void context_symb_set(context_t *Context, const char *Name, ml_value_t *Value) {
	stringmap_insert(Context->Locals, Name, Value);
}

static ml_value_t *context_get_local(void *Data, int Count, ml_value_t **Args) {
	context_t *Context = (context_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	return ml_property(Context, Name, (ml_getter_t)context_symb_get, (ml_setter_t)context_symb_set, NULL, NULL);
}

static ml_value_t *context_get_parent(void *Data, int Count, ml_value_t **Args) {
	context_t *Context = (context_t *)Args[0];
	return (ml_value_t *)Context->Parent ?: MLNil;
}

static ml_value_t *context_path(void *Data, int Count, ml_value_t **Args) {
	context_t *Context = (context_t *)Args[0];
	return ml_string(Context->Path, -1);
}

static ml_value_t *context_get_subdir(void *Data, int Count, ml_value_t **Args) {
	context_t *Context = (context_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	const char *Path = concat(Context->Path, "/", Name, NULL);
	return (ml_value_t *)context_find(Path) ?: MLNil;
}

static ml_value_t *context_get_scope(void *Data, int Count, ml_value_t **Args) {
	context_t *Context = (context_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	const char *Path = concat(Context->Path, ":", Name, NULL);
	return (ml_value_t *)context_find(Path) ?: MLNil;
}

void context_init() {
	DefaultString = ml_string("DEFAULT", -1);
	ContextT = ml_class(MLAnyT, "context");
	ml_method_by_name(".", 0, context_get_local, ContextT, MLStringT, NULL);
	ml_method_by_name("parent", 0, context_get_parent, ContextT, NULL);
	ml_method_by_name("path", 0, context_path, ContextT, NULL);
	ml_method_by_name("/", 0, context_get_subdir, ContextT, MLStringT, NULL);
	ml_method_by_name("@", 0, context_get_scope, ContextT, MLStringT, NULL);
}
