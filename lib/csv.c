#include "csv.h"
#include "minilang.h"
#include "minilang/stringmap.h"
#include "rabs.h"
#include <string.h>
#include <stdio.h>
#include <gc.h>

static ml_type_t *CsvT;

typedef struct csv_row_t csv_row_t;

typedef struct csv_t {
	const ml_type_t *Type;
	FILE *File;
	csv_row_t *Head, *Tail;
	ml_value_t *Row;
	struct csv_parser Parser[1];
} csv_t;

struct csv_row_t {
	csv_row_t *Next;
	ml_value_t *Values;
};

static void field_fn(void *Buffer, size_t Size, csv_t *Csv) {
	char *Copy = GC_malloc_atomic(Size + 1);
	memcpy(Copy, Buffer, Size);
	Copy[Size] = 0;
	ml_list_append(Csv->Row, ml_string(Copy, Size));
}

static void row_fn(int Delim, csv_t *Csv) {
	csv_row_t *Row = new(csv_row_t);
	Row->Values = Csv->Row;
	Csv->Row = ml_list();
	if (Csv->Tail) {
		Csv->Tail->Next = Row;
	} else {
		Csv->Head = Row;
	}
	Csv->Tail = Row;
}

static ml_value_t *csv_read_fn(void *Data, int Count, ml_value_t **Args) {
	csv_t *Csv = (csv_t *)Args[0];
	char Buffer[256];
	while (!Csv->Head) {
		size_t Size = fread(Buffer, 1, 256, Csv->File);
		if (Size == 0) return MLNil;
		csv_parse(Csv->Parser, Buffer, Size, (void *)field_fn, (void *)row_fn, Csv);
	}
	csv_row_t *Row = Csv->Head;
	Csv->Head = Row->Next;
	if (!Row->Next) Csv->Tail = 0;
	return Row->Values;
}

static ml_value_t *StringMethod;

static ml_value_t *csv_write_fn(void *Data, int Count, ml_value_t **Args) {
	csv_t *Csv = (csv_t *)Args[0];
	ml_list_t *Values = (ml_list_t *)Args[1];
	for (ml_list_node_t *Node = Values->Head; Node; Node = Node->Next) {
		ml_value_t *Result = Node->Value;
		if (Result->Type != MLStringT) {
			Result = ml_call(StringMethod, 1, &Result);
			if (Result->Type == MLErrorT) return Result;
			if (Result->Type != MLStringT) return ml_error("ResultError", "string method did not return string");
		}
		csv_fwrite(Csv->File, ml_string_value(Result), ml_string_length(Result));
		if (Node->Next) fputc(',', Csv->File);
	}
	fputc('\n', Csv->File);
	return Args[0];
}

static ml_value_t *csv_close_fn(void *Data, int Count, ml_value_t **Args) {
	csv_t *Csv = (csv_t *)Args[0];
	if (Csv->File) {
		fclose(Csv->File);
		Csv->File = 0;
	}
	return MLNil;
}

static void csv_finalize(csv_t *Csv, void *Data) {
	if (Csv->File) {
		fclose(Csv->File);
		Csv->File = 0;
	}
}

ml_value_t *open(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLStringT);
	const char *Path = ml_string_value(Args[0]);
	const char *Mode = ml_string_value(Args[1]);
	FILE *File = fopen(Path, Mode);
	if (!File) return ml_error("FileError", "failed to open %s in mode %s", Path, Mode);
	csv_t *Csv= new(csv_t);
	Csv->Type = CsvT;
	Csv->File = File;
	csv_init(Csv->Parser, 0);
	Csv->Row = ml_list();
	Csv->Parser->malloc_func = GC_malloc;
	Csv->Parser->realloc_func = GC_realloc;
	Csv->Parser->free_func = GC_free;
	GC_register_finalizer(Csv, (void *)csv_finalize, 0, 0, 0);
	return (ml_value_t *)Csv;
}

void *init(stringmap_t *Globals) {
	StringMethod = ml_method("string");
	CsvT = ml_class(MLAnyT, "csv-file");
	ml_method_by_name("read", 0, csv_read_fn, CsvT, NULL);
	ml_method_by_name("write", 0, csv_write_fn, CsvT, MLListT, NULL);
	ml_method_by_name("close", 0, csv_close_fn, CsvT, NULL);
	return 0;
}
