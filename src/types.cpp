struct Scope;
struct AstNode;

enum BasicKind {
	Basic_Invalid,
	Basic_bool,
	Basic_i8,
	Basic_u8,
	Basic_i16,
	Basic_u16,
	Basic_i32,
	Basic_u32,
	Basic_i64,
	Basic_u64,
	Basic_i128,
	Basic_u128,

	Basic_rune,

	// Basic_f16,
	Basic_f32,
	Basic_f64,

	// Basic_complex32,
	Basic_complex64,
	Basic_complex128,

	Basic_int,
	Basic_uint,
	Basic_rawptr,
	Basic_string, // ^u8 + int
	Basic_any,    // ^Type_Info + rawptr

	Basic_UntypedBool,
	Basic_UntypedInteger,
	Basic_UntypedFloat,
	Basic_UntypedComplex,
	Basic_UntypedString,
	Basic_UntypedRune,
	Basic_UntypedNil,
	Basic_UntypedUndef,

	Basic_COUNT,

	Basic_byte = Basic_u8,
};

enum BasicFlag {
	BasicFlag_Boolean     = GB_BIT(0),
	BasicFlag_Integer     = GB_BIT(1),
	BasicFlag_Unsigned    = GB_BIT(2),
	BasicFlag_Float       = GB_BIT(3),
	BasicFlag_Complex     = GB_BIT(4),
	BasicFlag_Pointer     = GB_BIT(5),
	BasicFlag_String      = GB_BIT(6),
	BasicFlag_Rune        = GB_BIT(7),
	BasicFlag_Untyped     = GB_BIT(8),

	BasicFlag_Numeric      = BasicFlag_Integer | BasicFlag_Float   | BasicFlag_Complex,
	BasicFlag_Ordered      = BasicFlag_Integer | BasicFlag_Float   | BasicFlag_String  | BasicFlag_Pointer | BasicFlag_Rune,
	BasicFlag_ConstantType = BasicFlag_Boolean | BasicFlag_Numeric | BasicFlag_String  | BasicFlag_Pointer | BasicFlag_Rune,
};

struct BasicType {
	BasicKind kind;
	u32       flags;
	i64       size; // -1 if arch. dep.
	String    name;
};

enum TypeRecordKind {
	TypeRecord_Invalid,

	TypeRecord_Struct,
	TypeRecord_RawUnion,
	TypeRecord_Union, // Tagged
	TypeRecord_Enum,

	TypeRecord_Count,
};

struct TypeRecord {
	TypeRecordKind kind;

	// All record types
	// Theses are arrays
	// Entity_Variable - struct/raw_union/union (for common fields)
	// Entity_Constant - enum
	Entity **fields;
	i32      field_count; // == struct_offsets count
	Entity **fields_in_src_order; // Entity_Variable
	AstNode *node;

	// Entity_TypeName - union
	Entity **variants;
	i32      variant_count;
	Entity * union__tag;
	i64      variant_block_size; // NOTE(bill): Internal use only

	Type *   variant_parent;
	i32      variant_index;

	i64 *    offsets;
	bool     are_offsets_set;
	bool     are_offsets_being_processed;
	bool     is_packed;
	bool     is_ordered;

	i64      custom_align; // NOTE(bill): Only used in structs at the moment
	Entity * names;

	Type *   enum_base_type;
	Entity * enum_count;
	Entity * enum_min_value;
	Entity * enum_max_value;
};

#define TYPE_KINDS                                        \
	TYPE_KIND(Basic,   BasicType)                         \
	TYPE_KIND(Generic, struct{ i64 id; })                 \
	TYPE_KIND(Pointer, struct { Type *elem; })            \
	TYPE_KIND(Atomic,  struct { Type *elem; })            \
	TYPE_KIND(Array,   struct { Type *elem; i64 count; }) \
	TYPE_KIND(DynamicArray, struct { Type *elem; })       \
	TYPE_KIND(Vector,  struct { Type *elem; i64 count; }) \
	TYPE_KIND(Slice,   struct { Type *elem; })            \
	TYPE_KIND(Record,  TypeRecord)                        \
	TYPE_KIND(Named, struct {                             \
		String  name;                                     \
		Type *  base;                                     \
		Entity *type_name; /* Entity_TypeName */          \
	})                                                    \
	TYPE_KIND(Tuple, struct {                             \
		Entity **variables; /* Entity_Variable */         \
		i32      variable_count;                          \
		bool     are_offsets_set;                         \
		i64 *    offsets;                                 \
	})                                                    \
	TYPE_KIND(Proc, struct {                              \
		AstNode *node;                                    \
		Scope *  scope;                                   \
		Type *   params;  /* Type_Tuple */                \
		Type *   results; /* Type_Tuple */                \
		i32      param_count;                             \
		i32      result_count;                            \
		bool     return_by_pointer;                       \
		Type **  abi_compat_params;                       \
		Type *   abi_compat_result_type;                  \
		bool     variadic;                                \
		bool     require_results;                         \
		bool     c_vararg;                                \
		bool     is_generic;                              \
		bool     is_generic_specialized;                  \
		ProcCallingConvention calling_convention;         \
	})                                                    \
	TYPE_KIND(Map, struct {                               \
		i64   count; /* 0 if dynamic */                   \
		Type *key;                                        \
		Type *value;                                      \
		Type *entry_type;                                 \
		Type *generated_struct_type;                      \
		Type *lookup_result_type;                         \
	})                                                    \
	TYPE_KIND(BitFieldValue, struct { u32 bits; })        \
	TYPE_KIND(BitField, struct {                          \
		Scope *  scope;                                   \
		Entity **fields;                                  \
		i32      field_count;                             \
		u32 *    offsets;                                 \
		u32 *    sizes;                                   \
		i64      custom_align;                            \
	})                                                    \



enum TypeKind {
	Type_Invalid,
#define TYPE_KIND(k, ...) GB_JOIN2(Type_, k),
	TYPE_KINDS
#undef TYPE_KIND
	Type_Count,
};

String const type_strings[] = {
	{cast(u8 *)"Invalid", gb_size_of("Invalid")},
#define TYPE_KIND(k, ...) {cast(u8 *)#k, gb_size_of(#k)-1},
	TYPE_KINDS
#undef TYPE_KIND
};

#define TYPE_KIND(k, ...) typedef __VA_ARGS__ GB_JOIN2(Type, k);
	TYPE_KINDS
#undef TYPE_KIND

struct Type {
	TypeKind kind;
	union {
#define TYPE_KIND(k, ...) GB_JOIN2(Type, k) k;
	TYPE_KINDS
#undef TYPE_KIND
	};
	bool failure;
};


// TODO(bill): Should I add extra information here specifying the kind of selection?
// e.g. field, constant, vector field, type field, etc.
struct Selection {
	Entity *   entity;
	Array<i32> index;
	bool       indirect; // Set if there was a pointer deref anywhere down the line
};
Selection empty_selection = {0};

Selection make_selection(Entity *entity, Array<i32> index, bool indirect) {
	Selection s = {entity, index, indirect};
	return s;
}

void selection_add_index(Selection *s, isize index) {
	// IMPORTANT NOTE(bill): this requires a stretchy buffer/dynamic array so it requires some form
	// of heap allocation
	// TODO(bill): Find a way to use a backing buffer for initial use as the general case is probably .count<3
	if (s->index.data == NULL) {
		array_init(&s->index, heap_allocator());
	}
	array_add(&s->index, cast(i32)index);
}



gb_global Type basic_types[] = {
	{Type_Basic, {Basic_Invalid,           0,                                          0, STR_LIT("invalid type")}},

	{Type_Basic, {Basic_bool,              BasicFlag_Boolean,                          1, STR_LIT("bool")}},

	{Type_Basic, {Basic_i8,                BasicFlag_Integer,                          1, STR_LIT("i8")}},
	{Type_Basic, {Basic_u8,                BasicFlag_Integer | BasicFlag_Unsigned,     1, STR_LIT("u8")}},
	{Type_Basic, {Basic_i16,               BasicFlag_Integer,                          2, STR_LIT("i16")}},
	{Type_Basic, {Basic_u16,               BasicFlag_Integer | BasicFlag_Unsigned,     2, STR_LIT("u16")}},
	{Type_Basic, {Basic_i32,               BasicFlag_Integer,                          4, STR_LIT("i32")}},
	{Type_Basic, {Basic_u32,               BasicFlag_Integer | BasicFlag_Unsigned,     4, STR_LIT("u32")}},
	{Type_Basic, {Basic_i64,               BasicFlag_Integer,                          8, STR_LIT("i64")}},
	{Type_Basic, {Basic_u64,               BasicFlag_Integer | BasicFlag_Unsigned,     8, STR_LIT("u64")}},
	{Type_Basic, {Basic_i128,              BasicFlag_Integer,                         16, STR_LIT("i128")}},
	{Type_Basic, {Basic_u128,              BasicFlag_Integer | BasicFlag_Unsigned,    16, STR_LIT("u128")}},

	{Type_Basic, {Basic_rune,              BasicFlag_Integer | BasicFlag_Rune,         4, STR_LIT("rune")}},

	// {Type_Basic, {Basic_f16,               BasicFlag_Float,                            2, STR_LIT("f16")}},
	{Type_Basic, {Basic_f32,               BasicFlag_Float,                            4, STR_LIT("f32")}},
	{Type_Basic, {Basic_f64,               BasicFlag_Float,                            8, STR_LIT("f64")}},

	// {Type_Basic, {Basic_complex32,         BasicFlag_Complex,                          4, STR_LIT("complex32")}},
	{Type_Basic, {Basic_complex64,         BasicFlag_Complex,                          8, STR_LIT("complex64")}},
	{Type_Basic, {Basic_complex128,        BasicFlag_Complex,                         16, STR_LIT("complex128")}},

	{Type_Basic, {Basic_int,               BasicFlag_Integer,                         -1, STR_LIT("int")}},
	{Type_Basic, {Basic_uint,              BasicFlag_Integer | BasicFlag_Unsigned,    -1, STR_LIT("uint")}},

	{Type_Basic, {Basic_rawptr,            BasicFlag_Pointer,                         -1, STR_LIT("rawptr")}},
	{Type_Basic, {Basic_string,            BasicFlag_String,                          -1, STR_LIT("string")}},
	{Type_Basic, {Basic_any,               0,                                         -1, STR_LIT("any")}},

	{Type_Basic, {Basic_UntypedBool,       BasicFlag_Boolean    | BasicFlag_Untyped,   0, STR_LIT("untyped bool")}},
	{Type_Basic, {Basic_UntypedInteger,    BasicFlag_Integer    | BasicFlag_Untyped,   0, STR_LIT("untyped integer")}},
	{Type_Basic, {Basic_UntypedFloat,      BasicFlag_Float      | BasicFlag_Untyped,   0, STR_LIT("untyped float")}},
	{Type_Basic, {Basic_UntypedComplex,    BasicFlag_Complex    | BasicFlag_Untyped,   0, STR_LIT("untyped complex")}},
	{Type_Basic, {Basic_UntypedString,     BasicFlag_String     | BasicFlag_Untyped,   0, STR_LIT("untyped string")}},
	{Type_Basic, {Basic_UntypedRune,       BasicFlag_Integer    | BasicFlag_Untyped,   0, STR_LIT("untyped rune")}},
	{Type_Basic, {Basic_UntypedNil,        BasicFlag_Untyped,                          0, STR_LIT("untyped nil")}},
	{Type_Basic, {Basic_UntypedUndef,      BasicFlag_Untyped,                          0, STR_LIT("untyped undefined")}},
};

// gb_global Type basic_type_aliases[] = {
// 	// {Type_Basic, {Basic_byte, BasicFlag_Integer | BasicFlag_Unsigned, 1, STR_LIT("byte")}},
// 	// {Type_Basic, {Basic_rune, BasicFlag_Integer,                      4, STR_LIT("rune")}},
// };

gb_global Type *t_invalid         = &basic_types[Basic_Invalid];
gb_global Type *t_bool            = &basic_types[Basic_bool];
gb_global Type *t_i8              = &basic_types[Basic_i8];
gb_global Type *t_u8              = &basic_types[Basic_u8];
gb_global Type *t_i16             = &basic_types[Basic_i16];
gb_global Type *t_u16             = &basic_types[Basic_u16];
gb_global Type *t_i32             = &basic_types[Basic_i32];
gb_global Type *t_u32             = &basic_types[Basic_u32];
gb_global Type *t_i64             = &basic_types[Basic_i64];
gb_global Type *t_u64             = &basic_types[Basic_u64];
gb_global Type *t_i128            = &basic_types[Basic_i128];
gb_global Type *t_u128            = &basic_types[Basic_u128];

gb_global Type *t_rune            = &basic_types[Basic_rune];

// gb_global Type *t_f16             = &basic_types[Basic_f16];
gb_global Type *t_f32             = &basic_types[Basic_f32];
gb_global Type *t_f64             = &basic_types[Basic_f64];

// gb_global Type *t_complex32       = &basic_types[Basic_complex32];
gb_global Type *t_complex64       = &basic_types[Basic_complex64];
gb_global Type *t_complex128      = &basic_types[Basic_complex128];

gb_global Type *t_int             = &basic_types[Basic_int];
gb_global Type *t_uint            = &basic_types[Basic_uint];

gb_global Type *t_rawptr          = &basic_types[Basic_rawptr];
gb_global Type *t_string          = &basic_types[Basic_string];
gb_global Type *t_any             = &basic_types[Basic_any];

gb_global Type *t_untyped_bool       = &basic_types[Basic_UntypedBool];
gb_global Type *t_untyped_integer    = &basic_types[Basic_UntypedInteger];
gb_global Type *t_untyped_float      = &basic_types[Basic_UntypedFloat];
gb_global Type *t_untyped_complex    = &basic_types[Basic_UntypedComplex];
gb_global Type *t_untyped_string     = &basic_types[Basic_UntypedString];
gb_global Type *t_untyped_rune       = &basic_types[Basic_UntypedRune];
gb_global Type *t_untyped_nil        = &basic_types[Basic_UntypedNil];
gb_global Type *t_untyped_undef      = &basic_types[Basic_UntypedUndef];


gb_global Type *t_u8_ptr       = NULL;
gb_global Type *t_int_ptr      = NULL;
gb_global Type *t_i64_ptr      = NULL;
gb_global Type *t_i128_ptr     = NULL;
gb_global Type *t_f64_ptr      = NULL;
gb_global Type *t_u8_slice     = NULL;
gb_global Type *t_string_slice = NULL;


// Type generated for the "preload" file
gb_global Type *t_type_info                   = NULL;
gb_global Type *t_type_info_record            = NULL;
gb_global Type *t_type_info_enum_value        = NULL;
gb_global Type *t_type_info_ptr               = NULL;
gb_global Type *t_type_info_record_ptr        = NULL;
gb_global Type *t_type_info_enum_value_ptr    = NULL;

gb_global Type *t_type_info_named             = NULL;
gb_global Type *t_type_info_integer           = NULL;
gb_global Type *t_type_info_rune              = NULL;
gb_global Type *t_type_info_float             = NULL;
gb_global Type *t_type_info_complex           = NULL;
gb_global Type *t_type_info_any               = NULL;
gb_global Type *t_type_info_string            = NULL;
gb_global Type *t_type_info_boolean           = NULL;
gb_global Type *t_type_info_pointer           = NULL;
gb_global Type *t_type_info_atomic            = NULL;
gb_global Type *t_type_info_procedure         = NULL;
gb_global Type *t_type_info_array             = NULL;
gb_global Type *t_type_info_dynamic_array     = NULL;
gb_global Type *t_type_info_slice             = NULL;
gb_global Type *t_type_info_vector            = NULL;
gb_global Type *t_type_info_tuple             = NULL;
gb_global Type *t_type_info_struct            = NULL;
gb_global Type *t_type_info_raw_union         = NULL;
gb_global Type *t_type_info_union             = NULL;
gb_global Type *t_type_info_enum              = NULL;
gb_global Type *t_type_info_map               = NULL;
gb_global Type *t_type_info_bit_field         = NULL;

gb_global Type *t_type_info_named_ptr         = NULL;
gb_global Type *t_type_info_integer_ptr       = NULL;
gb_global Type *t_type_info_rune_ptr          = NULL;
gb_global Type *t_type_info_float_ptr         = NULL;
gb_global Type *t_type_info_complex_ptr       = NULL;
gb_global Type *t_type_info_quaternion_ptr    = NULL;
gb_global Type *t_type_info_any_ptr           = NULL;
gb_global Type *t_type_info_string_ptr        = NULL;
gb_global Type *t_type_info_boolean_ptr       = NULL;
gb_global Type *t_type_info_pointer_ptr       = NULL;
gb_global Type *t_type_info_atomic_ptr        = NULL;
gb_global Type *t_type_info_procedure_ptr     = NULL;
gb_global Type *t_type_info_array_ptr         = NULL;
gb_global Type *t_type_info_dynamic_array_ptr = NULL;
gb_global Type *t_type_info_slice_ptr         = NULL;
gb_global Type *t_type_info_vector_ptr        = NULL;
gb_global Type *t_type_info_tuple_ptr         = NULL;
gb_global Type *t_type_info_struct_ptr        = NULL;
gb_global Type *t_type_info_raw_union_ptr     = NULL;
gb_global Type *t_type_info_union_ptr         = NULL;
gb_global Type *t_type_info_enum_ptr          = NULL;
gb_global Type *t_type_info_map_ptr           = NULL;
gb_global Type *t_type_info_bit_field_ptr     = NULL;

gb_global Type *t_allocator                   = NULL;
gb_global Type *t_allocator_ptr               = NULL;
gb_global Type *t_context                     = NULL;
gb_global Type *t_context_ptr                 = NULL;

gb_global Type *t_source_code_location        = NULL;
gb_global Type *t_source_code_location_ptr    = NULL;

gb_global Type *t_map_key                     = NULL;
gb_global Type *t_map_header                  = NULL;





i64     type_size_of   (gbAllocator allocator, Type *t);
i64     type_align_of  (gbAllocator allocator, Type *t);
i64     type_offset_of (gbAllocator allocator, Type *t, i32 index);
gbString type_to_string(Type *type);



Type *base_type(Type *t) {
	for (;;) {
		if (t == NULL) {
			break;
		}
		if (t->kind != Type_Named) {
			break;
		}
		if (t == t->Named.base) {
			return t_invalid;
		}
		t = t->Named.base;
	}
	return t;
}

Type *base_enum_type(Type *t) {
	Type *bt = base_type(t);
	if (bt != NULL &&
	    bt->kind == Type_Record &&
	    bt->Record.kind == TypeRecord_Enum) {
		return bt->Record.enum_base_type;
	}
	return t;
}

Type *core_type(Type *t) {
	for (;;) {
		if (t == NULL) {
			break;
		}

		switch (t->kind) {
		case Type_Named:
			if (t == t->Named.base) {
				return t_invalid;
			}
			t = t->Named.base;
			continue;
		case Type_Record:
			if (t->Record.kind == TypeRecord_Enum) {
				t = t->Record.enum_base_type;
				continue;
			}
			break;
		case Type_Atomic:
			t = t->Atomic.elem;
			continue;
		}
		break;
	}
	return t;
}

void set_base_type(Type *t, Type *base) {
	if (t && t->kind == Type_Named) {
		t->Named.base = base;
	}
}


Type *alloc_type(gbAllocator a, TypeKind kind) {
	Type *t = gb_alloc_item(a, Type);
	gb_zero_item(t);
	t->kind = kind;
	return t;
}


Type *make_type_basic(gbAllocator a, BasicType basic) {
	Type *t = alloc_type(a, Type_Basic);
	t->Basic = basic;
	return t;
}

Type *make_type_generic(gbAllocator a, i64 id) {
	Type *t = alloc_type(a, Type_Generic);
	t->Generic.id = id;
	return t;
}

Type *make_type_pointer(gbAllocator a, Type *elem) {
	Type *t = alloc_type(a, Type_Pointer);
	t->Pointer.elem = elem;
	return t;
}

Type *make_type_atomic(gbAllocator a, Type *elem) {
	Type *t = alloc_type(a, Type_Atomic);
	t->Atomic.elem = elem;
	return t;
}

Type *make_type_array(gbAllocator a, Type *elem, i64 count) {
	Type *t = alloc_type(a, Type_Array);
	t->Array.elem = elem;
	t->Array.count = count;
	return t;
}

Type *make_type_dynamic_array(gbAllocator a, Type *elem) {
	Type *t = alloc_type(a, Type_DynamicArray);
	t->DynamicArray.elem = elem;
	return t;
}

Type *make_type_vector(gbAllocator a, Type *elem, i64 count) {
	Type *t = alloc_type(a, Type_Vector);
	t->Vector.elem = elem;
	t->Vector.count = count;
	return t;
}

Type *make_type_slice(gbAllocator a, Type *elem) {
	Type *t = alloc_type(a, Type_Slice);
	t->Array.elem = elem;
	return t;
}


Type *make_type_struct(gbAllocator a) {
	Type *t = alloc_type(a, Type_Record);
	t->Record.kind = TypeRecord_Struct;
	return t;
}

Type *make_type_union(gbAllocator a) {
	Type *t = alloc_type(a, Type_Record);
	t->Record.kind = TypeRecord_Union;
	return t;
}

Type *make_type_raw_union(gbAllocator a) {
	Type *t = alloc_type(a, Type_Record);
	t->Record.kind = TypeRecord_RawUnion;
	return t;
}

Type *make_type_enum(gbAllocator a) {
	Type *t = alloc_type(a, Type_Record);
	t->Record.kind = TypeRecord_Enum;
	return t;
}





Type *make_type_named(gbAllocator a, String name, Type *base, Entity *type_name) {
	Type *t = alloc_type(a, Type_Named);
	t->Named.name = name;
	t->Named.base = base;
	t->Named.type_name = type_name;
	return t;
}

Type *make_type_tuple(gbAllocator a) {
	Type *t = alloc_type(a, Type_Tuple);
	return t;
}

Type *make_type_proc(gbAllocator a, Scope *scope, Type *params, isize param_count, Type *results, isize result_count, bool variadic, ProcCallingConvention calling_convention) {
	Type *t = alloc_type(a, Type_Proc);

	if (variadic) {
		if (param_count == 0) {
			GB_PANIC("variadic procedure must have at least one parameter");
		}
		GB_ASSERT(params != NULL && params->kind == Type_Tuple);
		Entity *e = params->Tuple.variables[param_count-1];
		if (base_type(e->type)->kind != Type_Slice) {
			// NOTE(bill): For custom calling convention
			GB_PANIC("variadic parameter must be of type slice");
		}
	}

	t->Proc.scope        = scope;
	t->Proc.params       = params;
	t->Proc.param_count  = param_count;
	t->Proc.results      = results;
	t->Proc.result_count = result_count;
	t->Proc.variadic     = variadic;
	t->Proc.calling_convention = calling_convention;
	return t;
}

bool is_type_valid_for_keys(Type *t);

Type *make_type_map(gbAllocator a, i64 count, Type *key, Type *value) {
	Type *t = alloc_type(a, Type_Map);
	if (key != NULL) {
		GB_ASSERT(is_type_valid_for_keys(key));
	}
	t->Map.count = count;
	t->Map.key   = key;
	t->Map.value = value;
	return t;
}

Type *make_type_bit_field_value(gbAllocator a, u32 bits) {
	Type *t = alloc_type(a, Type_BitFieldValue);
	t->BitFieldValue.bits = bits;
	return t;
}

Type *make_type_bit_field(gbAllocator a) {
	Type *t = alloc_type(a, Type_BitField);
	return t;
}



////////////////////////////////////////////////////////////////


Type *type_deref(Type *t) {
	if (t != NULL) {
		Type *bt = base_type(t);
		if (bt == NULL)
			return NULL;
		if (bt != NULL && bt->kind == Type_Pointer)
			return bt->Pointer.elem;
	}
	return t;
}

bool is_type_named(Type *t) {
	if (t->kind == Type_Basic) {
		return true;
	}
	return t->kind == Type_Named;
}
bool is_type_named_alias(Type *t) {
	if (!is_type_named(t)) {
		return false;
	}
	Entity *e = t->Named.type_name;
	if (e == NULL) {
		return false;
	}
	if (e->kind != Entity_TypeName) {
		return false;
	}
	return e->TypeName.is_type_alias;
}

bool is_type_boolean(Type *t) {
	t = core_type(t);
	if (t->kind == Type_Basic) {
		return (t->Basic.flags & BasicFlag_Boolean) != 0;
	}
	return false;
}
bool is_type_integer(Type *t) {
	t = core_type(t);
	if (t->kind == Type_Basic) {
		return (t->Basic.flags & BasicFlag_Integer) != 0;
	}
	return false;
}
bool is_type_unsigned(Type *t) {
	t = core_type(t);
	if (t->kind == Type_Basic) {
		return (t->Basic.flags & BasicFlag_Unsigned) != 0;
	}
	return false;
}
bool is_type_rune(Type *t) {
	t = core_type(t);
	if (t->kind == Type_Basic) {
		return (t->Basic.flags & BasicFlag_Rune) != 0;
	}
	return false;
}
bool is_type_numeric(Type *t) {
	t = core_type(t);
	if (t->kind == Type_Basic) {
		return (t->Basic.flags & BasicFlag_Numeric) != 0;
	}
	// TODO(bill): Should this be here?
	if (t->kind == Type_Vector) {
		return is_type_numeric(t->Vector.elem);
	}
	return false;
}
bool is_type_string(Type *t) {
	t = base_type(t);
	if (t->kind == Type_Basic) {
		return (t->Basic.flags & BasicFlag_String) != 0;
	}
	return false;
}
bool is_type_typed(Type *t) {
	t = base_type(t);
	if (t == NULL) {
		return false;
	}
	if (t->kind == Type_Basic) {
		return (t->Basic.flags & BasicFlag_Untyped) == 0;
	}
	return true;
}
bool is_type_untyped(Type *t) {
	t = base_type(t);
	if (t->kind == Type_Basic) {
		return (t->Basic.flags & BasicFlag_Untyped) != 0;
	}
	return false;
}
bool is_type_ordered(Type *t) {
	t = core_type(t);
	switch (t->kind) {
	case Type_Basic:
		return (t->Basic.flags & BasicFlag_Ordered) != 0;
	case Type_Pointer:
		return true;
	case Type_Vector:
		return is_type_ordered(t->Vector.elem);
	}
	return false;
}
bool is_type_constant_type(Type *t) {
	t = core_type(t);
	if (t->kind == Type_Basic) {
		return (t->Basic.flags & BasicFlag_ConstantType) != 0;
	}
	return false;
}
bool is_type_float(Type *t) {
	t = core_type(t);
	if (t->kind == Type_Basic) {
		return (t->Basic.flags & BasicFlag_Float) != 0;
	}
	return false;
}
bool is_type_complex(Type *t) {
	t = core_type(t);
	if (t->kind == Type_Basic) {
		return (t->Basic.flags & BasicFlag_Complex) != 0;
	}
	return false;
}
bool is_type_f32(Type *t) {
	t = core_type(t);
	if (t->kind == Type_Basic) {
		return t->Basic.kind == Basic_f32;
	}
	return false;
}
bool is_type_f64(Type *t) {
	t = core_type(t);
	if (t->kind == Type_Basic) {
		return t->Basic.kind == Basic_f64;
	}
	return false;
}
bool is_type_pointer(Type *t) {
	t = base_type(t);
	if (t->kind == Type_Basic) {
		return (t->Basic.flags & BasicFlag_Pointer) != 0;
	}
	return t->kind == Type_Pointer;
}
bool is_type_atomic(Type *t) {
	t = base_type(t);
	return t->kind == Type_Atomic;
}
bool is_type_tuple(Type *t) {
	t = base_type(t);
	return t->kind == Type_Tuple;
}


bool is_type_int_or_uint(Type *t) {
	if (t->kind == Type_Basic) {
		return (t->Basic.kind == Basic_int) || (t->Basic.kind == Basic_uint);
	}
	return false;
}
bool is_type_i128_or_u128(Type *t) {
	if (t->kind == Type_Basic) {
		return (t->Basic.kind == Basic_i128) || (t->Basic.kind == Basic_u128);
	}
	return false;
}
bool is_type_rawptr(Type *t) {
	if (t->kind == Type_Basic) {
		return t->Basic.kind == Basic_rawptr;
	}
	return false;
}
bool is_type_u8(Type *t) {
	if (t->kind == Type_Basic) {
		return t->Basic.kind == Basic_u8;
	}
	return false;
}
bool is_type_array(Type *t) {
	t = base_type(t);
	return t->kind == Type_Array;
}
bool is_type_dynamic_array(Type *t) {
	t = base_type(t);
	return t->kind == Type_DynamicArray;
}
bool is_type_slice(Type *t) {
	t = base_type(t);
	return t->kind == Type_Slice;
}
bool is_type_u8_slice(Type *t) {
	t = base_type(t);
	if (t->kind == Type_Slice) {
		return is_type_u8(t->Slice.elem);
	}
	return false;
}
bool is_type_vector(Type *t) {
	t = base_type(t);
	return t->kind == Type_Vector;
}
bool is_type_proc(Type *t) {
	t = base_type(t);
	return t->kind == Type_Proc;
}
bool is_type_gen_proc(Type *t) {
	t = base_type(t);
	return t->kind == Type_Proc && t->Proc.is_generic;
}
Type *base_vector_type(Type *t) {
	if (is_type_vector(t)) {
		t = base_type(t);
		return t->Vector.elem;
	}
	return t;
}

Type *base_complex_elem_type(Type *t) {
	t = core_type(t);
	if (is_type_complex(t)) {
		switch (t->Basic.kind) {
		// case Basic_complex32:      return t_f16;
		case Basic_complex64:      return t_f32;
		case Basic_complex128:     return t_f64;
		case Basic_UntypedComplex: return t_untyped_float;
		}
	}
	GB_PANIC("Invalid complex type");
	return t_invalid;
}

bool is_type_struct(Type *t) {
	t = base_type(t);
	return (t->kind == Type_Record && t->Record.kind == TypeRecord_Struct);
}
bool is_type_union(Type *t) {
	t = base_type(t);
	return (t->kind == Type_Record && t->Record.kind == TypeRecord_Union);
}
bool is_type_variant(Type *t) {
	t = base_type(t);
	if (t->kind == Type_Record) {
		return t->Record.kind == TypeRecord_Struct && t->Record.variant_parent != NULL;
	}
	return false;
}

bool is_type_raw_union(Type *t) {
	t = base_type(t);
	return (t->kind == Type_Record && t->Record.kind == TypeRecord_RawUnion);
}
bool is_type_enum(Type *t) {
	t = base_type(t);
	return (t->kind == Type_Record && t->Record.kind == TypeRecord_Enum);
}
bool is_type_bit_field(Type *t) {
	t = base_type(t);
	return (t->kind == Type_BitField);
}
bool is_type_bit_field_value(Type *t) {
	t = base_type(t);
	return (t->kind == Type_BitFieldValue);
}
bool is_type_map(Type *t) {
	t = base_type(t);
	return t->kind == Type_Map;
}

bool is_type_fixed_map(Type *t) {
	t = base_type(t);
	return t->kind == Type_Map && t->Map.count > 0;
}
bool is_type_dynamic_map(Type *t) {
	t = base_type(t);	return t->kind == Type_Map && t->Map.count == 0;
}




bool is_type_any(Type *t) {
	t = base_type(t);
	return (t->kind == Type_Basic && t->Basic.kind == Basic_any);
}
bool is_type_untyped_nil(Type *t) {
	t = base_type(t);
	return (t->kind == Type_Basic && t->Basic.kind == Basic_UntypedNil);
}
bool is_type_untyped_undef(Type *t) {
	t = base_type(t);
	return (t->kind == Type_Basic && t->Basic.kind == Basic_UntypedUndef);
}



bool is_type_valid_for_keys(Type *t) {
	t = core_type(t);
	if (is_type_untyped(t)) {
		return false;
	}
	if (is_type_integer(t)) {
		return true;
	}
	if (is_type_float(t)) {
		return true;
	}
	if (is_type_string(t)) {
		return true;
	}
	if (is_type_pointer(t)) {
		return true;
	}

	return false;
}


bool is_type_indexable(Type *t) {
	return is_type_array(t) || is_type_slice(t) || is_type_vector(t) || is_type_string(t);
}

bool is_type_generic(Type *t) {
	t = core_type(t);
	switch (t->kind) {
	case Type_Generic:
		return true;

	case Type_Pointer:
		return is_type_generic(t->Pointer.elem);
	case Type_Atomic:
		return is_type_generic(t->Atomic.elem);
	case Type_Array:
		return is_type_generic(t->Array.elem);
	case Type_DynamicArray:
		return is_type_generic(t->DynamicArray.elem);
	case Type_Vector:
		return is_type_generic(t->Vector.elem);
	case Type_Slice:
		return is_type_generic(t->Slice.elem);

	case Type_Tuple:
		for (isize i = 0; i < t->Tuple.variable_count; i++) {
			if (is_type_generic(t->Tuple.variables[i]->type)) {
				return true;
			}
		}
		break;

	case Type_Proc:
		if (t->Proc.param_count > 0 &&
		    is_type_generic(t->Proc.params)) {
			return true;
		}
		if (t->Proc.result_count > 0 &&
		    is_type_generic(t->Proc.results)) {
			return true;
		}
		break;

	// case Type_Record:
	// 	GB_ASSERT(t->Record.kind != TypeRecord_Enum);
	// 	for (isize i = 0; i < t->Record.field_count; i++) {
	// 	    if (is_type_generic(t->Record.fields[i]->type)) {
	// 	    	return true;
	// 	    }
	// 	}
	// 	for (isize i = 1; i < t->Record.variant_count; i++) {
	// 	    if (is_type_generic(t->Record.variants[i]->type)) {
	// 	    	return true;
	// 	    }
	// 	}
	// 	break;

	case Type_Map:
		if (is_type_generic(t->Map.key)) {
			return true;
		}
		if (is_type_generic(t->Map.value)) {
			return true;
		}
		break;

	}

	return false;
}


bool type_has_undef(Type *t) {
	t = base_type(t);
	return true;
}

bool type_has_nil(Type *t) {
	t = base_type(t);
	switch (t->kind) {
	case Type_Basic: {
		switch (t->Basic.kind) {
		case Basic_rawptr:
		case Basic_any:
			return true;
		}
		return false;
	} break;
	case Type_Slice:
	case Type_Proc:
	case Type_Pointer:
	case Type_DynamicArray:
	case Type_Map:
		return true;
	case Type_Record:
		switch (t->Record.kind) {
		case TypeRecord_Union:
			return true;
		}
		return false;
	}
	return false;
}


bool is_type_comparable(Type *t) {
	t = base_type(t);
	switch (t->kind) {
	case Type_Basic:
		switch (t->Basic.kind) {
		case Basic_UntypedNil:
		case Basic_any:
			return false;
		case Basic_rune:
			return true;
		}
		return true;
	case Type_Pointer:
		return true;
	case Type_Record: {
		if (is_type_enum(t)) {
			return is_type_comparable(core_type(t));
		}
		return false;
	} break;
	case Type_Array:
		return false;
	case Type_Vector:
		return is_type_comparable(t->Vector.elem);
	case Type_Proc:
		return true;
	}
	return false;
}

bool are_types_identical(Type *x, Type *y) {
	if (x == y) {
		return true;
	}

	if ((x == NULL && y != NULL) ||
	    (x != NULL && y == NULL)) {
		return false;
	}

	switch (x->kind) {
	case Type_Generic:
		if (y->kind == Type_Generic) {
			return true; // TODO(bill): Is this correct?
		}
		break;

	case Type_Basic:
		if (y->kind == Type_Basic) {
			return x->Basic.kind == y->Basic.kind;
		}
		break;

	case Type_Array:
		if (y->kind == Type_Array) {
			return (x->Array.count == y->Array.count) && are_types_identical(x->Array.elem, y->Array.elem);
		}
		break;

	case Type_DynamicArray:
		if (y->kind == Type_DynamicArray) {
			return are_types_identical(x->DynamicArray.elem, y->DynamicArray.elem);
		}
		break;

	case Type_Vector:
		if (y->kind == Type_Vector) {
			return (x->Vector.count == y->Vector.count) && are_types_identical(x->Vector.elem, y->Vector.elem);
		}
		break;

	case Type_Slice:
		if (y->kind == Type_Slice) {
			return are_types_identical(x->Slice.elem, y->Slice.elem);
		}
		break;

	case Type_Record:
		if (y->kind == Type_Record) {
			if (x->Record.kind == y->Record.kind) {
				switch (x->Record.kind) {
				case TypeRecord_Struct:
				case TypeRecord_RawUnion:
				case TypeRecord_Union:
					if (x->Record.field_count == y->Record.field_count &&
					    x->Record.variant_count == y->Record.variant_count &&
					    x->Record.is_packed == y->Record.is_packed &&
					    x->Record.is_ordered == y->Record.is_ordered &&
					    x->Record.custom_align == y->Record.custom_align) {
						// TODO(bill); Fix the custom alignment rule
						for (isize i = 0; i < x->Record.field_count; i++) {
							Entity *xf = x->Record.fields[i];
							Entity *yf = y->Record.fields[i];
							if (!are_types_identical(xf->type, yf->type)) {
								return false;
							}
							if (xf->token.string != yf->token.string) {
								return false;
							}
							bool xf_is_using = (xf->flags&EntityFlag_Using) != 0;
							bool yf_is_using = (yf->flags&EntityFlag_Using) != 0;
							if (xf_is_using ^ yf_is_using) {
								return false;
							}
						}
						// NOTE(bill): zeroth variant is NULL
						for (isize i = 1; i < x->Record.variant_count; i++) {
							if (!are_types_identical(x->Record.variants[i]->type, y->Record.variants[i]->type)) {
								return false;
							}
							if (x->Record.variants[i]->token.string != y->Record.variants[i]->token.string) {
								return false;
							}
						}
						return true;
					}
					break;

				case TypeRecord_Enum:
					return x == y; // NOTE(bill): All enums are unique
				}
			}
		}
		break;

	case Type_Pointer:
		if (y->kind == Type_Pointer) {
			return are_types_identical(x->Pointer.elem, y->Pointer.elem);
		}
		break;

	case Type_Named:
		if (y->kind == Type_Named) {
			return x->Named.type_name == y->Named.type_name;
		}
		break;

	case Type_Tuple:
		if (y->kind == Type_Tuple) {
			if (x->Tuple.variable_count == y->Tuple.variable_count) {
				for (isize i = 0; i < x->Tuple.variable_count; i++) {
					Entity *xe = x->Tuple.variables[i];
					Entity *ye = y->Tuple.variables[i];
					if (xe->kind != ye->kind || !are_types_identical(xe->type, ye->type)) {
						return false;
					}
				}
				return true;
			}
		}
		break;

	case Type_Proc:
		if (y->kind == Type_Proc) {
			return x->Proc.calling_convention == y->Proc.calling_convention &&
			       x->Proc.c_vararg == y->Proc.c_vararg &&
			       x->Proc.variadic == y->Proc.variadic &&
			       are_types_identical(x->Proc.params, y->Proc.params) &&
			       are_types_identical(x->Proc.results, y->Proc.results);
		}
		break;

	case Type_Map:
		if (y->kind == Type_Map) {
			return x->Map.count == y->Map.count &&
			       are_types_identical(x->Map.key,   y->Map.key) &&
			       are_types_identical(x->Map.value, y->Map.value);
		}
		break;
	}

	return false;
}

Type *default_bit_field_value_type(Type *type) {
	if (type == NULL) {
		return t_invalid;
	}
	Type *t = base_type(type);
	if (t->kind == Type_BitFieldValue) {
		i32 bits = t->BitFieldValue.bits;
		i32 size = 8*next_pow2((bits+7)/8);
		switch (size) {
		case 8:   return t_u8;
		case 16:  return t_u16;
		case 32:  return t_u32;
		case 64:  return t_u64;
		case 128: return t_u128;
		default:  GB_PANIC("Too big of a bit size!"); break;
		}
	}
	return type;
}

Type *default_type(Type *type) {
	if (type == NULL) {
		return t_invalid;
	}
	if (type->kind == Type_Basic) {
		switch (type->Basic.kind) {
		case Basic_UntypedBool:       return t_bool;
		case Basic_UntypedInteger:    return t_int;
		case Basic_UntypedFloat:      return t_f64;
		case Basic_UntypedComplex:    return t_complex128;
		case Basic_UntypedString:     return t_string;
		case Basic_UntypedRune:       return t_rune;
		}
	}
	if (type->kind == Type_BitFieldValue) {
		return default_bit_field_value_type(type);
	}
	return type;
}


// NOTE(bill): Valid Compile time execution #run type
bool is_type_cte_safe(Type *type) {
	type = default_type(base_type(type));
	switch (type->kind) {
	case Type_Basic:
		switch (type->Basic.kind) {
		case Basic_rawptr:
		case Basic_any:
			return false;
		}
		return true;

	case Type_Pointer:
		return false;

	case Type_Array:
		return is_type_cte_safe(type->Array.elem);

	case Type_DynamicArray:
		return false;
	case Type_Map:
		return false;

	case Type_Vector: // NOTE(bill): This should always to be true but this is for sanity reasons
		return is_type_cte_safe(type->Vector.elem);

	case Type_Slice:
		return false;

	case Type_Record: {
		if (type->Record.kind != TypeRecord_Struct) {
			return false;
		}
		for (isize i = 0; i < type->Record.field_count; i++) {
			Entity *v = type->Record.fields[i];
			if (!is_type_cte_safe(v->type)) {
				return false;
			}
		}
		return true;
	}

	case Type_Tuple: {
		for (isize i = 0; i < type->Tuple.variable_count; i++) {
			Entity *v = type->Tuple.variables[i];
			if (!is_type_cte_safe(v->type)) {
				return false;
			}
		}
		return true;
	}

	case Type_Proc:
		// TODO(bill): How should I handle procedures in the CTE stage?
		// return type->Proc.calling_convention == ProcCC_Odin;
		return false;
	}

	return false;
}

enum ProcTypeOverloadKind {
	ProcOverload_Identical, // The types are identical

	ProcOverload_CallingConvention,
	ProcOverload_ParamCount,
	ProcOverload_ParamVariadic,
	ProcOverload_ParamTypes,
	ProcOverload_ResultCount,
	ProcOverload_ResultTypes,
	ProcOverload_Polymorphic,

	ProcOverload_NotProcedure,

};

ProcTypeOverloadKind are_proc_types_overload_safe(Type *x, Type *y) {
	if (x == NULL && y == NULL) return ProcOverload_NotProcedure;
	if (x == NULL && y != NULL) return ProcOverload_NotProcedure;
	if (x != NULL && y == NULL) return ProcOverload_NotProcedure;
 	if (!is_type_proc(x))       return ProcOverload_NotProcedure;
 	if (!is_type_proc(y))       return ProcOverload_NotProcedure;

	TypeProc px = base_type(x)->Proc;
	TypeProc py = base_type(y)->Proc;

	// if (px.calling_convention != py.calling_convention) {
		// return ProcOverload_CallingConvention;
	// }

	if (px.is_generic != py.is_generic) {
		return ProcOverload_Polymorphic;
	}

	if (px.param_count != py.param_count) {
		return ProcOverload_ParamCount;
	}

	for (isize i = 0; i < px.param_count; i++) {
		Entity *ex = px.params->Tuple.variables[i];
		Entity *ey = py.params->Tuple.variables[i];
		if (!are_types_identical(ex->type, ey->type)) {
			return ProcOverload_ParamTypes;
		}
	}
	// IMPORTANT TODO(bill): Determine the rules for overloading procedures with variadic parameters
	if (px.variadic != py.variadic) {
		return ProcOverload_ParamVariadic;
	}

	if (px.result_count != py.result_count) {
		return ProcOverload_ResultCount;
	}

	for (isize i = 0; i < px.result_count; i++) {
		Entity *ex = px.results->Tuple.variables[i];
		Entity *ey = py.results->Tuple.variables[i];
		if (!are_types_identical(ex->type, ey->type)) {
			return ProcOverload_ResultTypes;
		}
	}

	if (px.params != NULL && py.params != NULL) {
		Entity *ex = px.params->Tuple.variables[0];
		Entity *ey = py.params->Tuple.variables[0];
		bool ok = are_types_identical(ex->type, ey->type);
		if (ok) {
		}
	}

	return ProcOverload_Identical;
}





Selection lookup_field_with_selection(gbAllocator a, Type *type_, String field_name, bool is_type, Selection sel);

Selection lookup_field(gbAllocator a, Type *type_, String field_name, bool is_type) {
	return lookup_field_with_selection(a, type_, field_name, is_type, empty_selection);
}

Selection lookup_field_from_index(gbAllocator a, Type *type, i64 index) {
	GB_ASSERT(is_type_struct(type) || is_type_union(type) || is_type_tuple(type));
	type = base_type(type);

	i64 max_count = 0;
	switch (type->kind) {
	case Type_Record:   max_count = type->Record.field_count;   break;
	case Type_Tuple:    max_count = type->Tuple.variable_count; break;
	case Type_BitField: max_count = type->BitField.field_count; break;
	}

	if (index >= max_count) {
		return empty_selection;
	}

	switch (type->kind) {
	case Type_Record:
		for (isize i = 0; i < max_count; i++) {
			Entity *f = type->Record.fields[i];
			if (f->kind == Entity_Variable) {
				if (f->Variable.field_src_index == index) {
					Array<i32> sel_array = {0};
					array_init_count(&sel_array, a, 1);
					sel_array[0] = i;
					return make_selection(f, sel_array, false);
				}
			}
		}
		break;
	case Type_Tuple:
		for (isize i = 0; i < max_count; i++) {
			Entity *f = type->Tuple.variables[i];
			if (i == index) {
				Array<i32> sel_array = {0};
				array_init_count(&sel_array, a, 1);
				sel_array[0] = i;
				return make_selection(f, sel_array, false);
			}
		}
		break;

	case Type_BitField: {
		Array<i32> sel_array = {0};
		array_init_count(&sel_array, a, 1);
		sel_array[0] = cast(i32)index;
		return make_selection(type->BitField.fields[index], sel_array, false);
	} break;

	}

	GB_PANIC("Illegal index");
	return empty_selection;
}


gb_global Entity *entity__any_data       = NULL;
gb_global Entity *entity__any_type_info  = NULL;

Selection lookup_field_with_selection(gbAllocator a, Type *type_, String field_name, bool is_type, Selection sel) {
	GB_ASSERT(type_ != NULL);

	if (field_name == "_") {
		return empty_selection;
	}

	Type *type = type_deref(type_);
	bool is_ptr = type != type_;
	sel.indirect = sel.indirect || is_ptr;

	type = base_type(type);

	if (type->kind == Type_Basic) {
		switch (type->Basic.kind) {
		case Basic_any: {
		#if 1
			// IMPORTANT TODO(bill): Should these members be available to should I only allow them with
			// `Raw_Any` type?
			String data_str = str_lit("data");
			String type_info_str = str_lit("type_info");
			if (entity__any_data == NULL) {
				entity__any_data = make_entity_field(a, NULL, make_token_ident(data_str), t_rawptr, false, 0);
			}
			if (entity__any_type_info == NULL) {
				entity__any_type_info = make_entity_field(a, NULL, make_token_ident(type_info_str), t_type_info_ptr, false, 1);
			}

			if (field_name == data_str) {
				selection_add_index(&sel, 0);
				sel.entity = entity__any_data;;
				return sel;
			} else if (field_name == type_info_str) {
				selection_add_index(&sel, 1);
				sel.entity = entity__any_type_info;
				return sel;
			}
		#endif
		} break;
		}

		return sel;
	} else if (type->kind == Type_Vector) {
		if (type->Vector.count <= 4 && !is_type_boolean(type->Vector.elem)) {
			// HACK(bill): Memory leak
			switch (type->Vector.count) {
			#define _VECTOR_FIELD_CASE(_length, _name) \
			case (_length): \
				if (field_name == _name) { \
					selection_add_index(&sel, (_length)-1); \
					sel.entity = make_entity_vector_elem(a, NULL, make_token_ident(str_lit(_name)), type->Vector.elem, (_length)-1); \
					return sel; \
				} \
				/*fallthrough*/

			_VECTOR_FIELD_CASE(4, "w");
			_VECTOR_FIELD_CASE(3, "z");
			_VECTOR_FIELD_CASE(2, "y");
			_VECTOR_FIELD_CASE(1, "x");
			default: break;

			#undef _VECTOR_FIELD_CASE
			}
		}
	}

	if (is_type) {
		if (type->kind == Type_Record) {
			if (type->Record.names != NULL &&
			    field_name == "names") {
				sel.entity = type->Record.names;
				return sel;
			}
		}

		if (is_type_union(type)) {
			for (isize i = 1; i < type->Record.variant_count; i++) {
				Entity *f = type->Record.variants[i];
				GB_ASSERT(f->kind == Entity_TypeName);
				String str = f->token.string;

				if (str == field_name) {
					sel.entity = f;
					// selection_add_index(&sel, i);
					return sel;
				}
			}
		} else if (is_type_enum(type)) {
			// NOTE(bill): These may not have been added yet, so check in case
			if (type->Record.enum_count != NULL) {
				if (field_name == "count") {
					sel.entity = type->Record.enum_count;
					return sel;
				}
				if (field_name == "min_value") {
					sel.entity = type->Record.enum_min_value;
					return sel;
				}
				if (field_name == "max_value") {
					sel.entity = type->Record.enum_max_value;
					return sel;
				}
			}

			for (isize i = 0; i < type->Record.field_count; i++) {
				Entity *f = type->Record.fields[i];
				GB_ASSERT(f->kind == Entity_Constant);
				String str = f->token.string;

				if (field_name == str) {
					sel.entity = f;
					// selection_add_index(&sel, i);
					return sel;
				}
			}
		}
	} else if (type->kind == Type_Record) {
		for (isize i = 0; i < type->Record.field_count; i++) {
			Entity *f = type->Record.fields[i];
			if (f->kind != Entity_Variable || (f->flags & EntityFlag_Field) == 0) {
				continue;
			}
			String str = f->token.string;
			if (field_name == str) {
				selection_add_index(&sel, i);  // HACK(bill): Leaky memory
				sel.entity = f;
				return sel;
			}

			if (f->flags & EntityFlag_Using) {
				isize prev_count = sel.index.count;
				selection_add_index(&sel, i); // HACK(bill): Leaky memory

				sel = lookup_field_with_selection(a, f->type, field_name, is_type, sel);

				if (sel.entity != NULL) {
					if (is_type_pointer(f->type)) {
						sel.indirect = true;
					}
					return sel;
				}
				sel.index.count = prev_count;
			}
		}
		if (type->Record.kind == TypeRecord_Union) {
			if (field_name == "__tag") {
				Entity *e = type->Record.union__tag;
				GB_ASSERT(e != NULL);
				selection_add_index(&sel, -1); // HACK(bill): Leaky memory
				sel.entity = e;
				return sel;
			}
		}
	} else if (type->kind == Type_BitField) {
		for (isize i = 0; i < type->BitField.field_count; i++) {
			Entity *f = type->BitField.fields[i];
			if (f->kind != Entity_Variable ||
			    (f->flags & EntityFlag_BitFieldValue) == 0) {
				continue;
			}

			String str = f->token.string;
			if (field_name == str) {
				selection_add_index(&sel, i);  // HACK(bill): Leaky memory
				sel.entity = f;
				return sel;
			}
		}
	}

	return sel;
}


struct TypePath {
	Array<Type *> path; // Entity_TypeName;
	bool failure;
};

void type_path_init(TypePath *tp) {
	// TODO(bill): Use an allocator that uses a backing array if it can and then use alternative allocator when exhausted
	array_init(&tp->path, heap_allocator());
}

void type_path_free(TypePath *tp) {
	array_free(&tp->path);
}

void type_path_print_illegal_cycle(TypePath *tp, isize start_index) {
	GB_ASSERT(tp != NULL);

	GB_ASSERT(start_index < tp->path.count);
	Type *t = tp->path[start_index];
	GB_ASSERT(t != NULL);

	GB_ASSERT_MSG(is_type_named(t), "%s", type_to_string(t));
	Entity *e = t->Named.type_name;
	error(e->token, "Illegal declaration cycle of `%.*s`", LIT(t->Named.name));
	// NOTE(bill): Print cycle, if it's deep enough
	for (isize j = start_index; j < tp->path.count; j++) {
		Type *t = tp->path[j];
		GB_ASSERT_MSG(is_type_named(t), "%s", type_to_string(t));
		Entity *e = t->Named.type_name;
		error(e->token, "\t%.*s refers to", LIT(t->Named.name));
	}
	// NOTE(bill): This will only print if the path count > 1
	error(e->token, "\t%.*s", LIT(t->Named.name));
	tp->failure = true;
	t->failure = true;
}

TypePath *type_path_push(TypePath *tp, Type *t) {
	GB_ASSERT(tp != NULL);

	for (isize i = 0; i < tp->path.count; i++) {
		if (tp->path[i] == t) {
			type_path_print_illegal_cycle(tp, i);
		}
	}

	if (!tp->failure && is_type_named(t)) {
		array_add(&tp->path, t);
	}
	return tp;
}

void type_path_pop(TypePath *tp) {
	if (tp != NULL && tp->path.count > 0) {
		array_pop(&tp->path);
	}
}


#define FAILURE_SIZE      0
#define FAILURE_ALIGNMENT 0


i64 type_size_of_internal (gbAllocator allocator, Type *t, TypePath *path);
i64 type_align_of_internal(gbAllocator allocator, Type *t, TypePath *path);

i64 align_formula(i64 size, i64 align) {
	if (align > 0) {
		i64 result = size + align-1;
		return result - result%align;
	}
	return size;
}

i64 type_size_of(gbAllocator allocator, Type *t) {
	if (t == NULL) {
		return 0;
	}
	i64 size;
	TypePath path = {0};
	type_path_init(&path);
	size = type_size_of_internal(allocator, t, &path);
	type_path_free(&path);
	return size;
}

i64 type_align_of(gbAllocator allocator, Type *t) {
	if (t == NULL) {
		return 1;
	}
	i64 align;
	TypePath path = {0};
	type_path_init(&path);
	align = type_align_of_internal(allocator, t, &path);
	type_path_free(&path);
	return align;
}


i64 type_align_of_internal(gbAllocator allocator, Type *t, TypePath *path) {
	if (t->failure) {
		return FAILURE_ALIGNMENT;
	}

	t = base_type(t);

	switch (t->kind) {
	case Type_Basic: {
		GB_ASSERT(is_type_typed(t));
		switch (t->Basic.kind) {
		case Basic_string: return build_context.word_size;
		case Basic_any:    return build_context.word_size;

		case Basic_int: case Basic_uint: case Basic_rawptr:
			return build_context.word_size;

		case Basic_complex64: case Basic_complex128:
			return type_size_of_internal(allocator, t, path) / 2;
		}
	} break;

	case Type_Array: {
		Type *elem = t->Array.elem;
		type_path_push(path, elem);
		if (path->failure) {
			return FAILURE_ALIGNMENT;
		}
		i64 align = type_align_of_internal(allocator, t->Array.elem, path);
		type_path_pop(path);
		return align;
	}

	case Type_DynamicArray:
		// data, count, capacity, allocator
		return build_context.word_size;

	case Type_Slice:
		return build_context.word_size;

	case Type_Vector: {
		Type *elem = t->Vector.elem;
		type_path_push(path, elem);
		if (path->failure) {
			return FAILURE_ALIGNMENT;
		}
		i64 size = type_size_of_internal(allocator, t->Vector.elem, path);
		type_path_pop(path);
		i64 count = gb_max(prev_pow2(t->Vector.count), 1);
		i64 total = size * count;
		return gb_clamp(total, 1, build_context.max_align);
	} break;

	case Type_Tuple: {
		i64 max = 1;
		for (isize i = 0; i < t->Tuple.variable_count; i++) {
			i64 align = type_align_of_internal(allocator, t->Tuple.variables[i]->type, path);
			if (max < align) {
				max = align;
			}
		}
		return max;
	} break;

	case Type_Map: {
		if (t->Map.count == 0) { // Dynamic
			return type_align_of_internal(allocator, t->Map.generated_struct_type, path);
		}
		GB_PANIC("TODO(bill): Fixed map alignment");
	} break;

	case Type_Record: {
		switch (t->Record.kind) {
		case TypeRecord_Enum:
			return type_align_of_internal(allocator, t->Record.enum_base_type, path);
		case TypeRecord_Struct:
			if (t->Record.custom_align > 0) {
				return gb_clamp(t->Record.custom_align, 1, build_context.max_align);
			}
			if (t->Record.field_count > 0) {
				i64 max = 1;
				if (t->Record.is_packed) {
					max = build_context.word_size;
				}
				for (isize i = 0; i < t->Record.field_count; i++) {
					Type *field_type = t->Record.fields[i]->type;
					type_path_push(path, field_type);
					if (path->failure) {
						return FAILURE_ALIGNMENT;
					}
					i64 align = type_align_of_internal(allocator, field_type, path);
					type_path_pop(path);
					if (max < align) {
						max = align;
					}
				}
				return max;
			}
			break;
		case TypeRecord_Union: {
			i64 max = 1;
			if (t->Record.field_count > 0) {
				Type *field_type = t->Record.fields[0]->type;
				type_path_push(path, field_type);
				i64 align = type_align_of_internal(allocator, field_type, path);
				if (path->failure) {
					return FAILURE_ALIGNMENT;
				}
				type_path_pop(path);
				if (max < align) {
					max = align;
				}
			}
			// NOTE(bill): field zero is null
			for (isize i = 1; i < t->Record.variant_count; i++) {
				Type *variant = t->Record.variants[i]->type;
				type_path_push(path, variant);
				if (path->failure) {
					return FAILURE_ALIGNMENT;
				}
				i64 align = type_align_of_internal(allocator, variant, path);
				type_path_pop(path);
				if (max < align) {
					max = align;
				}
			}
			return max;
		} break;
		case TypeRecord_RawUnion: {
			i64 max = 1;
			for (isize i = 0; i < t->Record.field_count; i++) {
				Type *field_type = t->Record.fields[i]->type;
				type_path_push(path, field_type);
				if (path->failure) {
					return FAILURE_ALIGNMENT;
				}
				i64 align = type_align_of_internal(allocator, field_type, path);
				type_path_pop(path);
				if (max < align) {
					max = align;
				}
			}
			return max;
		} break;
		}
	} break;

	case Type_BitField: {
		i64 align = 1;
		if (t->BitField.custom_align > 0) {
			align = t->BitField.custom_align;
		}
		return gb_clamp(next_pow2(align), 1, build_context.max_align);
	} break;
	}

	// return gb_clamp(next_pow2(type_size_of(allocator, t)), 1, build_context.max_align);
	// NOTE(bill): Things that are bigger than build_context.word_size, are actually comprised of smaller types
	// TODO(bill): Is this correct for 128-bit types (integers)?
	return gb_clamp(next_pow2(type_size_of_internal(allocator, t, path)), 1, build_context.word_size);
}

i64 *type_set_offsets_of(gbAllocator allocator, Entity **fields, isize field_count, bool is_packed) {
	i64 *offsets = gb_alloc_array(allocator, i64, field_count);
	i64 curr_offset = 0;
	if (is_packed) {
		for (isize i = 0; i < field_count; i++) {
			i64 size = type_size_of(allocator, fields[i]->type);
			offsets[i] = curr_offset;
			curr_offset += size;
		}
	} else {
		for (isize i = 0; i < field_count; i++) {
			i64 align = gb_max(type_align_of(allocator, fields[i]->type), 1);
			i64 size  = gb_max(type_size_of(allocator, fields[i]->type), 0);
			curr_offset = align_formula(curr_offset, align);
			offsets[i] = curr_offset;
			curr_offset += size;
		}
	}
	return offsets;
}

bool type_set_offsets(gbAllocator allocator, Type *t) {
	t = base_type(t);
	if (is_type_struct(t)) {
		if (!t->Record.are_offsets_set) {
			t->Record.are_offsets_being_processed = true;
			t->Record.offsets = type_set_offsets_of(allocator, t->Record.fields, t->Record.field_count, t->Record.is_packed);
			t->Record.are_offsets_set = true;
			return true;
		}
	} else if (is_type_union(t)) {
		if (!t->Record.are_offsets_set) {
			t->Record.are_offsets_being_processed = true;
			t->Record.offsets = type_set_offsets_of(allocator, t->Record.fields, t->Record.field_count, false);
			t->Record.are_offsets_set = true;
			return true;
		}
	}  else if (is_type_tuple(t)) {
		if (!t->Tuple.are_offsets_set) {
			t->Record.are_offsets_being_processed = true;
			t->Tuple.offsets = type_set_offsets_of(allocator, t->Tuple.variables, t->Tuple.variable_count, false);
			t->Tuple.are_offsets_set = true;
			return true;
		}
	} else {
		GB_PANIC("Invalid type for setting offsets");
	}
	return false;
}

i64 type_size_of_internal(gbAllocator allocator, Type *t, TypePath *path) {
	if (t->failure) {
		return FAILURE_SIZE;
	}

	switch (t->kind) {
	case Type_Named: {
		type_path_push(path, t);
		if (path->failure) {
			return FAILURE_ALIGNMENT;
		}
		i64 size = type_size_of_internal(allocator, t->Named.base, path);
		type_path_pop(path);
		return size;
	} break;

	case Type_Basic: {
		GB_ASSERT_MSG(is_type_typed(t), "%s", type_to_string(t));
		BasicKind kind = t->Basic.kind;
		i64 size = t->Basic.size;
		if (size > 0) {
			return size;
		}
		switch (kind) {
		case Basic_string: return 2*build_context.word_size;
		case Basic_any:    return 2*build_context.word_size;

		case Basic_int: case Basic_uint: case Basic_rawptr:
			return build_context.word_size;
		}
	} break;

	case Type_Array: {
		i64 count, align, size, alignment;
		count = t->Array.count;
		if (count == 0) {
			return 0;
		}
		align = type_align_of_internal(allocator, t->Array.elem, path);
		if (path->failure) {
			return FAILURE_SIZE;
		}
		size  = type_size_of_internal( allocator, t->Array.elem, path);
		alignment = align_formula(size, align);
		return alignment*(count-1) + size;
	} break;

	case Type_DynamicArray:
		// data + len + cap + allocator(procedure+data)
		return 3*build_context.word_size + 2*build_context.word_size;

	case Type_Vector: {
#if 0
		i64 count, bit_size, total_size_in_bits, total_size;
		count = t->Vector.count;
		if (count == 0) {
			return 0;
		}
		type_path_push(path, t->Vector.elem);
		if (path->failure) {
			return FAILURE_SIZE;
		}
		bit_size = 8*type_size_of_internal(allocator, t->Vector.elem, path);
		type_path_pop(path);
		if (is_type_boolean(t->Vector.elem)) {
			bit_size = 1; // NOTE(bill): LLVM can store booleans as 1 bit because a boolean _is_ an `i1`
			              // Silly LLVM spec
		}
		total_size_in_bits = bit_size * count;
		total_size = (total_size_in_bits+7)/8;
		return total_size;
#else
		i64 count = t->Vector.count;
		if (count == 0) {
			return 0;
		}
		i64 elem_align = type_align_of_internal(allocator, t->Vector.elem, path);
		if (path->failure) {
			return FAILURE_SIZE;
		}
		i64 vector_align = type_align_of_internal(allocator, t, path);
		i64 elem_size = type_size_of_internal(allocator, t->Vector.elem, path);
		i64 alignment = align_formula(elem_size, elem_align);
		return align_formula(alignment*(count-1) + elem_size, vector_align);
#endif
	} break;


	case Type_Slice: // ptr + count
		return 3 * build_context.word_size;

	case Type_Map: {
		if (t->Map.count == 0) { // Dynamic
			return type_size_of_internal(allocator, t->Map.generated_struct_type, path);
		}
		GB_PANIC("TODO(bill): Fixed map size");
	}

	case Type_Tuple: {
		i64 count, align, size;
		count = t->Tuple.variable_count;
		if (count == 0) {
			return 0;
		}
		align = type_align_of_internal(allocator, t, path);
		type_set_offsets(allocator, t);
		size = t->Tuple.offsets[count-1] + type_size_of_internal(allocator, t->Tuple.variables[count-1]->type, path);
		return align_formula(size, align);
	} break;

	case Type_Record: {
		switch (t->Record.kind) {
		case TypeRecord_Enum:
			return type_size_of_internal(allocator, t->Record.enum_base_type, path);

		case TypeRecord_Struct: {
			i64 count = t->Record.field_count;
			if (count == 0) {
				return 0;
			}
			i64 align = type_align_of_internal(allocator, t, path);
			if (path->failure) {
				return FAILURE_SIZE;
			}
			if (t->Record.are_offsets_being_processed && t->Record.offsets == NULL) {
				type_path_print_illegal_cycle(path, path->path.count-1);
				return FAILURE_SIZE;
			}
			type_set_offsets(allocator, t);
			i64 size = t->Record.offsets[count-1] + type_size_of_internal(allocator, t->Record.fields[count-1]->type, path);
			return align_formula(size, align);
		} break;

		case TypeRecord_Union: {
			i64 align = type_align_of_internal(allocator, t, path);
			if (path->failure) {
				return FAILURE_SIZE;
			}

			i64 max = 0;
			isize field_count = t->Record.field_count;
			isize variant_count = t->Record.variant_count;

			// Check for recursive types
			for (isize i = 0; i < field_count; i++) {
				i64 size = type_size_of_internal(allocator, t->Record.fields[i]->type, path);
				if (path->failure) {
					return FAILURE_SIZE;
				}
			}
			// NOTE(bill): Zeroth field is invalid
			type_set_offsets(allocator, t);

			if (field_count > 0) {
				Type *end_type = t->Record.fields[field_count-1]->type;
				i64 end_offset = t->Record.offsets[field_count-1];
				i64 end_size = type_size_of_internal(allocator, end_type, path);
				max = end_offset + end_size ;
			}
			i64 field_size = max;

			for (isize i = 1; i < variant_count; i++) {
				Type *variant_type = t->Record.variants[i]->type;
				i64 size = type_size_of_internal(allocator, variant_type, path);
				if (max < size) {
					max = size;
				}
			}

			// NOTE(bill): Align to int
			i64 size = align_formula(max, build_context.word_size);
			// NOTE(bill): Calculate the padding between the common fields and the tag
			t->Record.variant_block_size = size - field_size;

			size += type_size_of(allocator, t_int);
			size = align_formula(size, align);
			return size;
		} break;

		case TypeRecord_RawUnion: {
			i64 count = t->Record.field_count;
			i64 align = type_align_of_internal(allocator, t, path);
			if (path->failure) {
				return FAILURE_SIZE;
			}
			i64 max = 0;
			for (isize i = 0; i < count; i++) {
				i64 size = type_size_of_internal(allocator, t->Record.fields[i]->type, path);
				if (max < size) {
					max = size;
				}
			}
			// TODO(bill): Is this how it should work?
			return align_formula(max, align);
		} break;
		}
	} break;

	case Type_BitField: {
		i64 align = 8*type_align_of_internal(allocator, t, path);
		i64 end = 0;
		if (t->BitField.field_count > 0) {
			i64 last = t->BitField.field_count-1;
			end = t->BitField.offsets[last] + t->BitField.sizes[last];
		}
		i64 bits = align_formula(end, align);
		GB_ASSERT((bits%8) == 0);
		return bits/8;
	} break;
	}

	// Catch all
	return build_context.word_size;
}

i64 type_offset_of(gbAllocator allocator, Type *t, i32 index) {
	t = base_type(t);
	if (t->kind == Type_Record && (t->Record.kind == TypeRecord_Struct || t->Record.kind == TypeRecord_Union)) {
		type_set_offsets(allocator, t);
		if (gb_is_between(index, 0, t->Record.field_count-1)) {
			return t->Record.offsets[index];
		}
	} else if (t->kind == Type_Tuple) {
		type_set_offsets(allocator, t);
		if (gb_is_between(index, 0, t->Tuple.variable_count-1)) {
			return t->Tuple.offsets[index];
		}
	}  else if (t->kind == Type_Basic) {
		if (t->Basic.kind == Basic_string) {
			switch (index) {
			case 0: return 0;                       // data
			case 1: return build_context.word_size; // count
			}
		} else if (t->Basic.kind == Basic_any) {
			switch (index) {
			case 0: return 0;                       // type_info
			case 1: return build_context.word_size; // data
			}
		}
	} else if (t->kind == Type_Slice) {
		switch (index) {
		case 0: return 0;                         // data
		case 1: return 1*build_context.word_size; // count
		case 2: return 2*build_context.word_size; // capacity
		}
	} else if (t->kind == Type_DynamicArray) {
		switch (index) {
		case 0: return 0;                         // data
		case 1: return 1*build_context.word_size; // count
		case 2: return 2*build_context.word_size; // capacity
		case 3: return 3*build_context.word_size; // allocator
		}
	}
	return 0;
}


i64 type_offset_of_from_selection(gbAllocator allocator, Type *type, Selection sel) {
	GB_ASSERT(sel.indirect == false);

	Type *t = type;
	i64 offset = 0;
	for_array(i, sel.index) {
		isize index = sel.index[i];
		t = base_type(t);
		offset += type_offset_of(allocator, t, index);
		if (t->kind == Type_Record && t->Record.kind == TypeRecord_Struct) {
			t = t->Record.fields[index]->type;
		} else {
			// NOTE(bill): No need to worry about custom types, just need the alignment
			switch (t->kind) {
			case Type_Basic:
				if (t->Basic.kind == Basic_string) {
					switch (index) {
					case 0: t = t_rawptr; break;
					case 1: t = t_int;    break;
					}
				} else if (t->Basic.kind == Basic_any) {
					switch (index) {
					case 0: t = t_type_info_ptr; break;
					case 1: t = t_rawptr;        break;
					}
				}
				break;
			case Type_Slice:
				switch (index) {
				case 0: t = t_rawptr; break;
				case 1: t = t_int;    break;
				case 2: t = t_int;    break;
				}
				break;
			case Type_DynamicArray:
				switch (index) {
				case 0: t = t_rawptr;    break;
				case 1: t = t_int;       break;
				case 2: t = t_int;       break;
				case 3: t = t_allocator; break;
				}
				break;
			}
		}
	}
	return offset;
}

gbString write_type_to_string(gbString str, Type *type) {
	if (type == NULL) {
		return gb_string_appendc(str, "<no type>");
	}

	switch (type->kind) {
	case Type_Basic:
		str = gb_string_append_length(str, type->Basic.name.text, type->Basic.name.len);
		break;

	case Type_Generic:
		str = gb_string_appendc(str, "type");
		break;

	case Type_Pointer:
		str = gb_string_appendc(str, "^");
		str = write_type_to_string(str, type->Pointer.elem);
		break;

	case Type_Atomic:
		str = gb_string_appendc(str, "atomic ");
		str = write_type_to_string(str, type->Atomic.elem);
		break;

	case Type_Array:
		str = gb_string_appendc(str, gb_bprintf("[%d]", cast(int)type->Array.count));
		str = write_type_to_string(str, type->Array.elem);
		break;

	case Type_Vector:
		str = gb_string_appendc(str, gb_bprintf("[vector %d]", cast(int)type->Vector.count));
		str = write_type_to_string(str, type->Vector.elem);
		break;

	case Type_Slice:
		str = gb_string_appendc(str, "[]");
		str = write_type_to_string(str, type->Array.elem);
		break;

	case Type_DynamicArray:
		str = gb_string_appendc(str, "[dynamic]");
		str = write_type_to_string(str, type->DynamicArray.elem);
		break;

	case Type_Record: {
		switch (type->Record.kind) {
		case TypeRecord_Struct:
			str = gb_string_appendc(str, "struct");
			if (type->Record.is_packed) {
				str = gb_string_appendc(str, " #packed");
			}
			if (type->Record.is_ordered) {
				str = gb_string_appendc(str, " #ordered");
			}
			str = gb_string_appendc(str, " {");
			for (isize i = 0; i < type->Record.field_count; i++) {
				Entity *f = type->Record.fields[i];
				GB_ASSERT(f->kind == Entity_Variable);
				if (i > 0) {
					str = gb_string_appendc(str, ", ");
				}
				str = gb_string_append_length(str, f->token.string.text, f->token.string.len);
				str = gb_string_appendc(str, ": ");
				str = write_type_to_string(str, f->type);
			}
			str = gb_string_appendc(str, "}");
			break;

		case TypeRecord_Union:
			str = gb_string_appendc(str, "union{");
			for (isize i = 0; i < type->Record.field_count; i++) {
				Entity *f = type->Record.fields[i];
				GB_ASSERT(f->kind == Entity_Variable);
				if (i > 0) {
					str = gb_string_appendc(str, ", ");
				}
				str = gb_string_append_length(str, f->token.string.text, f->token.string.len);
				str = gb_string_appendc(str, ": ");
				str = write_type_to_string(str, f->type);
			}
			for (isize i = 1; i < type->Record.variant_count; i++) {
				Entity *f = type->Record.variants[i];
				GB_ASSERT(f->kind == Entity_TypeName);
				if (i > 1 || type->Record.field_count > 1) {
					str = gb_string_appendc(str, ", ");
				}
				str = gb_string_append_length(str, f->token.string.text, f->token.string.len);
				str = gb_string_appendc(str, "{");
				Type *variant = base_type(f->type);
				for (isize i = 0; i < variant->Record.field_count; i++) {
					Entity *f = variant->Record.fields[i];
					GB_ASSERT(f->kind == Entity_Variable);
					if (i > 0) {
						str = gb_string_appendc(str, ", ");
					}
					str = gb_string_append_length(str, f->token.string.text, f->token.string.len);
					str = gb_string_appendc(str, ": ");
					str = write_type_to_string(str, f->type);
				}
				str = gb_string_appendc(str, "{");

			}
			str = gb_string_appendc(str, "}");
			break;

		case TypeRecord_RawUnion:
			str = gb_string_appendc(str, "raw_union{");
			for (isize i = 0; i < type->Record.field_count; i++) {
				Entity *f = type->Record.fields[i];
				GB_ASSERT(f->kind == Entity_Variable);
				if (i > 0) {
					str = gb_string_appendc(str, ", ");
				}
				str = gb_string_append_length(str, f->token.string.text, f->token.string.len);
				str = gb_string_appendc(str, ": ");
				str = write_type_to_string(str, f->type);
			}
			str = gb_string_appendc(str, "}");
			break;

		case TypeRecord_Enum:
			str = gb_string_appendc(str, "enum");
			if (type->Record.enum_base_type != NULL) {
			str = gb_string_appendc(str, " ");
				str = write_type_to_string(str, type->Record.enum_base_type);
			}
			str = gb_string_appendc(str, " {");
			for (isize i = 0; i < type->Record.field_count; i++) {
				Entity *f = type->Record.fields[i];
				GB_ASSERT(f->kind == Entity_Constant);
				if (i > 0) {
					str = gb_string_appendc(str, ", ");
				}
				str = gb_string_append_length(str, f->token.string.text, f->token.string.len);
				// str = gb_string_appendc(str, " = ");
			}
			str = gb_string_appendc(str, "}");
			break;
		}
	} break;

	case Type_Map: {
		str = gb_string_appendc(str, "map[");
		if (type->Map.count > 0) {
			str = gb_string_appendc(str, gb_bprintf("%d, ", cast(int)type->Map.count));
		}
		str = write_type_to_string(str, type->Map.key);
		str = gb_string_appendc(str, "]");
		str = write_type_to_string(str, type->Map.value);
	} break;

	case Type_Named:
		if (type->Named.type_name != NULL) {
			str = gb_string_append_length(str, type->Named.name.text, type->Named.name.len);
		} else {
			// NOTE(bill): Just in case
			str = gb_string_appendc(str, "<named type>");
		}
		break;

	case Type_Tuple:
		if (type->Tuple.variable_count > 0) {
			for (isize i = 0; i < type->Tuple.variable_count; i++) {
				Entity *var = type->Tuple.variables[i];
				if (var != NULL) {
					if (i > 0) {
						str = gb_string_appendc(str, ", ");
					}
					if (var->kind == Entity_Variable) {
						if (var->flags&EntityFlag_CVarArg) {
							str = gb_string_appendc(str, "#c_vararg ");
						}
						if (var->flags&EntityFlag_Ellipsis) {
							Type *slice = base_type(var->type);
							str = gb_string_appendc(str, "..");
							GB_ASSERT(is_type_slice(var->type));
							str = write_type_to_string(str, slice->Slice.elem);
						} else {
							str = write_type_to_string(str, var->type);
						}
					} else {
						GB_ASSERT(var->kind == Entity_TypeName);
						str = gb_string_appendc(str, "type/");
						str = write_type_to_string(str, var->type);
					}
				}
			}
		}
		break;

	case Type_Proc:
		str = gb_string_appendc(str, "proc(");
		if (type->Proc.params) {
			str = write_type_to_string(str, type->Proc.params);
		}
		str = gb_string_appendc(str, ")");
		if (type->Proc.results) {
			str = gb_string_appendc(str, " -> ");
			str = write_type_to_string(str, type->Proc.results);
		}
		switch (type->Proc.calling_convention) {
		case ProcCC_Odin:
			// str = gb_string_appendc(str, " #cc_odin");
			break;
		case ProcCC_C:
			str = gb_string_appendc(str, " #cc_c");
			break;
		case ProcCC_Std:
			str = gb_string_appendc(str, " #cc_std");
			break;
		case ProcCC_Fast:
			str = gb_string_appendc(str, " #cc_fast");
			break;
		}
		break;

	case Type_BitField:
		str = gb_string_appendc(str, "bit_field ");
		if (type->BitField.custom_align != 0) {
			str = gb_string_appendc(str, gb_bprintf("#align %d ", cast(int)type->BitField.custom_align));
		}
		str = gb_string_appendc(str, "{");

		for (isize i = 0; i < type->BitField.field_count; i++) {
			Entity *f = type->BitField.fields[i];
			GB_ASSERT(f->kind == Entity_Variable);
			GB_ASSERT(f->type != NULL && f->type->kind == Type_BitFieldValue);
			str = gb_string_appendc(str, "{");
			if (i > 0) {
				str = gb_string_appendc(str, ", ");
			}
			str = gb_string_append_length(str, f->token.string.text, f->token.string.len);
			str = gb_string_appendc(str, " : ");
			str = gb_string_appendc(str, gb_bprintf("%lld", cast(long long)f->type->BitFieldValue.bits));
		}
		str = gb_string_appendc(str, "}");
		break;

	case Type_BitFieldValue:
		str = gb_string_appendc(str, gb_bprintf("(bit field value with %d bits)", cast(int)type->BitFieldValue.bits));
		break;
	}

	return str;
}


gbString type_to_string(Type *type) {
	return write_type_to_string(gb_string_make(heap_allocator(), ""), type);
}


