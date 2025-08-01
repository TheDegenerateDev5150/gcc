/****************************************************************************
 *                                                                          *
 *                         GNAT COMPILER COMPONENTS                         *
 *                                                                          *
 *                                U T I L S                                 *
 *                                                                          *
 *                          C Implementation File                           *
 *                                                                          *
 *          Copyright (C) 1992-2025, Free Software Foundation, Inc.         *
 *                                                                          *
 * GNAT is free software;  you can  redistribute it  and/or modify it under *
 * terms of the  GNU General Public License as published  by the Free Soft- *
 * ware  Foundation;  either version 3,  or (at your option) any later ver- *
 * sion.  GNAT is distributed in the hope that it will be useful, but WITH- *
 * OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License *
 * for  more details.  You should have received a copy of the GNU General   *
 * Public License along with GCC; see the file COPYING3.  If not see        *
 * <http://www.gnu.org/licenses/>.                                          *
 *                                                                          *
 * GNAT was originally developed  by the GNAT team at  New York University. *
 * Extensive contributions were provided by Ada Core Technologies Inc.      *
 *                                                                          *
 ****************************************************************************/

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "target.h"
#include "function.h"
#include "tree.h"
#include "stringpool.h"
#include "cgraph.h"
#include "diagnostic.h"
#include "alias.h"
#include "fold-const.h"
#include "stor-layout.h"
#include "attribs.h"
#include "varasm.h"
#include "toplev.h"
#include "opts.h"
#include "ipa-strub.h"
#include "output.h"
#include "debug.h"
#include "convert.h"
#include "common/common-target.h"
#include "langhooks.h"
#include "tree-dump.h"
#include "tree-inline.h"

#include "ada.h"
#include "types.h"
#include "atree.h"
#include "nlists.h"
#include "snames.h"
#include "uintp.h"
#include "fe.h"
#include "sinfo.h"
#include "einfo.h"
#include "ada-tree.h"
#include "gigi.h"

/* If nonzero, pretend we are allocating at global level.  */
int force_global;

/* The default alignment of "double" floating-point types, i.e. floating
   point types whose size is equal to 64 bits, or 0 if this alignment is
   not specifically capped.  */
int double_float_alignment;

/* The default alignment of "double" or larger scalar types, i.e. scalar
   types whose size is greater or equal to 64 bits, or 0 if this alignment
   is not specifically capped.  */
int double_scalar_alignment;

/* True if floating-point arithmetics may use wider intermediate results.  */
bool fp_arith_may_widen = true;

/* Tree nodes for the various types and decls we create.  */
tree gnat_std_decls[(int) ADT_LAST];

/* Functions to call for each of the possible raise reasons.  */
tree gnat_raise_decls[(int) LAST_REASON_CODE + 1];

/* Likewise, but with extra info for each of the possible raise reasons.  */
tree gnat_raise_decls_ext[(int) LAST_REASON_CODE + 1];

/* Forward declarations for handlers of attributes.  */
static tree handle_const_attribute (tree *, tree, tree, int, bool *);
static tree handle_nothrow_attribute (tree *, tree, tree, int, bool *);
static tree handle_expected_throw_attribute (tree *, tree, tree, int, bool *);
static tree handle_pure_attribute (tree *, tree, tree, int, bool *);
static tree handle_novops_attribute (tree *, tree, tree, int, bool *);
static tree handle_nonnull_attribute (tree *, tree, tree, int, bool *);
static tree handle_sentinel_attribute (tree *, tree, tree, int, bool *);
static tree handle_noreturn_attribute (tree *, tree, tree, int, bool *);
static tree handle_stack_protect_attribute (tree *, tree, tree, int, bool *);
static tree handle_no_stack_protector_attribute (tree *, tree, tree, int, bool *);
static tree handle_strub_attribute (tree *, tree, tree, int, bool *);
static tree handle_noinline_attribute (tree *, tree, tree, int, bool *);
static tree handle_noclone_attribute (tree *, tree, tree, int, bool *);
static tree handle_noicf_attribute (tree *, tree, tree, int, bool *);
static tree handle_noipa_attribute (tree *, tree, tree, int, bool *);
static tree handle_leaf_attribute (tree *, tree, tree, int, bool *);
static tree handle_always_inline_attribute (tree *, tree, tree, int, bool *);
static tree handle_malloc_attribute (tree *, tree, tree, int, bool *);
static tree handle_type_generic_attribute (tree *, tree, tree, int, bool *);
static tree handle_flatten_attribute (tree *, tree, tree, int, bool *);
static tree handle_used_attribute (tree *, tree, tree, int, bool *);
static tree handle_uninitialized_attribute (tree *, tree, tree, int, bool *);
static tree handle_cold_attribute (tree *, tree, tree, int, bool *);
static tree handle_hot_attribute (tree *, tree, tree, int, bool *);
static tree handle_simd_attribute (tree *, tree, tree, int, bool *);
static tree handle_target_attribute (tree *, tree, tree, int, bool *);
static tree handle_target_clones_attribute (tree *, tree, tree, int, bool *);
static tree handle_vector_size_attribute (tree *, tree, tree, int, bool *);
static tree handle_vector_type_attribute (tree *, tree, tree, int, bool *);
static tree handle_zero_call_used_regs_attribute (tree *, tree, tree, int,
						  bool *);

static const struct attribute_spec::exclusions attr_cold_hot_exclusions[] =
{
  { "cold", true,  true,  true  },
  { "hot" , true,  true,  true  },
  { NULL  , false, false, false }
};

static const struct attribute_spec::exclusions attr_stack_protect_exclusions[] =
{
  { "stack_protect", true, false, false },
  { "no_stack_protector", true, false, false },
  { NULL, false, false, false },
};

static const struct attribute_spec::exclusions attr_always_inline_exclusions[] =
{
  { "noinline", true, true, true },
  { "target_clones", true, true, true },
  { NULL, false, false, false },
};

static const struct attribute_spec::exclusions attr_noinline_exclusions[] =
{
  { "always_inline", true, true, true },
  { NULL, false, false, false },
};

static const struct attribute_spec::exclusions attr_target_exclusions[] =
{
  { "target_clones", TARGET_HAS_FMV_TARGET_ATTRIBUTE,
    TARGET_HAS_FMV_TARGET_ATTRIBUTE, TARGET_HAS_FMV_TARGET_ATTRIBUTE },
  { NULL, false, false, false },
};

static const struct attribute_spec::exclusions attr_target_clones_exclusions[] =
{
  { "always_inline", true, true, true },
  { "target", TARGET_HAS_FMV_TARGET_ATTRIBUTE, TARGET_HAS_FMV_TARGET_ATTRIBUTE,
    TARGET_HAS_FMV_TARGET_ATTRIBUTE },
  { NULL, false, false, false },
};

/* Fake handler for attributes we don't properly support, typically because
   they'd require dragging a lot of the common-c front-end circuitry.  */
static tree fake_attribute_handler (tree *, tree, tree, int, bool *);

/* Table of machine-independent internal attributes for Ada.  We support
   this minimal set of attributes to accommodate the needs of builtins.  */
static const attribute_spec gnat_internal_attributes[] =
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req,
       affects_type_identity, handler, exclude } */
  { "const",        0, 0,  true,  false, false, false,
    handle_const_attribute, NULL },
  { "nothrow",      0, 0,  true,  false, false, false,
    handle_nothrow_attribute, NULL },
  { "expected_throw", 0, 0, true,  false, false, false,
    handle_expected_throw_attribute, NULL },
  { "pure",         0, 0,  true,  false, false, false,
    handle_pure_attribute, NULL },
  { "no vops",      0, 0,  true,  false, false, false,
    handle_novops_attribute, NULL },
  { "nonnull",      0, -1, false, true,  true,  false,
    handle_nonnull_attribute, NULL },
  { "sentinel",     0, 1,  false, true,  true,  false,
    handle_sentinel_attribute, NULL },
  { "noreturn",     0, 0,  true,  false, false, false,
    handle_noreturn_attribute, NULL },
  { "stack_protect",0, 0, true,  false, false, false,
    handle_stack_protect_attribute,
    attr_stack_protect_exclusions },
  { "no_stack_protector",0, 0, true,  false, false, false,
    handle_no_stack_protector_attribute,
    attr_stack_protect_exclusions },
  { "strub",	    0, 1, false, true, false, true,
    handle_strub_attribute, NULL },
  { "noinline",     0, 0,  true,  false, false, false,
    handle_noinline_attribute, attr_noinline_exclusions },
  { "noclone",      0, 0,  true,  false, false, false,
    handle_noclone_attribute, NULL },
  { "no_icf",       0, 0,  true,  false, false, false,
    handle_noicf_attribute, NULL },
  { "noipa",        0, 0,  true,  false, false, false,
    handle_noipa_attribute, NULL },
  { "leaf",         0, 0,  true,  false, false, false,
    handle_leaf_attribute, NULL },
  { "always_inline",0, 0,  true,  false, false, false,
    handle_always_inline_attribute, attr_always_inline_exclusions },
  { "malloc",       0, 0,  true,  false, false, false,
    handle_malloc_attribute, NULL },
  { "type generic", 0, 0,  false, true,  true,  false,
    handle_type_generic_attribute, NULL },

  { "flatten",      0, 0,  true,  false, false, false,
    handle_flatten_attribute, NULL },
  { "used",         0, 0,  true,  false, false, false,
    handle_used_attribute, NULL },
  { "uninitialized",0, 0,  true,  false, false, false,
    handle_uninitialized_attribute, NULL },
  { "cold",         0, 0,  true,  false, false, false,
    handle_cold_attribute, attr_cold_hot_exclusions },
  { "hot",          0, 0,  true,  false, false, false,
    handle_hot_attribute, attr_cold_hot_exclusions },
  { "simd",         0, 1,  true,  false, false, false,
    handle_simd_attribute, NULL },
  { "target",       1, -1, true,  false, false, false,
    handle_target_attribute, attr_target_exclusions },
  { "target_clones",1, -1, true,  false, false, false,
    handle_target_clones_attribute, attr_target_clones_exclusions },

  { "vector_size",  1, 1,  false, true,  false, false,
    handle_vector_size_attribute, NULL },
  { "vector_type",  0, 0,  false, true,  false, false,
    handle_vector_type_attribute, NULL },
  { "may_alias",    0, 0,  false, true,  false, false,
    NULL, NULL },

  { "zero_call_used_regs", 1, 1, true, false, false, false,
    handle_zero_call_used_regs_attribute, NULL },

  /* ??? format and format_arg are heavy and not supported, which actually
     prevents support for stdio builtins, which we however declare as part
     of the common builtins.def contents.  */
  { "format",       3, 3,  false, true,  true,  false,
    fake_attribute_handler, NULL },
  { "format_arg",   1, 1,  false, true,  true,  false,
    fake_attribute_handler, NULL },

  /* This is handled entirely in the front end.  */
  { "hardbool",     0, 0,  false, true, false, true,
    fake_attribute_handler, NULL },
};

const scoped_attribute_specs gnat_internal_attribute_table =
{
  "gnu", { gnat_internal_attributes }
};

/* Associates a GNAT tree node to a GCC tree node. It is used in
   `save_gnu_tree', `get_gnu_tree' and `present_gnu_tree'. See documentation
   of `save_gnu_tree' for more info.  */
static GTY((length ("max_gnat_nodes"))) tree *associate_gnat_to_gnu;

#define GET_GNU_TREE(GNAT_ENTITY)	\
  associate_gnat_to_gnu[(GNAT_ENTITY) - First_Node_Id]

#define SET_GNU_TREE(GNAT_ENTITY,VAL)	\
  associate_gnat_to_gnu[(GNAT_ENTITY) - First_Node_Id] = (VAL)

#define PRESENT_GNU_TREE(GNAT_ENTITY)	\
  (associate_gnat_to_gnu[(GNAT_ENTITY) - First_Node_Id] != NULL_TREE)

/* Associates a GNAT entity to a GCC tree node used as a dummy, if any.  */
static GTY((length ("max_gnat_nodes"))) tree *dummy_node_table;

#define GET_DUMMY_NODE(GNAT_ENTITY)	\
  dummy_node_table[(GNAT_ENTITY) - First_Node_Id]

#define SET_DUMMY_NODE(GNAT_ENTITY,VAL)	\
  dummy_node_table[(GNAT_ENTITY) - First_Node_Id] = (VAL)

#define PRESENT_DUMMY_NODE(GNAT_ENTITY)	\
  (dummy_node_table[(GNAT_ENTITY) - First_Node_Id] != NULL_TREE)

/* This variable keeps a table for types for each precision so that we only
   allocate each of them once. Signed and unsigned types are kept separate.

   Note that these types are only used when fold-const requests something
   special.  Perhaps we should NOT share these types; we'll see how it
   goes later.  */
static GTY(()) tree signed_and_unsigned_types[2 * MAX_BITS_PER_WORD + 1][2];

/* Likewise for float types, but record these by mode.  */
static GTY(()) tree float_types[NUM_MACHINE_MODES];

/* For each binding contour we allocate a binding_level structure to indicate
   the binding depth.  */

struct GTY((chain_next ("%h.chain"))) gnat_binding_level {
  /* The binding level containing this one (the enclosing binding level). */
  struct gnat_binding_level *chain;
  /* The BLOCK node for this level.  */
  tree block;
};

/* The binding level currently in effect.  */
static GTY(()) struct gnat_binding_level *current_binding_level;

/* A chain of gnat_binding_level structures awaiting reuse.  */
static GTY((deletable)) struct gnat_binding_level *free_binding_level;

/* The context to be used for global declarations.  */
static GTY(()) tree global_context;

/* An array of global declarations.  */
static GTY(()) vec<tree, va_gc> *global_decls;

/* An array of builtin function declarations.  */
static GTY(()) vec<tree, va_gc> *builtin_decls;

/* A chain of unused BLOCK nodes. */
static GTY((deletable)) tree free_block_chain;

/* A hash table of packable types.  It is modelled on the generic type
   hash table in tree.cc, which must thus be used as a reference.  */

struct GTY((for_user)) packable_type_hash
{
  hashval_t hash;
  tree type;
};

struct packable_type_hasher : ggc_cache_ptr_hash<packable_type_hash>
{
  static inline hashval_t hash (packable_type_hash *t) { return t->hash; }
  static bool equal (packable_type_hash *a, packable_type_hash *b);

  static int
  keep_cache_entry (packable_type_hash *&t)
  {
    return ggc_marked_p (t->type);
  }
};

static GTY ((cache)) hash_table<packable_type_hasher> *packable_type_hash_table;

/* A hash table of padded types.  It is modelled on the generic type
   hash table in tree.cc, which must thus be used as a reference.  */

struct GTY((for_user)) pad_type_hash
{
  hashval_t hash;
  tree type;
};

struct pad_type_hasher : ggc_cache_ptr_hash<pad_type_hash>
{
  static inline hashval_t hash (pad_type_hash *t) { return t->hash; }
  static bool equal (pad_type_hash *a, pad_type_hash *b);

  static int
  keep_cache_entry (pad_type_hash *&t)
  {
    return ggc_marked_p (t->type);
  }
};

static GTY ((cache)) hash_table<pad_type_hasher> *pad_type_hash_table;

struct GTY((for_user)) sized_type_hash
{
  hashval_t hash;
  tree type;
};

struct sized_type_hasher : ggc_cache_ptr_hash<sized_type_hash>
{
  static inline hashval_t hash (sized_type_hash *t) { return t->hash; }
  static bool equal (sized_type_hash *a, sized_type_hash *b);

  static int
  keep_cache_entry (sized_type_hash *&t)
  {
    return ggc_marked_p (t->type);
  }
};

static GTY ((cache)) hash_table<sized_type_hasher> *sized_type_hash_table;

static tree merge_sizes (tree, tree, tree, bool, bool);
static tree fold_bit_position (const_tree);
static tree compute_related_constant (tree, tree);
static tree split_plus (tree, tree *);
static tree float_type_for_precision (int, machine_mode);
static tree convert_to_fat_pointer (tree, tree);
static unsigned int scale_by_factor_of (tree, unsigned int);

/* Linked list used as a queue to defer the initialization of the DECL_CONTEXT
   of ..._DECL nodes and of the TYPE_CONTEXT of ..._TYPE nodes.  */
struct deferred_decl_context_node
{
  /* The ..._DECL node to work on.  */
  tree decl;

  /* The corresponding entity's Scope.  */
  Entity_Id gnat_scope;

  /* The value of force_global when DECL was pushed.  */
  int force_global;

  /* The list of ..._TYPE nodes to propagate the context to.  */
  vec<tree> types;

  /* The next queue item.  */
  struct deferred_decl_context_node *next;
};

static struct deferred_decl_context_node *deferred_decl_context_queue = NULL;

/* Defer the initialization of DECL's DECL_CONTEXT attribute, scheduling to
   feed it with the elaboration of GNAT_SCOPE.  */
static struct deferred_decl_context_node *
add_deferred_decl_context (tree decl, Entity_Id gnat_scope, int force_global);

/* Defer the initialization of TYPE's TYPE_CONTEXT attribute, scheduling to
   feed it with the DECL_CONTEXT computed as part of N as soon as it is
   computed.  */
static void add_deferred_type_context (struct deferred_decl_context_node *n,
				       tree type);

/* Initialize data structures of the utils.cc module.  */

void
init_gnat_utils (void)
{
  /* Initialize the association of GNAT nodes to GCC trees.  */
  associate_gnat_to_gnu = ggc_cleared_vec_alloc<tree> (max_gnat_nodes);

  /* Initialize the association of GNAT nodes to GCC trees as dummies.  */
  dummy_node_table = ggc_cleared_vec_alloc<tree> (max_gnat_nodes);

  /* Initialize the hash table of packable types.  */
  packable_type_hash_table = hash_table<packable_type_hasher>::create_ggc (512);

  /* Initialize the hash table of padded types.  */
  pad_type_hash_table = hash_table<pad_type_hasher>::create_ggc (512);

  /* Initialize the hash table of sized types.  */
  sized_type_hash_table = hash_table<sized_type_hasher>::create_ggc (512);
}

/* Destroy data structures of the utils.cc module.  */

void
destroy_gnat_utils (void)
{
  /* Destroy the association of GNAT nodes to GCC trees.  */
  ggc_free (associate_gnat_to_gnu);
  associate_gnat_to_gnu = NULL;

  /* Destroy the association of GNAT nodes to GCC trees as dummies.  */
  ggc_free (dummy_node_table);
  dummy_node_table = NULL;

  /* Destroy the hash table of packable types.  */
  packable_type_hash_table->empty ();
  packable_type_hash_table = NULL;

  /* Destroy the hash table of padded types.  */
  pad_type_hash_table->empty ();
  pad_type_hash_table = NULL;

  /* Destroy the hash table of sized types.  */
  sized_type_hash_table->empty ();
  sized_type_hash_table = NULL;
}

/* GNAT_ENTITY is a GNAT tree node for an entity.  Associate GNU_DECL, a GCC
   tree node, with GNAT_ENTITY.  If GNU_DECL is not a ..._DECL node, abort.
   If NO_CHECK is true, the latter check is suppressed.

   If GNU_DECL is zero, reset a previous association.  */

void
save_gnu_tree (Entity_Id gnat_entity, tree gnu_decl, bool no_check)
{
  /* Check that GNAT_ENTITY is not already defined and that it is being set
     to something which is a decl.  If that is not the case, this usually
     means GNAT_ENTITY is defined twice, but occasionally is due to some
     Gigi problem.  */
  gcc_assert (!(gnu_decl
		&& (PRESENT_GNU_TREE (gnat_entity)
		    || (!no_check && !DECL_P (gnu_decl)))));

  SET_GNU_TREE (gnat_entity, gnu_decl);
}

/* GNAT_ENTITY is a GNAT tree node for an entity.  Return the GCC tree node
   that was associated with it.  If there is no such tree node, abort.

   In some cases, such as delayed elaboration or expressions that need to
   be elaborated only once, GNAT_ENTITY is really not an entity.  */

tree
get_gnu_tree (Entity_Id gnat_entity)
{
  gcc_assert (PRESENT_GNU_TREE (gnat_entity));
  return GET_GNU_TREE (gnat_entity);
}

/* Return nonzero if a GCC tree has been associated with GNAT_ENTITY.  */

bool
present_gnu_tree (Entity_Id gnat_entity)
{
  return PRESENT_GNU_TREE (gnat_entity);
}

/* Make a dummy type corresponding to GNAT_TYPE.  */

tree
make_dummy_type (Entity_Id gnat_type)
{
  Entity_Id gnat_equiv = Gigi_Equivalent_Type (Underlying_Type (gnat_type));
  tree gnu_type, debug_type;

  /* If there was no equivalent type (can only happen when just annotating
     types) or underlying type, go back to the original type.  */
  if (No (gnat_equiv))
    gnat_equiv = gnat_type;

  /* If there is already a dummy type, use that one.  Else make one.  */
  if (PRESENT_DUMMY_NODE (gnat_equiv))
    return GET_DUMMY_NODE (gnat_equiv);

  /* If this is a record, make a RECORD_TYPE or UNION_TYPE; else make
     an ENUMERAL_TYPE.  */
  gnu_type = make_node (Is_Record_Type (gnat_equiv)
			? tree_code_for_record_type (gnat_equiv)
			: ENUMERAL_TYPE);
  TYPE_NAME (gnu_type) = get_entity_name (gnat_type);
  TYPE_DUMMY_P (gnu_type) = 1;
  TYPE_STUB_DECL (gnu_type)
    = create_type_stub_decl (TYPE_NAME (gnu_type), gnu_type);
  if (Is_By_Reference_Type (gnat_equiv))
    TYPE_BY_REFERENCE_P (gnu_type) = 1;
  if (Has_Discriminants (gnat_equiv))
    decl_attributes (&gnu_type,
		     tree_cons (get_identifier ("may_alias"), NULL_TREE,
				NULL_TREE),
		     ATTR_FLAG_TYPE_IN_PLACE);

  SET_DUMMY_NODE (gnat_equiv, gnu_type);

  /* Create a debug type so that debuggers only see an unspecified type.  */
  if (Needs_Debug_Info (gnat_type))
    {
      debug_type = make_node (LANG_TYPE);
      TYPE_NAME (debug_type) = TYPE_NAME (gnu_type);
      TYPE_ARTIFICIAL (debug_type) = TYPE_ARTIFICIAL (gnu_type);
      SET_TYPE_DEBUG_TYPE (gnu_type, debug_type);
    }

  return gnu_type;
}

/* Return the dummy type that was made for GNAT_TYPE, if any.  */

tree
get_dummy_type (Entity_Id gnat_type)
{
  return GET_DUMMY_NODE (gnat_type);
}

/* Build dummy fat and thin pointer types whose designated type is specified
   by GNAT_DESIG_TYPE/GNU_DESIG_TYPE and attach them to the latter.  */

void
build_dummy_unc_pointer_types (Entity_Id gnat_desig_type, tree gnu_desig_type)
{
  tree gnu_template_type, gnu_ptr_template, gnu_array_type, gnu_ptr_array;
  tree gnu_fat_type, fields, gnu_object_type;

  gnu_template_type = make_node (RECORD_TYPE);
  TYPE_NAME (gnu_template_type) = create_concat_name (gnat_desig_type, "XUB");
  TYPE_DUMMY_P (gnu_template_type) = 1;
  gnu_ptr_template = build_pointer_type (gnu_template_type);

  gnu_array_type = make_node (ENUMERAL_TYPE);
  TYPE_NAME (gnu_array_type) = create_concat_name (gnat_desig_type, "XUA");
  TYPE_DUMMY_P (gnu_array_type) = 1;
  gnu_ptr_array = build_pointer_type (gnu_array_type);

  gnu_fat_type = make_node (RECORD_TYPE);
  /* Build a stub DECL to trigger the special processing for fat pointer types
     in gnat_pushdecl.  */
  TYPE_NAME (gnu_fat_type)
    = create_type_stub_decl (create_concat_name (gnat_desig_type, "XUP"),
			     gnu_fat_type);
  fields = create_field_decl (get_identifier ("P_ARRAY"), gnu_ptr_array,
			      gnu_fat_type, NULL_TREE, NULL_TREE, 0, 1);
  DECL_CHAIN (fields)
    = create_field_decl (get_identifier ("P_BOUNDS"), gnu_ptr_template,
			 gnu_fat_type, NULL_TREE, NULL_TREE, 0, 1);
  finish_fat_pointer_type (gnu_fat_type, fields);
  SET_TYPE_UNCONSTRAINED_ARRAY (gnu_fat_type, gnu_desig_type);
  /* Suppress debug info until after the type is completed.  */
  TYPE_DECL_SUPPRESS_DEBUG (TYPE_STUB_DECL (gnu_fat_type)) = 1;

  gnu_object_type = make_node (RECORD_TYPE);
  TYPE_NAME (gnu_object_type) = create_concat_name (gnat_desig_type, "XUT");
  TYPE_DUMMY_P (gnu_object_type) = 1;

  TYPE_POINTER_TO (gnu_desig_type) = gnu_fat_type;
  TYPE_REFERENCE_TO (gnu_desig_type) = gnu_fat_type;
  TYPE_OBJECT_RECORD_TYPE (gnu_desig_type) = gnu_object_type;
}

/* Return true if we are in the global binding level.  */

bool
global_bindings_p (void)
{
  return force_global || !current_function_decl;
}

/* Enter a new binding level.  */

void
gnat_pushlevel (void)
{
  struct gnat_binding_level *newlevel = NULL;

  /* Reuse a struct for this binding level, if there is one.  */
  if (free_binding_level)
    {
      newlevel = free_binding_level;
      free_binding_level = free_binding_level->chain;
    }
  else
    newlevel = ggc_alloc<gnat_binding_level> ();

  /* Use a free BLOCK, if any; otherwise, allocate one.  */
  if (free_block_chain)
    {
      newlevel->block = free_block_chain;
      free_block_chain = BLOCK_CHAIN (free_block_chain);
      BLOCK_CHAIN (newlevel->block) = NULL_TREE;
    }
  else
    newlevel->block = make_node (BLOCK);

  /* Point the BLOCK we just made to its parent.  */
  if (current_binding_level)
    BLOCK_SUPERCONTEXT (newlevel->block) = current_binding_level->block;

  BLOCK_VARS (newlevel->block) = NULL_TREE;
  BLOCK_SUBBLOCKS (newlevel->block) = NULL_TREE;
  TREE_USED (newlevel->block) = 1;

  /* Add this level to the front of the chain (stack) of active levels.  */
  newlevel->chain = current_binding_level;
  current_binding_level = newlevel;
}

/* Set SUPERCONTEXT of the BLOCK for the current binding level to FNDECL
   and point FNDECL to this BLOCK.  */

void
set_current_block_context (tree fndecl)
{
  BLOCK_SUPERCONTEXT (current_binding_level->block) = fndecl;
  DECL_INITIAL (fndecl) = current_binding_level->block;
  set_block_for_group (current_binding_level->block);
}

/* Exit a binding level.  Set any BLOCK into the current code group.  */

void
gnat_poplevel (void)
{
  struct gnat_binding_level *level = current_binding_level;
  tree block = level->block;

  BLOCK_VARS (block) = nreverse (BLOCK_VARS (block));
  BLOCK_SUBBLOCKS (block) = blocks_nreverse (BLOCK_SUBBLOCKS (block));

  /* If this is a function-level BLOCK don't do anything.  Otherwise, if there
     are no variables free the block and merge its subblocks into those of its
     parent block.  Otherwise, add it to the list of its parent.  */
  if (TREE_CODE (BLOCK_SUPERCONTEXT (block)) == FUNCTION_DECL)
    ;
  else if (!BLOCK_VARS (block))
    {
      BLOCK_SUBBLOCKS (level->chain->block)
	= block_chainon (BLOCK_SUBBLOCKS (block),
			 BLOCK_SUBBLOCKS (level->chain->block));
      BLOCK_CHAIN (block) = free_block_chain;
      free_block_chain = block;
    }
  else
    {
      BLOCK_CHAIN (block) = BLOCK_SUBBLOCKS (level->chain->block);
      BLOCK_SUBBLOCKS (level->chain->block) = block;
      TREE_USED (block) = 1;
      set_block_for_group (block);
    }

  /* Free this binding structure.  */
  current_binding_level = level->chain;
  level->chain = free_binding_level;
  free_binding_level = level;
}

/* Exit a binding level and discard the associated BLOCK.  */

void
gnat_zaplevel (void)
{
  struct gnat_binding_level *level = current_binding_level;
  tree block = level->block;

  BLOCK_CHAIN (block) = free_block_chain;
  free_block_chain = block;

  /* Free this binding structure.  */
  current_binding_level = level->chain;
  level->chain = free_binding_level;
  free_binding_level = level;
}

/* Set the context of TYPE and its parallel types (if any) to CONTEXT.  */

static void
gnat_set_type_context (tree type, tree context)
{
  tree decl = TYPE_STUB_DECL (type);

  TYPE_CONTEXT (type) = context;

  while (decl && DECL_PARALLEL_TYPE (decl))
    {
      tree parallel_type = DECL_PARALLEL_TYPE (decl);

      /* Give a context to the parallel types and their stub decl, if any.
	 Some parallel types seems to be present in multiple parallel type
	 chains, so don't mess with their context if they already have one.  */
      if (!TYPE_CONTEXT (parallel_type))
	{
	  if (TYPE_STUB_DECL (parallel_type))
	    DECL_CONTEXT (TYPE_STUB_DECL (parallel_type)) = context;
	  TYPE_CONTEXT (parallel_type) = context;
	}

      decl = TYPE_STUB_DECL (DECL_PARALLEL_TYPE (decl));
    }
}

/* Return the innermost scope, starting at GNAT_NODE, we are be interested in
   the debug info, or Empty if there is no such scope.  If not NULL, set
   IS_SUBPROGRAM to whether the returned entity is a subprogram.  */

Entity_Id
get_debug_scope (Node_Id gnat_node, bool *is_subprogram)
{
  Entity_Id gnat_entity;

  if (is_subprogram)
    *is_subprogram = false;

  if (Nkind (gnat_node) == N_Defining_Identifier
      || Nkind (gnat_node) == N_Defining_Operator_Symbol)
    gnat_entity = Scope (gnat_node);
  else
    return Empty;

  while (Present (gnat_entity))
    {
      switch (Ekind (gnat_entity))
	{
	case E_Function:
	case E_Procedure:
	  if (Present (Protected_Body_Subprogram (gnat_entity)))
	    gnat_entity = Protected_Body_Subprogram (gnat_entity);

	  /* If the scope is a subprogram, then just rely on
	     current_function_decl, so that we don't have to defer
	     anything.  This is needed because other places rely on the
	     validity of the DECL_CONTEXT attribute of FUNCTION_DECL nodes. */
	  if (is_subprogram)
	    *is_subprogram = true;
	  return gnat_entity;

	case E_Record_Type:
	case E_Record_Subtype:
	  return gnat_entity;

	default:
	  /* By default, we are not interested in this particular scope: go to
	     the outer one.  */
	  break;
	}

      gnat_entity = Scope (gnat_entity);
    }

  return Empty;
}

/* If N is NULL, set TYPE's context to CONTEXT.  Defer this to the processing
   of N otherwise.  */

static void
defer_or_set_type_context (tree type, tree context,
			   struct deferred_decl_context_node *n)
{
  if (n)
    add_deferred_type_context (n, type);
  else
    gnat_set_type_context (type, context);
}

/* Return global_context, but create it first if need be.  */

static tree
get_global_context (void)
{
  if (!global_context)
    {
      global_context
	= build_translation_unit_decl (get_identifier (main_input_filename));
      debug_hooks->register_main_translation_unit (global_context);
    }

  return global_context;
}

/* Record DECL as belonging to the current lexical scope and use GNAT_NODE
   for location information and flag propagation.  */

void
gnat_pushdecl (tree decl, Node_Id gnat_node)
{
  tree context = NULL_TREE;
  struct deferred_decl_context_node *deferred_decl_context = NULL;

  /* If explicitly asked to make DECL global or if it's an imported nested
     object, short-circuit the regular Scope-based context computation.  */
  if (!((TREE_PUBLIC (decl) && DECL_EXTERNAL (decl)) || force_global == 1))
    {
      /* Rely on the GNAT scope, or fallback to the current_function_decl if
	 the GNAT scope reached the global scope, if it reached a subprogram
	 or the declaration is a subprogram or a variable (for them we skip
	 intermediate context types because the subprogram body elaboration
	 machinery and the inliner both expect a subprogram context).

	 Falling back to current_function_decl is necessary for implicit
	 subprograms created by gigi, such as the elaboration subprograms.  */
      bool context_is_subprogram = false;
      const Entity_Id gnat_scope
        = get_debug_scope (gnat_node, &context_is_subprogram);

      if (Present (gnat_scope)
	  && !context_is_subprogram
	  && TREE_CODE (decl) != FUNCTION_DECL
	  && TREE_CODE (decl) != VAR_DECL)
	/* Always assume the scope has not been elaborated, thus defer the
	   context propagation to the time its elaboration will be
	   available.  */
	deferred_decl_context
	  = add_deferred_decl_context (decl, gnat_scope, force_global);

      /* External declarations (when force_global > 0) may not be in a
	 local context.  */
      else if (current_function_decl && force_global == 0)
	context = current_function_decl;
    }

  /* If either we are forced to be in global mode or if both the GNAT scope and
     the current_function_decl did not help in determining the context, use the
     global scope.  */
  if (!deferred_decl_context && !context)
    context = get_global_context ();

  /* Mark functions really nested in another function, that is to say defined
     there as opposed to imported from elsewhere, as initially needing a static
     chain for the sake of uniformity (lower_nested_functions will recompute it
     exacly later) and as private to the translation unit (the static chain may
     be clobbered by calling conventions used across translation units).  */
  if (TREE_CODE (decl) == FUNCTION_DECL
      && !DECL_EXTERNAL (decl)
      && context
      && (TREE_CODE (context) == FUNCTION_DECL
	  || decl_function_context (context)))
    {
      DECL_STATIC_CHAIN (decl) = 1;
      TREE_PUBLIC (decl) = 0;
    }

  if (!deferred_decl_context)
    DECL_CONTEXT (decl) = context;

  /* Disable warnings for compiler-generated entities or explicit request.  */
  if (No (gnat_node)
      || !Comes_From_Source (gnat_node)
      || Warnings_Off (gnat_node))
    suppress_warning (decl);

  /* Set the location of DECL and emit a declaration for it.  */
  if (Present (gnat_node) && !renaming_from_instantiation_p (gnat_node))
    Sloc_to_locus (Sloc (gnat_node), &DECL_SOURCE_LOCATION (decl));

  add_decl_expr (decl, gnat_node);

  /* Put the declaration on the list.  The list of declarations is in reverse
     order.  The list will be reversed later.  Put global declarations in the
     globals list and local ones in the current block.  But skip TYPE_DECLs
     for UNCONSTRAINED_ARRAY_TYPE in both cases, as they will cause trouble
     with the debugger and aren't needed anyway.  */
  if (!(TREE_CODE (decl) == TYPE_DECL
        && TREE_CODE (TREE_TYPE (decl)) == UNCONSTRAINED_ARRAY_TYPE))
    {
      /* External declarations must go to the binding level they belong to.
	 This will make corresponding imported entities are available in the
	 debugger at the proper time.  */
      if (DECL_EXTERNAL (decl)
	  && TREE_CODE (decl) == FUNCTION_DECL
	  && fndecl_built_in_p (decl))
	vec_safe_push (builtin_decls, decl);
      else if (global_bindings_p ())
	vec_safe_push (global_decls, decl);
      else
	{
	  DECL_CHAIN (decl) = BLOCK_VARS (current_binding_level->block);
	  BLOCK_VARS (current_binding_level->block) = decl;
	}
    }

/* Pointer types aren't named types in the C sense so we need to generate a
   typedef in DWARF for them.  Also do that for fat pointer types because,
   even though they are named types in the C sense, they are still the XUP
   types created for the base array type at this point.  */
#define TYPE_IS_POINTER_P(NODE) \
  (TREE_CODE (NODE) == POINTER_TYPE || TYPE_IS_FAT_POINTER_P (NODE))

  /* For the declaration of a type, set its name either if it isn't already
     set or if the previous type name was not derived from a source name.
     We'd rather have the type named with a real name and all the pointer
     types to the same object have the same node, except when the names are
     both derived from source names.  */
  if (TREE_CODE (decl) == TYPE_DECL && DECL_NAME (decl))
    {
      tree t = TREE_TYPE (decl);

      /* For pointer types, make sure the typedef is generated and preserved
	 in DWARF, unless the type is artificial.  */
      if (!(TYPE_NAME (t) && TREE_CODE (TYPE_NAME (t)) == TYPE_DECL)
	  && (!TYPE_IS_POINTER_P (t) || DECL_ARTIFICIAL (decl)))
	;
      /* For pointer types, create the DECL_ORIGINAL_TYPE that will generate
	 the typedef in DWARF.  */
      else if (TYPE_IS_POINTER_P (t) && !DECL_ARTIFICIAL (decl))
	{
	  tree tt = build_variant_type_copy (t);
	  TYPE_NAME (tt) = decl;
	  defer_or_set_type_context (tt,
				     DECL_CONTEXT (decl),
				     deferred_decl_context);
	  TREE_TYPE (decl) = tt;
	  if (TYPE_NAME (t)
	      && TREE_CODE (TYPE_NAME (t)) == TYPE_DECL
	      && DECL_ORIGINAL_TYPE (TYPE_NAME (t)))
	    DECL_ORIGINAL_TYPE (decl) = DECL_ORIGINAL_TYPE (TYPE_NAME (t));
	  else
	    DECL_ORIGINAL_TYPE (decl) = t;
	  /* Remark the canonical fat pointer type as artificial.  */
	  if (TYPE_IS_FAT_POINTER_P (t))
	    TYPE_ARTIFICIAL (t) = 1;
	  t = NULL_TREE;
	}
      else if (TYPE_NAME (t)
	       && TREE_CODE (TYPE_NAME (t)) == TYPE_DECL
	       && DECL_ARTIFICIAL (TYPE_NAME (t)) && !DECL_ARTIFICIAL (decl))
	;
      else
	t = NULL_TREE;

      /* Propagate the name to all the variants, this is needed for the type
	 qualifiers machinery to work properly (see check_qualified_type).
	 Also propagate the context to them.  Note that it will be propagated
	 to all parallel types too thanks to gnat_set_type_context.  */
      if (t)
	for (t = TYPE_MAIN_VARIANT (t); t; t = TYPE_NEXT_VARIANT (t))
	  /* Skip it for pointer types to preserve the typedef.  */
	  if (!(TYPE_IS_POINTER_P (t)
		&& TYPE_NAME (t)
		&& TREE_CODE (TYPE_NAME (t)) == TYPE_DECL))
	    {
	      TYPE_NAME (t) = decl;
	      defer_or_set_type_context (t,
					 DECL_CONTEXT (decl),
					 deferred_decl_context);
	    }
    }

#undef TYPE_IS_POINTER_P
}

/* Create a record type that contains a SIZE bytes long field of TYPE with a
   starting bit position so that it is aligned to ALIGN bits, and leaving at
   least ROOM bytes free before the field.  BASE_ALIGN is the alignment the
   record is guaranteed to get.  GNAT_NODE is used for the position of the
   associated TYPE_DECL.  */

tree
make_aligning_type (tree type, unsigned int align, tree size,
		    unsigned int base_align, int room, Node_Id gnat_node)
{
  /* We will be crafting a record type with one field at a position set to be
     the next multiple of ALIGN past record'address + room bytes.  We use a
     record placeholder to express record'address.  */
  tree record_type = make_node (RECORD_TYPE);
  tree record = build0 (PLACEHOLDER_EXPR, record_type);

  tree record_addr_st
    = convert (sizetype, build_unary_op (ADDR_EXPR, NULL_TREE, record));

  /* The diagram below summarizes the shape of what we manipulate:

                    <--------- pos ---------->
                {  +------------+-------------+-----------------+
      record  =>{  |############|     ...     | field (type)    |
                {  +------------+-------------+-----------------+
		   |<-- room -->|<- voffset ->|<---- size ----->|
		   o            o
		   |            |
		   record_addr  vblock_addr

     Every length is in sizetype bytes there, except "pos" which has to be
     set as a bit position in the GCC tree for the record.  */
  tree room_st = size_int (room);
  tree vblock_addr_st = size_binop (PLUS_EXPR, record_addr_st, room_st);
  tree voffset_st, pos, field;

  tree name = TYPE_IDENTIFIER (type);

  name = concat_name (name, "ALIGN");
  TYPE_NAME (record_type) = name;

  /* Compute VOFFSET and then POS.  The next byte position multiple of some
     alignment after some address is obtained by "and"ing the alignment minus
     1 with the two's complement of the address.   */
  voffset_st = size_binop (BIT_AND_EXPR,
			   fold_build1 (NEGATE_EXPR, sizetype, vblock_addr_st),
			   size_int ((align / BITS_PER_UNIT) - 1));

  /* POS = (ROOM + VOFFSET) * BIT_PER_UNIT, in bitsizetype.  */
  pos = size_binop (MULT_EXPR,
		    convert (bitsizetype,
			     size_binop (PLUS_EXPR, room_st, voffset_st)),
                    bitsize_unit_node);

  /* Craft the GCC record representation.  We exceptionally do everything
     manually here because 1) our generic circuitry is not quite ready to
     handle the complex position/size expressions we are setting up, 2) we
     have a strong simplifying factor at hand: we know the maximum possible
     value of voffset, and 3) we have to set/reset at least the sizes in
     accordance with this maximum value anyway, as we need them to convey
     what should be "alloc"ated for this type.

     Use -1 as the 'addressable' indication for the field to prevent the
     creation of a bitfield.  We don't need one, it would have damaging
     consequences on the alignment computation, and create_field_decl would
     make one without this special argument, for instance because of the
     complex position expression.  */
  field = create_field_decl (get_identifier ("F"), type, record_type, size,
			     pos, 1, -1);
  TYPE_FIELDS (record_type) = field;

  SET_TYPE_ALIGN (record_type, base_align);
  TYPE_USER_ALIGN (record_type) = 1;

  TYPE_SIZE (record_type)
    = size_binop (PLUS_EXPR,
                  size_binop (MULT_EXPR, convert (bitsizetype, size),
                              bitsize_unit_node),
		  bitsize_int (align + room * BITS_PER_UNIT));
  TYPE_SIZE_UNIT (record_type)
    = size_binop (PLUS_EXPR, size,
		  size_int (room + align / BITS_PER_UNIT));

  SET_TYPE_MODE (record_type, BLKmode);
  relate_alias_sets (record_type, type, ALIAS_SET_COPY);

  /* Declare it now since it will never be declared otherwise.  This is
     necessary to ensure that its subtrees are properly marked.  */
  create_type_decl (name, record_type, true, false, gnat_node);

  return record_type;
}

/* Return true iff the packable types are equivalent.  */

bool
packable_type_hasher::equal (packable_type_hash *t1, packable_type_hash *t2)
{
  tree type1, type2;

  if (t1->hash != t2->hash)
    return 0;

  type1 = t1->type;
  type2 = t2->type;

  /* We consider that packable types are equivalent if they have the same name,
     size, alignment, RM size and storage order. Taking the mode into account
     is redundant since it is determined by the others.  */
  return
    TYPE_NAME (type1) == TYPE_NAME (type2)
    && TYPE_SIZE (type1) == TYPE_SIZE (type2)
    && TYPE_ALIGN (type1) == TYPE_ALIGN (type2)
    && TYPE_ADA_SIZE (type1) == TYPE_ADA_SIZE (type2)
    && TYPE_REVERSE_STORAGE_ORDER (type1) == TYPE_REVERSE_STORAGE_ORDER (type2);
}

/* Compute the hash value for the packable TYPE.  */

static hashval_t
hash_packable_type (tree type)
{
  hashval_t hashcode;

  hashcode = iterative_hash_expr (TYPE_NAME (type), 0);
  hashcode = iterative_hash_expr (TYPE_SIZE (type), hashcode);
  hashcode = iterative_hash_hashval_t (TYPE_ALIGN (type), hashcode);
  hashcode = iterative_hash_expr (TYPE_ADA_SIZE (type), hashcode);
  hashcode
    = iterative_hash_hashval_t (TYPE_REVERSE_STORAGE_ORDER (type), hashcode);

  return hashcode;
}

/* Look up the packable TYPE in the hash table and return its canonical version
   if it exists; otherwise, insert it into the hash table.  */

static tree
canonicalize_packable_type (tree type)
{
  const hashval_t hashcode = hash_packable_type (type);
  struct packable_type_hash in, *h, **slot;

  in.hash = hashcode;
  in.type = type;
  slot = packable_type_hash_table->find_slot_with_hash (&in, hashcode, INSERT);
  h = *slot;
  if (!h)
    {
      h = ggc_alloc<packable_type_hash> ();
      h->hash = hashcode;
      h->type = type;
      *slot = h;
    }

  return h->type;
}

/* TYPE is an ARRAY_TYPE that is being used as the type of a field in a packed
   record.  See if we can rewrite it as a type that has non-BLKmode, which we
   can pack tighter in the packed record.  If so, return the new type; if not,
   return the original type.  */

static tree
make_packable_array_type (tree type)
{
  const unsigned HOST_WIDE_INT size = tree_to_uhwi (TYPE_SIZE (type));
  unsigned HOST_WIDE_INT new_size;
  unsigned int new_align;

  /* No point in doing anything if the size is either zero or too large for an
     integral mode, or if the type already has non-BLKmode.  */
  if (size == 0 || size > MAX_FIXED_MODE_SIZE || TYPE_MODE (type) != BLKmode)
    return type;

  /* Punt if the component type is an aggregate type for now.  */
  if (AGGREGATE_TYPE_P (TREE_TYPE (type)))
    return type;

  tree new_type = copy_type (type);

  new_size = ceil_pow2 (size);
  new_align = MIN (new_size, BIGGEST_ALIGNMENT);
  SET_TYPE_ALIGN (new_type, new_align);

  TYPE_SIZE (new_type) = bitsize_int (new_size);
  TYPE_SIZE_UNIT (new_type) = size_int (new_size / BITS_PER_UNIT);

  SET_TYPE_MODE (new_type, mode_for_size (new_size, MODE_INT, 1).else_blk ());

  return new_type;
}

/* TYPE is a RECORD_TYPE, UNION_TYPE or QUAL_UNION_TYPE that is being used
   as the type of a field in a packed record if IN_RECORD is true, or as
   the component type of a packed array if IN_RECORD is false.  See if we
   can rewrite it either as a type that has non-BLKmode, which we can pack
   tighter in the packed record case, or as a smaller type with at most
   MAX_ALIGN alignment if the value is non-zero.  If so, return the new
   type; if not, return the original type.  */

tree
make_packable_type (tree type, bool in_record, unsigned int max_align)
{
  const unsigned HOST_WIDE_INT size = tree_to_uhwi (TYPE_SIZE (type));
  const unsigned int align = TYPE_ALIGN (type);
  unsigned HOST_WIDE_INT new_size;
  unsigned int new_align;

  /* No point in doing anything if the size is zero.  */
  if (size == 0)
    return type;

  tree new_type = make_node (TREE_CODE (type));

  /* Copy the name and flags from the old type to that of the new.
     Note that we rely on the pointer equality created here for
     TYPE_NAME to look through conversions in various places.  */
  TYPE_NAME (new_type) = TYPE_NAME (type);
  TYPE_JUSTIFIED_MODULAR_P (new_type) = TYPE_JUSTIFIED_MODULAR_P (type);
  TYPE_CONTAINS_TEMPLATE_P (new_type) = TYPE_CONTAINS_TEMPLATE_P (type);
  TYPE_REVERSE_STORAGE_ORDER (new_type) = TYPE_REVERSE_STORAGE_ORDER (type);
  if (TREE_CODE (type) == RECORD_TYPE)
    TYPE_PADDING_P (new_type) = TYPE_PADDING_P (type);

  /* If we are in a record and have a small size, set the alignment to
     try for an integral mode.  Otherwise set it to try for a smaller
     type with BLKmode.  */
  if (in_record && size <= MAX_FIXED_MODE_SIZE)
    {
      new_size = ceil_pow2 (size);
      new_align = MIN (new_size, BIGGEST_ALIGNMENT);
      SET_TYPE_ALIGN (new_type, new_align);
      /* build_aligned_type needs to be able to adjust back the alignment.  */
      TYPE_PACKED (new_type) = 0;
    }
  else
    {
      tree ada_size = TYPE_ADA_SIZE (type);

      /* Do not try to shrink the size if the RM size is not constant.  */
      if (TYPE_CONTAINS_TEMPLATE_P (type) || !tree_fits_uhwi_p (ada_size))
	return type;

      /* Round the RM size up to a unit boundary to get the minimal size
	 for a BLKmode record.  Give up if it's already the size and we
	 don't need to lower the alignment.  */
      new_size = tree_to_uhwi (ada_size);
      new_size = (new_size + BITS_PER_UNIT - 1) & -BITS_PER_UNIT;
      if (new_size == size && (max_align == 0 || align <= max_align))
	return type;

      new_align = MIN (new_size & -new_size, BIGGEST_ALIGNMENT);
      if (max_align > 0 && new_align > max_align)
	new_align = max_align;
      SET_TYPE_ALIGN (new_type, MIN (align, new_align));
      TYPE_PACKED (new_type) = 1;
    }

  TYPE_USER_ALIGN (new_type) = 1;

  /* Now copy the fields, keeping the position and size as we don't want
     to change the layout by propagating the packedness downwards.  */
  tree new_field_list = NULL_TREE;
  for (tree field = TYPE_FIELDS (type); field; field = DECL_CHAIN (field))
    {
      tree new_field_type = TREE_TYPE (field);
      tree new_field, new_field_size;

      if (AGGREGATE_TYPE_P (new_field_type)
	  && tree_fits_uhwi_p (TYPE_SIZE (new_field_type)))
	{
	  if (RECORD_OR_UNION_TYPE_P (new_field_type)
	      && !TYPE_FAT_POINTER_P (new_field_type))
	    new_field_type
	      = make_packable_type (new_field_type, true, max_align);
	  else if (in_record
		   && max_align > 0
		   && max_align < BITS_PER_UNIT
		   && TREE_CODE (new_field_type) == ARRAY_TYPE)
	    new_field_type = make_packable_array_type (new_field_type);
	}

      /* However, for the last field in a not already packed record type
	 that is of an aggregate type, we need to use the RM size in the
	 packable version of the record type, see finish_record_type.  */
      if (!DECL_CHAIN (field)
	  && !TYPE_PACKED (type)
	  && RECORD_OR_UNION_TYPE_P (new_field_type)
	  && !TYPE_FAT_POINTER_P (new_field_type)
	  && !TYPE_CONTAINS_TEMPLATE_P (new_field_type)
	  && TYPE_ADA_SIZE (new_field_type))
	new_field_size = TYPE_ADA_SIZE (new_field_type);
      else
	{
	  new_field_size = DECL_SIZE (field);

	  /* Make sure not to use too small a type for the size.  */
	  if (TYPE_MODE (new_field_type) == BLKmode)
	    new_field_type = TREE_TYPE (field);
	}

      /* This is a layout with full representation, alignment and size clauses
	 so we simply pass 0 as PACKED like gnat_to_gnu_field in this case.  */
      new_field
	= create_field_decl (DECL_NAME (field), new_field_type, new_type,
			     new_field_size, bit_position (field), 0,
			     !DECL_NONADDRESSABLE_P (field));

      DECL_INTERNAL_P (new_field) = DECL_INTERNAL_P (field);
      SET_DECL_ORIGINAL_FIELD_TO_FIELD (new_field, field);
      if (TREE_CODE (new_type) == QUAL_UNION_TYPE)
	DECL_QUALIFIER (new_field) = DECL_QUALIFIER (field);

      DECL_CHAIN (new_field) = new_field_list;
      new_field_list = new_field;
    }

  /* If this is a padding record, we never want to make the size smaller
     than what was specified.  For QUAL_UNION_TYPE, also copy the size.  */
  if (TYPE_IS_PADDING_P (type) || TREE_CODE (type) == QUAL_UNION_TYPE)
    {
      TYPE_SIZE (new_type) = TYPE_SIZE (type);
      TYPE_SIZE_UNIT (new_type) = TYPE_SIZE_UNIT (type);
      new_size = size;
    }
  else
    {
      TYPE_SIZE (new_type) = bitsize_int (new_size);
      TYPE_SIZE_UNIT (new_type) = size_int (new_size / BITS_PER_UNIT);
    }

  if (!TYPE_CONTAINS_TEMPLATE_P (type))
    SET_TYPE_ADA_SIZE (new_type, TYPE_ADA_SIZE (type));

  finish_record_type (new_type, nreverse (new_field_list), 2, false);
  relate_alias_sets (new_type, type, ALIAS_SET_COPY);
  if (gnat_encodings != DWARF_GNAT_ENCODINGS_ALL)
    SET_TYPE_DEBUG_TYPE (new_type, TYPE_DEBUG_TYPE (type));
  else if (TYPE_STUB_DECL (type))
    SET_DECL_PARALLEL_TYPE (TYPE_STUB_DECL (new_type),
			    DECL_PARALLEL_TYPE (TYPE_STUB_DECL (type)));

  /* Try harder to get a packable type if necessary, for example in case
     the record itself contains a BLKmode field.  */
  if (in_record && TYPE_MODE (new_type) == BLKmode)
    SET_TYPE_MODE (new_type,
		   mode_for_size_tree (TYPE_SIZE (new_type),
				       MODE_INT, 1).else_blk ());

  /* If neither mode nor size nor alignment shrunk, return the old type.  */
  if (TYPE_MODE (new_type) == BLKmode && new_size >= size && max_align == 0)
    return type;

  /* If the packable type is named, we canonicalize it by means of the hash
     table.  This is consistent with the language semantics and ensures that
     gigi and the middle-end have a common view of these packable types.  */
  return
    TYPE_NAME (new_type) ? canonicalize_packable_type (new_type) : new_type;
}

/* Return true if TYPE has an unsigned representation.  This needs to be used
   when the representation of types whose precision is not equal to their size
   is manipulated based on the RM size.  */

static inline bool
type_unsigned_for_rm (tree type)
{
  /* This is the common case.  */
  if (TYPE_UNSIGNED (type))
    return true;

  /* See the E_Signed_Integer_Subtype case of gnat_to_gnu_entity.  */
  if (TREE_CODE (TYPE_MIN_VALUE (type)) == INTEGER_CST
      && tree_int_cst_sgn (TYPE_MIN_VALUE (type)) >= 0)
    return true;

  return false;
}

/* Return true iff the sized types are equivalent.  */

bool
sized_type_hasher::equal (sized_type_hash *t1, sized_type_hash *t2)
{
  tree type1, type2;

  if (t1->hash != t2->hash)
    return false;

  type1 = t1->type;
  type2 = t2->type;

  /* We consider sized types equivalent if they have the same name,
     size, alignment, RM size, and biasing.  The range is not expected
     to vary across different-sized versions of the same base
     type.  */
  bool res
    = (TYPE_NAME (type1) == TYPE_NAME (type2)
       && TYPE_SIZE (type1) == TYPE_SIZE (type2)
       && TYPE_ALIGN (type1) == TYPE_ALIGN (type2)
       && TYPE_RM_SIZE (type1) == TYPE_RM_SIZE (type2)
       && (TYPE_BIASED_REPRESENTATION_P (type1)
	   == TYPE_BIASED_REPRESENTATION_P (type2)));

  gcc_assert (!res
	      || (TYPE_RM_MIN_VALUE (type1) == TYPE_RM_MIN_VALUE (type2)
		  && TYPE_RM_MAX_VALUE (type1) == TYPE_RM_MAX_VALUE (type2)));

  return res;
}

/* Compute the hash value for the sized TYPE.  */

static hashval_t
hash_sized_type (tree type)
{
  hashval_t hashcode;

  hashcode = iterative_hash_expr (TYPE_NAME (type), 0);
  hashcode = iterative_hash_expr (TYPE_SIZE (type), hashcode);
  hashcode = iterative_hash_hashval_t (TYPE_ALIGN (type), hashcode);
  hashcode = iterative_hash_expr (TYPE_RM_SIZE (type), hashcode);
  hashcode
    = iterative_hash_hashval_t (TYPE_BIASED_REPRESENTATION_P (type), hashcode);

  return hashcode;
}

/* Look up the sized TYPE in the hash table and return its canonical version
   if it exists; otherwise, insert it into the hash table.  */

static tree
canonicalize_sized_type (tree type)
{
  const hashval_t hashcode = hash_sized_type (type);
  struct sized_type_hash in, *h, **slot;

  in.hash = hashcode;
  in.type = type;
  slot = sized_type_hash_table->find_slot_with_hash (&in, hashcode, INSERT);
  h = *slot;
  if (!h)
    {
      h = ggc_alloc<sized_type_hash> ();
      h->hash = hashcode;
      h->type = type;
      *slot = h;
    }

  return h->type;
}

/* Given a type TYPE, return a new type whose size is appropriate for SIZE.
   If TYPE is the best type, return it.  Otherwise, make a new type.  We
   only support new integral and pointer types.  FOR_BIASED is true if
   we are making a biased type.  */

tree
make_type_from_size (tree type, tree size_tree, bool for_biased)
{
  unsigned HOST_WIDE_INT size;
  bool biased_p;
  tree new_type;

  /* If size indicates an error, just return TYPE to avoid propagating
     the error.  Likewise if it's too large to represent.  */
  if (!size_tree || !tree_fits_uhwi_p (size_tree))
    return type;

  size = tree_to_uhwi (size_tree);

  switch (TREE_CODE (type))
    {
    case BOOLEAN_TYPE:
      /* Do not mess with boolean types that have foreign convention.  */
      if (TYPE_PRECISION (type) == 1 && TYPE_SIZE (type) == size_tree)
	break;

      /* ... fall through ... */

    case INTEGER_TYPE:
    case ENUMERAL_TYPE:
      biased_p = (TREE_CODE (type) == INTEGER_TYPE
		  && TYPE_BIASED_REPRESENTATION_P (type));

      /* FOR_BIASED initially refers to the entity's representation,
	 not to its type's.  The type we're to return must take both
	 into account.  */
      for_biased |= biased_p;

      /* Integer types with precision 0 are forbidden.  */
      if (size == 0)
	size = 1;

      /* Only do something if the type is not a bit-packed array type and does
	 not already have the proper size and the size is not too large.  */
      if (BIT_PACKED_ARRAY_TYPE_P (type)
	  || (TYPE_PRECISION (type) == size && biased_p == for_biased)
	  || size > (Enable_128bit_Types ? 128 : LONG_LONG_TYPE_SIZE))
	break;

      /* The type should be an unsigned type if the original type is unsigned
	 or if the lower bound is constant and non-negative or if the type is
	 biased, see E_Signed_Integer_Subtype case of gnat_to_gnu_entity.  */
      if (type_unsigned_for_rm (type) || for_biased)
	new_type = make_unsigned_type (size);
      else
	new_type = make_signed_type (size);
      TREE_TYPE (new_type) = TREE_TYPE (type) ? TREE_TYPE (type) : type;
      SET_TYPE_RM_MIN_VALUE (new_type, TYPE_MIN_VALUE (type));
      SET_TYPE_RM_MAX_VALUE (new_type, TYPE_MAX_VALUE (type));
      /* Copy the name to show that it's essentially the same type and
	 not a subrange type.  */
      TYPE_NAME (new_type) = TYPE_NAME (type);
      TYPE_BIASED_REPRESENTATION_P (new_type) = for_biased;
      SET_TYPE_RM_SIZE (new_type, bitsize_int (size));

      return (TYPE_NAME (new_type)
	      ? canonicalize_sized_type (new_type)
	      : new_type);

    case RECORD_TYPE:
      /* Do something if this is a fat pointer, in which case we
	 may need to return the thin pointer.  */
      if (TYPE_FAT_POINTER_P (type) && size < POINTER_SIZE * 2)
	{
	  scalar_int_mode p_mode;
	  if (!int_mode_for_size (size, 0).exists (&p_mode)
	      || !targetm.valid_pointer_mode (p_mode))
	    p_mode = ptr_mode;
	  return
	    build_pointer_type_for_mode
	      (TYPE_OBJECT_RECORD_TYPE (TYPE_UNCONSTRAINED_ARRAY (type)),
	       p_mode, 0);
	}
      break;

    case POINTER_TYPE:
      /* Only do something if this is a thin pointer, in which case we
	 may need to return the fat pointer.  */
      if (TYPE_IS_THIN_POINTER_P (type) && size >= POINTER_SIZE * 2)
	return
	  build_pointer_type (TYPE_UNCONSTRAINED_ARRAY (TREE_TYPE (type)));
      break;

    default:
      break;
    }

  return type;
}

/* Return true iff the padded types are equivalent.  */

bool
pad_type_hasher::equal (pad_type_hash *t1, pad_type_hash *t2)
{
  tree type1, type2;

  if (t1->hash != t2->hash)
    return 0;

  type1 = t1->type;
  type2 = t2->type;

  /* We consider that padded types are equivalent if they pad the same type
     and have the same size, alignment, RM size and storage order.  Taking the
     mode into account is redundant since it is determined by the others.  */
  return
    TREE_TYPE (TYPE_FIELDS (type1)) == TREE_TYPE (TYPE_FIELDS (type2))
    && TYPE_SIZE (type1) == TYPE_SIZE (type2)
    && TYPE_ALIGN (type1) == TYPE_ALIGN (type2)
    && TYPE_ADA_SIZE (type1) == TYPE_ADA_SIZE (type2)
    && TYPE_REVERSE_STORAGE_ORDER (type1) == TYPE_REVERSE_STORAGE_ORDER (type2);
}

/* Compute the hash value for the padded TYPE.  */

static hashval_t
hash_pad_type (tree type)
{
  hashval_t hashcode;

  hashcode
    = iterative_hash_object (TYPE_HASH (TREE_TYPE (TYPE_FIELDS (type))), 0);
  hashcode = iterative_hash_expr (TYPE_SIZE (type), hashcode);
  hashcode = iterative_hash_hashval_t (TYPE_ALIGN (type), hashcode);
  hashcode = iterative_hash_expr (TYPE_ADA_SIZE (type), hashcode);
  hashcode
    = iterative_hash_hashval_t (TYPE_REVERSE_STORAGE_ORDER (type), hashcode);

  return hashcode;
}

/* Look up the padded TYPE in the hash table and return its canonical version
   if it exists; otherwise, insert it into the hash table.  */

static tree
canonicalize_pad_type (tree type)
{
  const hashval_t hashcode = hash_pad_type (type);
  struct pad_type_hash in, *h, **slot;

  in.hash = hashcode;
  in.type = type;
  slot = pad_type_hash_table->find_slot_with_hash (&in, hashcode, INSERT);
  h = *slot;
  if (!h)
    {
      h = ggc_alloc<pad_type_hash> ();
      h->hash = hashcode;
      h->type = type;
      *slot = h;
    }

  return h->type;
}

/* Ensure that TYPE has SIZE and ALIGN.  Make and return a new padded type
   if needed.  We have already verified that SIZE and ALIGN are large enough.
   GNAT_ENTITY is used to name the resulting record and to issue a warning.
   IS_COMPONENT_TYPE is true if this is being done for the component type of
   an array.  DEFINITION is true if this type is being defined.  SET_RM_SIZE
   is true if the RM size of the resulting type is to be set to SIZE too; in
   this case, the padded type is canonicalized before being returned.

   Note that, if TYPE is an array, then we pad it even if it has already got
   an alignment of ALIGN, provided that it's larger than the alignment of the
   element type.  This ensures that the size of the type is a multiple of its
   alignment as required by the GCC type system, and alleviates the oddity of
   the larger alignment, which is used to implement alignment clauses present
   on unconstrained array types.  */

tree
maybe_pad_type (tree type, tree size, unsigned int align,
		Entity_Id gnat_entity, bool is_component_type,
		bool definition, bool set_rm_size)
{
  tree orig_size = TYPE_SIZE (type);
  unsigned int orig_align
    = TREE_CODE (type) == ARRAY_TYPE
      ? TYPE_ALIGN (TREE_TYPE (type))
      : TYPE_ALIGN (type);
  tree record, field;

  /* If TYPE is a padded type, see if it agrees with any size and alignment
     we were given.  If so, return the original type.  Otherwise, strip
     off the padding, since we will either be returning the inner type
     or repadding it.  If no size or alignment is specified, use that of
     the original padded type.  */
  if (TYPE_IS_PADDING_P (type))
    {
      if ((!size
	   || operand_equal_p (round_up (size, orig_align), orig_size, 0))
	  && (align == 0 || align == orig_align))
	return type;

      if (!size)
	size = orig_size;
      if (align == 0)
	align = orig_align;

      type = TREE_TYPE (TYPE_FIELDS (type));
      orig_size = TYPE_SIZE (type);
      orig_align
	= TREE_CODE (type) == ARRAY_TYPE
	  ? TYPE_ALIGN (TREE_TYPE (type))
	  : TYPE_ALIGN (type);
    }

  /* If the size is either not being changed or is being made smaller (which
     is not done here and is only valid for bitfields anyway), show the size
     isn't changing.  Likewise, clear the alignment if it isn't being
     changed.  Then return if we aren't doing anything.  */
  if (size
      && (operand_equal_p (size, orig_size, 0)
	  || (TREE_CODE (orig_size) == INTEGER_CST
	      && tree_int_cst_lt (size, orig_size))))
    size = NULL_TREE;

  if (align == orig_align)
    align = 0;

  if (align == 0 && !size)
    return type;

  /* We used to modify the record in place in some cases, but that could
     generate incorrect debugging information.  So make a new record
     type and name.  */
  record = make_node (RECORD_TYPE);
  TYPE_PADDING_P (record) = 1;

  if (Present (gnat_entity))
    TYPE_NAME (record) = create_concat_name (gnat_entity, "PAD");

  SET_TYPE_ALIGN (record, align ? align : orig_align);
  TYPE_SIZE (record) = size ? size : orig_size;
  TYPE_SIZE_UNIT (record)
    = convert (sizetype,
	       size_binop (EXACT_DIV_EXPR, TYPE_SIZE (record),
			   bitsize_unit_node));

  /* If we are changing the alignment and the input type is a record with
     BLKmode and a small constant size, try to make a form that has an
     integral mode.  This might allow the padding record to also have an
     integral mode, which will be much more efficient.  There is no point
     in doing so if a size is specified unless it is also a small constant
     size and it is incorrect to do so if we cannot guarantee that the mode
     will be naturally aligned since the field must always be addressable.

     ??? This might not always be a win when done for a stand-alone object:
     since the nominal and the effective type of the object will now have
     different modes, a VIEW_CONVERT_EXPR will be required for converting
     between them and it might be hard to overcome afterwards, including
     at the RTL level when the stand-alone object is accessed as a whole.  */
  if (align > 0
      && RECORD_OR_UNION_TYPE_P (type)
      && !TYPE_IS_FAT_POINTER_P (type)
      && TYPE_MODE (type) == BLKmode
      && !TYPE_BY_REFERENCE_P (type)
      && TREE_CODE (orig_size) == INTEGER_CST
      && !TREE_OVERFLOW (orig_size)
      && compare_tree_int (orig_size, MAX_FIXED_MODE_SIZE) <= 0
      && (!size
	  || (TREE_CODE (size) == INTEGER_CST
	      && compare_tree_int (size, MAX_FIXED_MODE_SIZE) <= 0)))
    {
      tree packable_type = make_packable_type (type, true, align);
      if (TYPE_MODE (packable_type) != BLKmode
	  && compare_tree_int (TYPE_SIZE (packable_type), align) <= 0)
        type = packable_type;
    }

  /* Now create the field with the original size.  */
  field = create_field_decl (get_identifier ("F"), type, record, orig_size,
			     bitsize_zero_node, 0, 1);
  DECL_INTERNAL_P (field) = 1;

  /* We will output additional debug info manually below.  */
  finish_record_type (record, field, 1, false);

  /* Set the RM size if requested.  */
  if (set_rm_size)
    {
      SET_TYPE_ADA_SIZE (record, size ? size : orig_size);

      /* If the padded type is complete and has constant size, we canonicalize
	 it by means of the hash table.  This is consistent with the language
	 semantics and ensures that gigi and the middle-end have a common view
	 of these padded types.  */
      if (TREE_CONSTANT (TYPE_SIZE (record)))
	{
	  tree canonical = canonicalize_pad_type (record);
	  if (canonical != record)
	    {
	      record = canonical;
	      goto built;
	    }
	}
    }

  /* Make the inner type the debug type of the padded type.  */
  if (gnat_encodings != DWARF_GNAT_ENCODINGS_ALL)
    SET_TYPE_DEBUG_TYPE (record, maybe_debug_type (type));

  /* Unless debugging information isn't being written for the input type,
     write a record that shows what we are a subtype of and also make a
     variable that indicates our size, if still variable.  */
  if (TREE_CODE (orig_size) != INTEGER_CST
      && TYPE_NAME (record)
      && TYPE_NAME (type)
      && !(TREE_CODE (TYPE_NAME (type)) == TYPE_DECL
	   && DECL_IGNORED_P (TYPE_NAME (type))))
    {
      tree name = TYPE_IDENTIFIER (record);
      tree size_unit = TYPE_SIZE_UNIT (record);

      /* A variable that holds the size is required even with no encoding since
	 it will be referenced by debugging information attributes.  At global
	 level, we need a single variable across all translation units.  */
      if (size
	  && TREE_CODE (size) != INTEGER_CST
	  && (definition || global_bindings_p ()))
	{
	  /* Whether or not gnat_entity comes from source, this XVZ variable is
	     is a compilation artifact.  */
	  size_unit
	    = create_var_decl (concat_name (name, "XVZ"), NULL_TREE, sizetype,
			      size_unit, true, global_bindings_p (),
			      !definition && global_bindings_p (), false,
			      false, true, true, NULL, gnat_entity, false);
	  TYPE_SIZE_UNIT (record) = size_unit;
	}

      /* There is no need to show what we are a subtype of when outputting as
	 few encodings as possible: regular debugging infomation makes this
	 redundant.  */
      if (gnat_encodings == DWARF_GNAT_ENCODINGS_ALL)
	{
	  tree marker = make_node (RECORD_TYPE);
	  tree orig_name = TYPE_IDENTIFIER (type);

	  TYPE_NAME (marker) = concat_name (name, "XVS");
	  finish_record_type (marker,
			      create_field_decl (orig_name,
						 build_reference_type (type),
						 marker, NULL_TREE, NULL_TREE,
						 0, 0),
			      0, true);
	  TYPE_SIZE_UNIT (marker) = size_unit;

	  add_parallel_type (record, marker);
	}
    }

built:
  /* If a simple size was explicitly given, maybe issue a warning.  */
  if (!size
      || TREE_CODE (size) == COND_EXPR
      || TREE_CODE (size) == MAX_EXPR
      || No (gnat_entity))
    return record;

  /* But don't do it if we are just annotating types and the type is tagged or
     concurrent, since these types aren't fully laid out in this mode.  */
  if (type_annotate_only)
    {
      Entity_Id gnat_type
	= is_component_type
	  ? Component_Type (gnat_entity) : Etype (gnat_entity);

      if (Is_Tagged_Type (gnat_type) || Is_Concurrent_Type (gnat_type))
	return record;
    }

  /* Take the original size as the maximum size of the input if there was an
     unconstrained record involved and round it up to the specified alignment,
     if one was specified, but only for aggregate types.  */
  if (CONTAINS_PLACEHOLDER_P (orig_size))
    orig_size = max_size (orig_size, true);

  if (align && AGGREGATE_TYPE_P (type))
    orig_size = round_up (orig_size, align);

  if (!operand_equal_p (size, orig_size, 0)
      && !(TREE_CODE (size) == INTEGER_CST
	   && TREE_CODE (orig_size) == INTEGER_CST
	   && (TREE_OVERFLOW (size)
	       || TREE_OVERFLOW (orig_size)
	       || tree_int_cst_lt (size, orig_size))))
    {
      Node_Id gnat_error_node;

      /* For a packed array, post the message on the original array type.  */
      if (Is_Packed_Array_Impl_Type (gnat_entity))
	gnat_entity = Original_Array_Type (gnat_entity);

      if ((Ekind (gnat_entity) == E_Component
	   || Ekind (gnat_entity) == E_Discriminant)
	  && Present (Component_Clause (gnat_entity)))
	gnat_error_node = Last_Bit (Component_Clause (gnat_entity));
      else if (Has_Size_Clause (gnat_entity))
	gnat_error_node = Expression (Size_Clause (gnat_entity));
      else if (Has_Object_Size_Clause (gnat_entity))
	gnat_error_node = Expression (Object_Size_Clause (gnat_entity));
      else
	gnat_error_node = Empty;

      /* Generate message only for entities that come from source, since
	 if we have an entity created by expansion, the message will be
	 generated for some other corresponding source entity.  */
      if (Comes_From_Source (gnat_entity))
	{
	  if (is_component_type)
	    post_error_ne_tree ("component of& padded{ by ^ bits}??",
				gnat_entity, gnat_entity,
				size_diffop (size, orig_size));
	  else if (Present (gnat_error_node))
	    post_error_ne_tree ("{^ }bits of & unused??",
				gnat_error_node, gnat_entity,
				size_diffop (size, orig_size));
	}
    }

  return record;
}

/* Return true if padded TYPE was built with an RM size.  */

bool
pad_type_has_rm_size (tree type)
{
  /* This is required for the lookup.  */
  if (!TREE_CONSTANT (TYPE_SIZE (type)))
    return false;

  const hashval_t hashcode = hash_pad_type (type);
  struct pad_type_hash in, *h;

  in.hash = hashcode;
  in.type = type;
  h = pad_type_hash_table->find_with_hash (&in, hashcode);

  /* The types built with an RM size are the canonicalized ones.  */
  return h && h->type == type;
}

/* Return a copy of the padded TYPE but with reverse storage order.  */

tree
set_reverse_storage_order_on_pad_type (tree type)
{
  if (flag_checking)
    {
      /* If the inner type is not scalar then the function does nothing.  */
      tree inner_type = TREE_TYPE (TYPE_FIELDS (type));
      gcc_assert (!AGGREGATE_TYPE_P (inner_type)
		  && !VECTOR_TYPE_P (inner_type));
    }

  /* This is required for the canonicalization.  */
  gcc_assert (TREE_CONSTANT (TYPE_SIZE (type)));

  tree field = copy_node (TYPE_FIELDS (type));
  type = copy_type (type);
  DECL_CONTEXT (field) = type;
  TYPE_FIELDS (type) = field;
  TYPE_REVERSE_STORAGE_ORDER (type) = 1;
  return canonicalize_pad_type (type);
}

/* Relate the alias sets of NEW_TYPE and OLD_TYPE according to OP.
   If this is a multi-dimensional array type, do this recursively.

   OP may be
   - ALIAS_SET_COPY:     the new set is made a copy of the old one.
   - ALIAS_SET_SUPERSET: the new set is made a superset of the old one.
   - ALIAS_SET_SUBSET:   the new set is made a subset of the old one.  */

void
relate_alias_sets (tree new_type, tree old_type, enum alias_set_op op)
{
  /* Remove any padding from GNU_OLD_TYPE.  It doesn't matter in the case
     of a one-dimensional array, since the padding has the same alias set
     as the field type, but if it's a multi-dimensional array, we need to
     see the inner types.  */
  while (TREE_CODE (old_type) == RECORD_TYPE
	 && (TYPE_JUSTIFIED_MODULAR_P (old_type)
	     || TYPE_PADDING_P (old_type)))
    old_type = TREE_TYPE (TYPE_FIELDS (old_type));

  /* Unconstrained array types are deemed incomplete and would thus be given
     alias set 0.  Retrieve the underlying array type.  */
  if (TREE_CODE (old_type) == UNCONSTRAINED_ARRAY_TYPE)
    old_type = TREE_TYPE (TREE_TYPE (TYPE_FIELDS (TREE_TYPE (old_type))));
  if (TREE_CODE (new_type) == UNCONSTRAINED_ARRAY_TYPE)
    new_type = TREE_TYPE (TREE_TYPE (TYPE_FIELDS (TREE_TYPE (new_type))));

  if (TREE_CODE (new_type) == ARRAY_TYPE
      && TREE_CODE (TREE_TYPE (new_type)) == ARRAY_TYPE
      && TYPE_MULTI_ARRAY_P (TREE_TYPE (new_type)))
    relate_alias_sets (TREE_TYPE (new_type), TREE_TYPE (old_type), op);

  switch (op)
    {
    case ALIAS_SET_COPY:
      /* The alias set shouldn't be copied between array types with different
	 aliasing settings because this can break the aliasing relationship
	 between the array type and its element type.  */
      if (flag_checking || flag_strict_aliasing)
	gcc_assert (!(TREE_CODE (new_type) == ARRAY_TYPE
		      && TREE_CODE (old_type) == ARRAY_TYPE
		      && TYPE_NONALIASED_COMPONENT (new_type)
			 != TYPE_NONALIASED_COMPONENT (old_type)));

      /* The alias set is a property of the TYPE_CANONICAL if it exists.  */
      if (TYPE_STRUCTURAL_EQUALITY_P (new_type))
	TYPE_ALIAS_SET (new_type) = get_alias_set (old_type);
      else
	TYPE_ALIAS_SET (TYPE_CANONICAL (new_type)) = get_alias_set (old_type);
      break;

    case ALIAS_SET_SUBSET:
    case ALIAS_SET_SUPERSET:
      {
	alias_set_type old_set = get_alias_set (old_type);
	alias_set_type new_set = get_alias_set (new_type);

	/* Do nothing if the alias sets conflict.  This ensures that we
	   never call record_alias_subset several times for the same pair
	   or at all for alias set 0.  */
	if (!alias_sets_conflict_p (old_set, new_set))
	  {
	    if (op == ALIAS_SET_SUBSET)
	      record_alias_subset (old_set, new_set);
	    else
	      record_alias_subset (new_set, old_set);
	  }
      }
      break;

    default:
      gcc_unreachable ();
    }

  record_component_aliases (new_type);
}

/* Record TYPE as a builtin type for Ada.  NAME is the name of the type.
   ARTIFICIAL_P is true if the type was generated by the compiler.  */

void
record_builtin_type (const char *name, tree type, bool artificial_p)
{
  tree type_decl = build_decl (input_location,
			       TYPE_DECL, get_identifier (name), type);
  DECL_ARTIFICIAL (type_decl) = artificial_p;
  DECL_NAMELESS (type_decl) = (artificial_p
			       && gnat_encodings != DWARF_GNAT_ENCODINGS_ALL);
  TYPE_ARTIFICIAL (type) = artificial_p;
  gnat_pushdecl (type_decl, Empty);

  if (debug_hooks->type_decl)
    debug_hooks->type_decl (type_decl, false);
}

/* Finish constructing the character type CHAR_TYPE.

  In Ada character types are enumeration types and, as a consequence, are
  represented in the front-end by integral types holding the positions of
  the enumeration values as defined by the language, which means that the
  integral types are unsigned.

  Unfortunately the signedness of 'char' in C is implementation-defined
  and GCC even has the option -f[un]signed-char to toggle it at run time.
  Since GNAT's philosophy is to be compatible with C by default, to wit
  Interfaces.C.char is defined as a mere copy of Character, we may need
  to declare character types as signed types in GENERIC and generate the
  necessary adjustments to make them behave as unsigned types.

  The overall strategy is as follows: if 'char' is unsigned, do nothing;
  if 'char' is signed, translate character types of CHAR_TYPE_SIZE and
  character subtypes with RM_Size = Esize = CHAR_TYPE_SIZE into signed
  types.  The idea is to ensure that the bit pattern contained in the
  Esize'd objects is not changed, even though the numerical value will
  be interpreted differently depending on the signedness.  */

void
finish_character_type (tree char_type)
{
  if (TYPE_UNSIGNED (char_type))
    return;

  /* Make a copy of a generic unsigned version since we'll modify it.  */
  tree unsigned_char_type
    = (char_type == char_type_node
       ? unsigned_char_type_node
       : copy_type (gnat_unsigned_type_for (char_type)));

  /* Create an unsigned version of the type and set it as debug type.  */
  TYPE_NAME (unsigned_char_type) = TYPE_NAME (char_type);
  TYPE_STRING_FLAG (unsigned_char_type) = TYPE_STRING_FLAG (char_type);
  TYPE_ARTIFICIAL (unsigned_char_type) = TYPE_ARTIFICIAL (char_type);
  SET_TYPE_DEBUG_TYPE (char_type, unsigned_char_type);

  /* If this is a subtype, make the debug type a subtype of the debug type
     of the base type and convert literal RM bounds to unsigned.  */
  if (TREE_TYPE (char_type))
    {
      tree base_unsigned_char_type = TYPE_DEBUG_TYPE (TREE_TYPE (char_type));
      tree min_value = TYPE_RM_MIN_VALUE (char_type);
      tree max_value = TYPE_RM_MAX_VALUE (char_type);

      if (TREE_CODE (min_value) == INTEGER_CST)
	min_value = fold_convert (base_unsigned_char_type, min_value);
      if (TREE_CODE (max_value) == INTEGER_CST)
	max_value = fold_convert (base_unsigned_char_type, max_value);

      TREE_TYPE (unsigned_char_type) = base_unsigned_char_type;
      SET_TYPE_RM_MIN_VALUE (unsigned_char_type, min_value);
      SET_TYPE_RM_MAX_VALUE (unsigned_char_type, max_value);
    }

  /* Adjust the RM bounds of the original type to unsigned; that's especially
     important for types since they are implicit in this case.  */
  SET_TYPE_RM_MIN_VALUE (char_type, TYPE_MIN_VALUE (unsigned_char_type));
  SET_TYPE_RM_MAX_VALUE (char_type, TYPE_MAX_VALUE (unsigned_char_type));
}

/* Given a record type RECORD_TYPE and a list of FIELD_DECL nodes FIELD_LIST,
   finish constructing the record type as a fat pointer type.  */

void
finish_fat_pointer_type (tree record_type, tree field_list)
{
  /* Make sure we can put it into a register.  */
  if (STRICT_ALIGNMENT)
    SET_TYPE_ALIGN (record_type, MIN (BIGGEST_ALIGNMENT, 2 * POINTER_SIZE));

  /* Show what it really is.  */
  TYPE_FAT_POINTER_P (record_type) = 1;

  /* Do not emit debug info for it since the types of its fields may still be
     incomplete at this point.  */
  finish_record_type (record_type, field_list, 0, false);

  /* Force type_contains_placeholder_p to return true on it.  Although the
     PLACEHOLDER_EXPRs are referenced only indirectly, this isn't a pointer
     type but the representation of the unconstrained array.  */
  TYPE_CONTAINS_PLACEHOLDER_INTERNAL (record_type) = 2;
}

/* Clear DECL_BIT_FIELD flag and associated markers on FIELD, which is a field
   of aggregate type TYPE.  */

static void
clear_decl_bit_field (tree field, tree type)
{
  DECL_BIT_FIELD (field) = 0;
  DECL_BIT_FIELD_TYPE (field) = NULL_TREE;

  /* DECL_BIT_FIELD_REPRESENTATIVE is not defined for QUAL_UNION_TYPE since
     it uses the same slot as DECL_QUALIFIER.  */
  if (TREE_CODE (type) != QUAL_UNION_TYPE)
    DECL_BIT_FIELD_REPRESENTATIVE (field) = NULL_TREE;
}

/* Given a record type RECORD_TYPE and a list of FIELD_DECL nodes FIELD_LIST,
   finish constructing the record or union type.  If REP_LEVEL is zero, this
   record has no representation clause and so will be entirely laid out here.
   If REP_LEVEL is one, this record has a representation clause and has been
   laid out already; only set the sizes and alignment.  If REP_LEVEL is two,
   this record is derived from a parent record and thus inherits its layout;
   only make a pass on the fields to finalize them.  DEBUG_INFO_P is true if
   additional debug info needs to be output for this type.  */

void
finish_record_type (tree record_type, tree field_list, int rep_level,
		    bool debug_info_p)
{
  const enum tree_code orig_code = TREE_CODE (record_type);
  const bool had_size = TYPE_SIZE (record_type) != NULL_TREE;
  const bool had_align = TYPE_ALIGN (record_type) > 0;
  /* For all-repped records with a size specified, lay the QUAL_UNION_TYPE
     out just like a UNION_TYPE, since the size will be fixed.  */
  const enum tree_code code
    = (orig_code == QUAL_UNION_TYPE && rep_level > 0 && had_size
       ? UNION_TYPE : orig_code);
  tree name = TYPE_IDENTIFIER (record_type);
  tree ada_size = bitsize_zero_node;
  tree size = bitsize_zero_node;
  tree field;

  TYPE_FIELDS (record_type) = field_list;

  /* Always attach the TYPE_STUB_DECL for a record type.  It is required to
     generate debug info and have a parallel type.  */
  TYPE_STUB_DECL (record_type) = create_type_stub_decl (name, record_type);

  /* Globally initialize the record first.  If this is a rep'ed record,
     that just means some initializations; otherwise, layout the record.  */
  if (rep_level > 0)
    {
      if (TYPE_ALIGN (record_type) < BITS_PER_UNIT)
	SET_TYPE_ALIGN (record_type, BITS_PER_UNIT);

      if (!had_size)
	TYPE_SIZE (record_type) = bitsize_zero_node;
    }
  else
    {
      /* Ensure there isn't a size already set.  There can be in an error
	 case where there is a rep clause but all fields have errors and
	 no longer have a position.  */
      TYPE_SIZE (record_type) = NULL_TREE;

      /* Ensure we use the traditional GCC layout for bitfields when we need
	 to pack the record type or have a representation clause.  The other
	 possible layout (Microsoft C compiler), if available, would prevent
	 efficient packing in almost all cases.  */
#ifdef TARGET_MS_BITFIELD_LAYOUT
      if (TARGET_MS_BITFIELD_LAYOUT && TYPE_PACKED (record_type))
	decl_attributes (&record_type,
			 tree_cons (get_identifier ("gcc_struct"),
				    NULL_TREE, NULL_TREE),
			 ATTR_FLAG_TYPE_IN_PLACE);
#endif

      layout_type (record_type);
    }

  /* At this point, the position and size of each field is known.  It was
     either set before entry by a rep clause, or by laying out the type above.

     We now run a pass over the fields (in reverse order for QUAL_UNION_TYPEs)
     to compute the Ada size; the GCC size and alignment (for rep'ed records
     that are not padding types); and the mode (for rep'ed records).  We also
     clear the DECL_BIT_FIELD indication for the cases we know have not been
     handled yet, and adjust DECL_NONADDRESSABLE_P accordingly.  */

  if (code == QUAL_UNION_TYPE)
    field_list = nreverse (field_list);

  for (field = field_list; field; field = DECL_CHAIN (field))
    {
      tree type = TREE_TYPE (field);
      tree pos = bit_position (field);
      tree this_size = DECL_SIZE (field);
      tree this_ada_size;

      if (RECORD_OR_UNION_TYPE_P (type)
	  && !TYPE_FAT_POINTER_P (type)
	  && !TYPE_CONTAINS_TEMPLATE_P (type)
	  && TYPE_ADA_SIZE (type))
	this_ada_size = TYPE_ADA_SIZE (type);
      else
	this_ada_size = this_size;

      const bool variant_part = (TREE_CODE (type) == QUAL_UNION_TYPE);

      /* Clear DECL_BIT_FIELD for the cases layout_decl does not handle.  */
      if (DECL_BIT_FIELD (field)
	  && operand_equal_p (this_size, TYPE_SIZE (type), 0))
	{
	  const unsigned int align = default_field_alignment (field, type);

	  /* In the general case, type alignment is required.  */
	  if (value_factor_p (pos, align))
	    {
	      /* The enclosing record type must be sufficiently aligned.
		 Otherwise, if no alignment was specified for it and it
		 has been laid out already, bump its alignment to the
		 desired one if this is compatible with its size and
		 maximum alignment, if any.  */
	      if (TYPE_ALIGN (record_type) >= align)
		{
		  SET_DECL_ALIGN (field, MAX (DECL_ALIGN (field), align));
		  clear_decl_bit_field (field, record_type);
		}
	      else if (!had_align
		       && rep_level == 0
		       && value_factor_p (TYPE_SIZE (record_type), align)
		       && (!TYPE_MAX_ALIGN (record_type)
			   || TYPE_MAX_ALIGN (record_type) >= align))
		{
		  SET_TYPE_ALIGN (record_type, align);
		  SET_DECL_ALIGN (field, MAX (DECL_ALIGN (field), align));
		  clear_decl_bit_field (field, record_type);
		}
	    }

	  /* In the non-strict alignment case, only byte alignment is.  */
	  if (!STRICT_ALIGNMENT
	      && DECL_BIT_FIELD (field)
	      && value_factor_p (pos, BITS_PER_UNIT))
	    clear_decl_bit_field (field, record_type);
	}

      /* Clear DECL_BIT_FIELD_TYPE for a variant part at offset 0, it's simply
	 not supported by the DECL_BIT_FIELD_REPRESENTATIVE machinery because
	 the variant part is always the last field in the list.  */
      if (variant_part && integer_zerop (pos))
	DECL_BIT_FIELD_TYPE (field) = NULL_TREE;

      /* If we still have DECL_BIT_FIELD set at this point, we know that the
	 field is technically not addressable.  Except that it can actually
	 be addressed if it is BLKmode and happens to be properly aligned.  */
      if (DECL_BIT_FIELD (field)
	  && !(DECL_MODE (field) == BLKmode
	       && value_factor_p (pos, BITS_PER_UNIT)))
	DECL_NONADDRESSABLE_P (field) = 1;

      /* A type must be as aligned as its most aligned field that is not
	 a bit-field.  But this is already enforced by layout_type.  */
      if (rep_level > 0 && !DECL_BIT_FIELD (field))
	SET_TYPE_ALIGN (record_type,
			MAX (TYPE_ALIGN (record_type), DECL_ALIGN (field)));

      switch (code)
	{
	case UNION_TYPE:
	  ada_size = size_binop (MAX_EXPR, ada_size, this_ada_size);
	  size = size_binop (MAX_EXPR, size, this_size);
	  break;

	case QUAL_UNION_TYPE:
	  ada_size
	    = fold_build3 (COND_EXPR, bitsizetype, DECL_QUALIFIER (field),
			   this_ada_size, ada_size);
	  size = fold_build3 (COND_EXPR, bitsizetype, DECL_QUALIFIER (field),
			      this_size, size);
	  break;

	case RECORD_TYPE:
	  /* Since we know here that all fields are sorted in order of
	     increasing bit position, the size of the record is one
	     higher than the ending bit of the last field processed
	     unless we have a rep clause, because we might be processing
	     the REP part of a record with a variant part for which the
	     variant part has a rep clause but not the fixed part, in
	     which case this REP part may contain overlapping fields
	     and thus needs to be treated like a union tyoe above, so
	     use a MAX in that case.  Also, if this field is a variant
	     part, we need to take into account the previous size in
	     the case of empty variants.  */
	  ada_size
	    = merge_sizes (ada_size, pos, this_ada_size, rep_level > 0,
			   variant_part);
	  size
	    = merge_sizes (size, pos, this_size, rep_level > 0, variant_part);
	  break;

	default:
	  gcc_unreachable ();
	}
    }

  if (code == QUAL_UNION_TYPE)
    nreverse (field_list);

  /* We need to set the regular sizes if REP_LEVEL is one.  */
  if (rep_level == 1)
    {
      /* We round TYPE_SIZE and TYPE_SIZE_UNIT up to TYPE_ALIGN separately
	 to avoid having very large masking constants in TYPE_SIZE_UNIT.  */
      const unsigned int align = TYPE_ALIGN (record_type);

      /* If this is a padding record, we never want to make the size smaller
	 than what was specified in it, if any.  */
      if (TYPE_IS_PADDING_P (record_type) && had_size)
	size = round_up (TYPE_SIZE (record_type), BITS_PER_UNIT);
      else
	size = round_up (size, BITS_PER_UNIT);

      TYPE_SIZE (record_type) = variable_size (round_up (size, align));

      tree size_unit
	= convert (sizetype,
		   size_binop (EXACT_DIV_EXPR, size, bitsize_unit_node));
      TYPE_SIZE_UNIT (record_type)
	= variable_size (round_up (size_unit, align / BITS_PER_UNIT));
    }

  /* We need to set the Ada size if REP_LEVEL is zero or one.  */
  if (rep_level < 2)
    {
      /* Now set any of the values we've just computed that apply.  */
      if (!TYPE_FAT_POINTER_P (record_type)
	  && !TYPE_CONTAINS_TEMPLATE_P (record_type))
	SET_TYPE_ADA_SIZE (record_type, ada_size);
    }

  /* We need to set the mode if REP_LEVEL is one or two.  */
  if (rep_level > 0)
    {
      compute_record_mode (record_type);
      finish_bitfield_layout (record_type);
    }

  /* Reset the TYPE_MAX_ALIGN field since it's private to gigi.  */
  TYPE_MAX_ALIGN (record_type) = 0;

  if (debug_info_p)
    rest_of_record_type_compilation (record_type);
}

/* Append PARALLEL_TYPE on the chain of parallel types of TYPE.  If
   PARRALEL_TYPE has no context and its computation is not deferred yet, also
   propagate TYPE's context to PARALLEL_TYPE's or defer its propagation to the
   moment TYPE will get a context.  */

void
add_parallel_type (tree type, tree parallel_type)
{
  tree decl = TYPE_STUB_DECL (type);

  while (DECL_PARALLEL_TYPE (decl))
    decl = TYPE_STUB_DECL (DECL_PARALLEL_TYPE (decl));

  SET_DECL_PARALLEL_TYPE (decl, parallel_type);

  /* If PARALLEL_TYPE already has a context, we are done.  */
  if (TYPE_CONTEXT (parallel_type))
    return;

  /* Otherwise, try to get one from TYPE's context.  If so, simply propagate
     it to PARALLEL_TYPE.  */
  if (TYPE_CONTEXT (type))
    gnat_set_type_context (parallel_type, TYPE_CONTEXT (type));

  /* Otherwise TYPE has not context yet.  We know it will have one thanks to
     gnat_pushdecl and then its context will be propagated to PARALLEL_TYPE,
     so we have nothing to do in this case.  */
}

/* Return true if TYPE has a parallel type.  */

static bool
has_parallel_type (tree type)
{
  tree decl = TYPE_STUB_DECL (type);

  return DECL_PARALLEL_TYPE (decl) != NULL_TREE;
}

/* Wrap up compilation of RECORD_TYPE, i.e. output additional debug info
   associated with it.  It need not be invoked directly in most cases as
   finish_record_type takes care of doing so.  */

void
rest_of_record_type_compilation (tree record_type)
{
  bool var_size = false;
  tree field;

  /* If this is a padded type, the bulk of the debug info has already been
     generated for the field's type.  */
  if (TYPE_IS_PADDING_P (record_type))
    return;

  /* If the type already has a parallel type (XVS type), then we're done.  */
  if (has_parallel_type (record_type))
    return;

  for (field = TYPE_FIELDS (record_type); field; field = DECL_CHAIN (field))
    {
      /* We need to make an XVE/XVU record if any field has variable size,
	 whether or not the record does.  For example, if we have a union,
	 it may be that all fields, rounded up to the alignment, have the
	 same size, in which case we'll use that size.  But the debug
	 output routines (except Dwarf2) won't be able to output the fields,
	 so we need to make the special record.  */
      if (TREE_CODE (DECL_SIZE (field)) != INTEGER_CST
	  /* If a field has a non-constant qualifier, the record will have
	     variable size too.  */
	  || (TREE_CODE (record_type) == QUAL_UNION_TYPE
	      && TREE_CODE (DECL_QUALIFIER (field)) != INTEGER_CST))
	{
	  var_size = true;
	  break;
	}
    }

  /* If this record type is of variable size, make a parallel record type that
     will tell the debugger how the former is laid out (see exp_dbug.ads).  */
  if (var_size && gnat_encodings == DWARF_GNAT_ENCODINGS_ALL)
    {
      tree new_record_type
	= make_node (TREE_CODE (record_type) == QUAL_UNION_TYPE
		     ? UNION_TYPE : TREE_CODE (record_type));
      tree orig_name = TYPE_IDENTIFIER (record_type), new_name;
      tree last_pos = bitsize_zero_node;

      new_name
	= concat_name (orig_name, TREE_CODE (record_type) == QUAL_UNION_TYPE
				  ? "XVU" : "XVE");
      TYPE_NAME (new_record_type) = new_name;
      SET_TYPE_ALIGN (new_record_type, BIGGEST_ALIGNMENT);
      TYPE_STUB_DECL (new_record_type)
	= create_type_stub_decl (new_name, new_record_type);
      DECL_IGNORED_P (TYPE_STUB_DECL (new_record_type))
	= DECL_IGNORED_P (TYPE_STUB_DECL (record_type));
      gnat_pushdecl (TYPE_STUB_DECL (new_record_type), Empty);
      TYPE_SIZE (new_record_type) = size_int (TYPE_ALIGN (record_type));
      TYPE_SIZE_UNIT (new_record_type)
	= size_int (TYPE_ALIGN (record_type) / BITS_PER_UNIT);

      /* Now scan all the fields, replacing each field with a new field
	 corresponding to the new encoding.  */
      for (tree old_field = TYPE_FIELDS (record_type);
	   old_field;
	   old_field = DECL_CHAIN (old_field))
	{
	  tree field_type = TREE_TYPE (old_field);
	  tree field_name = DECL_NAME (old_field);
	  tree curpos = fold_bit_position (old_field);
	  tree pos, new_field;
	  bool var = false;
	  unsigned int align = 0;

	  /* See how the position was modified from the last position.

	     There are two basic cases we support: a value was added
	     to the last position or the last position was rounded to
	     a boundary and they something was added.  Check for the
	     first case first.  If not, see if there is any evidence
	     of rounding.  If so, round the last position and retry.

	     If this is a union, the position can be taken as zero.  */
	  if (TREE_CODE (new_record_type) == UNION_TYPE)
	    pos = bitsize_zero_node;
	  else
	    pos = compute_related_constant (curpos, last_pos);

	  if (pos)
	    ;
	  else if (TREE_CODE (curpos) == MULT_EXPR
		   && tree_fits_uhwi_p (TREE_OPERAND (curpos, 1)))
	    {
	      tree offset = TREE_OPERAND (curpos, 0);
	      align = tree_to_uhwi (TREE_OPERAND (curpos, 1));
	      align = scale_by_factor_of (offset, align);
	      last_pos = round_up (last_pos, align);
	      pos = compute_related_constant (curpos, last_pos);
	    }
	  else if (TREE_CODE (curpos) == PLUS_EXPR
		   && tree_fits_uhwi_p (TREE_OPERAND (curpos, 1))
		   && TREE_CODE (TREE_OPERAND (curpos, 0)) == MULT_EXPR
		   && tree_fits_uhwi_p
		      (TREE_OPERAND (TREE_OPERAND (curpos, 0), 1)))
	    {
	      tree offset = TREE_OPERAND (TREE_OPERAND (curpos, 0), 0);
	      unsigned HOST_WIDE_INT addend
	        = tree_to_uhwi (TREE_OPERAND (curpos, 1));
	      align
		= tree_to_uhwi (TREE_OPERAND (TREE_OPERAND (curpos, 0), 1));
	      align = scale_by_factor_of (offset, align);
	      align = MIN (align, addend & -addend);
	      last_pos = round_up (last_pos, align);
	      pos = compute_related_constant (curpos, last_pos);
	    }
	  else
	    {
	      align = DECL_ALIGN (old_field);
	      last_pos = round_up (last_pos, align);
	      pos = compute_related_constant (curpos, last_pos);
	    }

	  /* See if this type is variable-sized and make a pointer type
	     and indicate the indirection if so.  Beware that the debug
	     back-end may adjust the position computed above according
	     to the alignment of the field type, i.e. the pointer type
	     in this case, if we don't preventively counter that.  */
	  if (TREE_CODE (DECL_SIZE (old_field)) != INTEGER_CST)
	    {
	      field_type = copy_type (build_pointer_type (field_type));
	      SET_TYPE_ALIGN (field_type, BITS_PER_UNIT);
	      var = true;

	      /* ??? Kludge to work around a bug in Workbench's debugger.  */
	      if (align == 0)
		{
		  align = DECL_ALIGN (old_field);
		  last_pos = round_up (last_pos, align);
		  pos = compute_related_constant (curpos, last_pos);
		}
	    }

	  /* If we can't compute a position, set it to zero.

	     ??? We really should abort here, but it's too much work
	     to get this correct for all cases.  */
	  if (!pos)
	    pos = bitsize_zero_node;

	  /* Make a new field name, if necessary.  */
	  if (var || align != 0)
	    {
	      char suffix[16];

	      if (align != 0)
		sprintf (suffix, "XV%c%u", var ? 'L' : 'A',
			 align / BITS_PER_UNIT);
	      else
		strcpy (suffix, "XVL");

	      field_name = concat_name (field_name, suffix);
	    }

	  new_field
	    = create_field_decl (field_name, field_type, new_record_type,
				 DECL_SIZE (old_field), pos, 0, 0);
	  /* The specified position is not the actual position of the field
	     but the gap with the previous field, so the computation of the
	     bit-field status may be incorrect.  We adjust it manually to
	     avoid generating useless attributes for the field in DWARF.  */
	  if (DECL_SIZE (old_field) == TYPE_SIZE (field_type)
	      && value_factor_p (pos, BITS_PER_UNIT))
	    clear_decl_bit_field (new_field, new_record_type);
	  DECL_CHAIN (new_field) = TYPE_FIELDS (new_record_type);
	  TYPE_FIELDS (new_record_type) = new_field;

	  /* If old_field is a QUAL_UNION_TYPE, take its size as being
	     zero.  The only time it's not the last field of the record
	     is when there are other components at fixed positions after
	     it (meaning there was a rep clause for every field) and we
	     want to be able to encode them.  */
	  last_pos = size_binop (PLUS_EXPR, curpos,
				 (TREE_CODE (TREE_TYPE (old_field))
				  == QUAL_UNION_TYPE)
				 ? bitsize_zero_node
				 : DECL_SIZE (old_field));
	}

      TYPE_FIELDS (new_record_type) = nreverse (TYPE_FIELDS (new_record_type));

      add_parallel_type (record_type, new_record_type);
    }
}

/* Utility function of above to merge LAST_SIZE, the previous size of a record
   with FIRST_BIT and SIZE that describe a field.  If MAX is true, we take the
   MAX of the end position of this field with LAST_SIZE.  In all other cases,
   we use FIRST_BIT plus SIZE.  SPECIAL is true if it's for a QUAL_UNION_TYPE,
   in which case we must look for COND_EXPRs and replace a value of zero with
   the old size.  Return an expression for the size.  */

static tree
merge_sizes (tree last_size, tree first_bit, tree size, bool max, bool special)
{
  tree type = TREE_TYPE (last_size);
  tree new_size;

  if (!special || TREE_CODE (size) != COND_EXPR)
    {
      new_size = size_binop (PLUS_EXPR, first_bit, size);
      if (max)
	new_size = size_binop (MAX_EXPR, last_size, new_size);
    }

  else
    new_size = fold_build3 (COND_EXPR, type, TREE_OPERAND (size, 0),
			    integer_zerop (TREE_OPERAND (size, 1))
			    ? last_size : merge_sizes (last_size, first_bit,
						       TREE_OPERAND (size, 1),
						       max, special),
			    integer_zerop (TREE_OPERAND (size, 2))
			    ? last_size : merge_sizes (last_size, first_bit,
						       TREE_OPERAND (size, 2),
						       max, special));

  /* We don't need any NON_VALUE_EXPRs and they can confuse us (especially
     when fed through SUBSTITUTE_IN_EXPR) into thinking that a constant
     size is not constant.  */
  while (TREE_CODE (new_size) == NON_LVALUE_EXPR)
    new_size = TREE_OPERAND (new_size, 0);

  return new_size;
}

/* Convert the size expression EXPR to TYPE and fold the result.  */

static tree
fold_convert_size (tree type, tree expr)
{
  /* We assume that size expressions do not wrap around.  */
  if (TREE_CODE (expr) == MULT_EXPR || TREE_CODE (expr) == PLUS_EXPR)
    return size_binop (TREE_CODE (expr),
		       fold_convert_size (type, TREE_OPERAND (expr, 0)),
		       fold_convert_size (type, TREE_OPERAND (expr, 1)));

  return fold_convert (type, expr);
}

/* Return the bit position of FIELD, in bits from the start of the record,
   and fold it as much as possible.  This is a tree of type bitsizetype.  */

static tree
fold_bit_position (const_tree field)
{
  tree offset = fold_convert_size (bitsizetype, DECL_FIELD_OFFSET (field));
  return size_binop (PLUS_EXPR, DECL_FIELD_BIT_OFFSET (field),
 		     size_binop (MULT_EXPR, offset, bitsize_unit_node));
}

/* Utility function of above to see if OP0 and OP1, both of SIZETYPE, are
   related by the addition of a constant.  Return that constant if so.  */

static tree
compute_related_constant (tree op0, tree op1)
{
  tree factor, op0_var, op1_var, op0_cst, op1_cst, result;

  if (TREE_CODE (op0) == MULT_EXPR
      && TREE_CODE (op1) == MULT_EXPR
      && TREE_CODE (TREE_OPERAND (op0, 1)) == INTEGER_CST
      && TREE_OPERAND (op1, 1) == TREE_OPERAND (op0, 1))
    {
      factor = TREE_OPERAND (op0, 1);
      op0 = TREE_OPERAND (op0, 0);
      op1 = TREE_OPERAND (op1, 0);
    }
  else
    factor = NULL_TREE;

  op0_cst = split_plus (op0, &op0_var);
  op1_cst = split_plus (op1, &op1_var);
  result = size_binop (MINUS_EXPR, op0_cst, op1_cst);

  if (operand_equal_p (op0_var, op1_var, 0))
    return factor ? size_binop (MULT_EXPR, factor, result) : result;

  return NULL_TREE;
}

/* Utility function of above to split a tree OP which may be a sum, into a
   constant part, which is returned, and a variable part, which is stored
   in *PVAR.  *PVAR may be bitsize_zero_node.  All operations must be of
   bitsizetype.  */

static tree
split_plus (tree in, tree *pvar)
{
  /* Strip conversions in order to ease the tree traversal and maximize the
     potential for constant or plus/minus discovery.  We need to be careful
     to always return and set *pvar to bitsizetype trees, but it's worth
     the effort.  */
  in = remove_conversions (in, false);

  *pvar = convert (bitsizetype, in);

  if (TREE_CODE (in) == INTEGER_CST)
    {
      *pvar = bitsize_zero_node;
      return convert (bitsizetype, in);
    }
  else if (TREE_CODE (in) == PLUS_EXPR || TREE_CODE (in) == MINUS_EXPR)
    {
      tree lhs_var, rhs_var;
      tree lhs_con = split_plus (TREE_OPERAND (in, 0), &lhs_var);
      tree rhs_con = split_plus (TREE_OPERAND (in, 1), &rhs_var);

      if (lhs_var == TREE_OPERAND (in, 0)
	  && rhs_var == TREE_OPERAND (in, 1))
	return bitsize_zero_node;

      *pvar = size_binop (TREE_CODE (in), lhs_var, rhs_var);
      return size_binop (TREE_CODE (in), lhs_con, rhs_con);
    }
  else
    return bitsize_zero_node;
}

/* Return a copy of TYPE but safe to modify in any way.  */

tree
copy_type (tree type)
{
  tree new_type = copy_node (type);

  /* Unshare the language-specific data.  */
  if (TYPE_LANG_SPECIFIC (type))
    {
      TYPE_LANG_SPECIFIC (new_type) = NULL;
      SET_TYPE_LANG_SPECIFIC (new_type, GET_TYPE_LANG_SPECIFIC (type));
    }

  /* And the contents of the language-specific slot if needed.  */
  if ((INTEGRAL_TYPE_P (type) || SCALAR_FLOAT_TYPE_P (type))
      && TYPE_RM_VALUES (type))
    {
      TYPE_RM_VALUES (new_type) = NULL_TREE;
      SET_TYPE_RM_SIZE (new_type, TYPE_RM_SIZE (type));
      SET_TYPE_RM_MIN_VALUE (new_type, TYPE_RM_MIN_VALUE (type));
      SET_TYPE_RM_MAX_VALUE (new_type, TYPE_RM_MAX_VALUE (type));
    }

  /* copy_node clears this field instead of copying it, because it is
     aliased with TREE_CHAIN.  */
  TYPE_STUB_DECL (new_type) = TYPE_STUB_DECL (type);

  TYPE_POINTER_TO (new_type) = NULL_TREE;
  TYPE_REFERENCE_TO (new_type) = NULL_TREE;
  TYPE_MAIN_VARIANT (new_type) = new_type;
  TYPE_NEXT_VARIANT (new_type) = NULL_TREE;
  TYPE_CANONICAL (new_type) = new_type;

  return new_type;
}

/* Return a subtype of sizetype with range MIN to MAX and whose
   TYPE_INDEX_TYPE is INDEX.  GNAT_NODE is used for the position
   of the associated TYPE_DECL.  */

tree
create_index_type (tree min, tree max, tree index, Node_Id gnat_node)
{
  /* First build a type for the desired range.  */
  tree type = build_nonshared_range_type (sizetype, min, max);

  /* Then set the index type.  */
  SET_TYPE_INDEX_TYPE (type, index);
  create_type_decl (NULL_TREE, type, true, false, gnat_node);

  return type;
}

/* Return a subtype of TYPE with range MIN to MAX.  If TYPE is NULL,
   sizetype is used.  */

tree
create_range_type (tree type, tree min, tree max)
{
  tree range_type;

  if (!type)
    type = sizetype;

  /* First build a type with the base range.  */
  range_type = build_nonshared_range_type (type, TYPE_MIN_VALUE (type),
						 TYPE_MAX_VALUE (type));

  /* Then set the actual range.  */
  SET_TYPE_RM_MIN_VALUE (range_type, min);
  SET_TYPE_RM_MAX_VALUE (range_type, max);

  return range_type;
}

/* Return an extra subtype of TYPE with range MIN to MAX.  */

tree
create_extra_subtype (tree type, tree min, tree max)
{
  const bool uns = TYPE_UNSIGNED (type);
  const unsigned prec = TYPE_PRECISION (type);
  tree subtype = uns ? make_unsigned_type (prec) : make_signed_type (prec);

  TREE_TYPE (subtype) = type;
  TYPE_EXTRA_SUBTYPE_P (subtype) = 1;

  SET_TYPE_RM_MIN_VALUE (subtype, min);
  SET_TYPE_RM_MAX_VALUE (subtype, max);

  return subtype;
}

/* Return a TYPE_DECL node suitable for the TYPE_STUB_DECL field of TYPE.
   NAME gives the name of the type to be used in the declaration.  */

tree
create_type_stub_decl (tree name, tree type)
{
  tree type_decl = build_decl (input_location, TYPE_DECL, name, type);
  DECL_ARTIFICIAL (type_decl) = 1;
  DECL_NAMELESS (type_decl) = gnat_encodings != DWARF_GNAT_ENCODINGS_ALL;
  TYPE_ARTIFICIAL (type) = 1;
  return type_decl;
}

/* Return a TYPE_DECL node for TYPE.  NAME gives the name of the type to be
   used in the declaration.  ARTIFICIAL_P is true if the declaration was
   generated by the compiler.  DEBUG_INFO_P is true if we need to write
   debug information about this type.  GNAT_NODE is used for the position
   of the decl.  Normally, an artificial type might be marked as
   nameless.  However, if CAN_BE_NAMELESS is false, this marking is
   disabled and the name will always be attached for the type.  */

tree
create_type_decl (tree name, tree type, bool artificial_p, bool debug_info_p,
		  Node_Id gnat_node, bool can_be_nameless)
{
  enum tree_code code = TREE_CODE (type);
  bool is_named
    = TYPE_NAME (type) && TREE_CODE (TYPE_NAME (type)) == TYPE_DECL;
  tree type_decl;

  /* Only the builtin TYPE_STUB_DECL should be used for dummy types.  */
  gcc_assert (!TYPE_IS_DUMMY_P (type));

  /* If the type hasn't been named yet, we're naming it; preserve an existing
     TYPE_STUB_DECL that has been attached to it for some purpose.  */
  if (!is_named && TYPE_STUB_DECL (type))
    {
      type_decl = TYPE_STUB_DECL (type);
      DECL_NAME (type_decl) = name;
    }
  else
    type_decl = build_decl (input_location, TYPE_DECL, name, type);

  DECL_ARTIFICIAL (type_decl) = artificial_p;
  TYPE_ARTIFICIAL (type) = artificial_p;
  DECL_NAMELESS (type_decl) = (artificial_p
			       && can_be_nameless
			       && gnat_encodings != DWARF_GNAT_ENCODINGS_ALL);

  /* Add this decl to the current binding level.  */
  gnat_pushdecl (type_decl, gnat_node);

  /* If we're naming the type, equate the TYPE_STUB_DECL to the name.  This
     causes the name to be also viewed as a "tag" by the debug back-end, with
     the advantage that no DW_TAG_typedef is emitted for artificial "tagged"
     types in DWARF.

     Note that if "type" is used as a DECL_ORIGINAL_TYPE, it may be referenced
     from multiple contexts, and "type_decl" references a copy of it: in such a
     case, do not mess TYPE_STUB_DECL: we do not want to re-use the TYPE_DECL
     with the mechanism above.  */
  if (!is_named && type != DECL_ORIGINAL_TYPE (type_decl))
    TYPE_STUB_DECL (type) = type_decl;

  /* Do not generate debug info for UNCONSTRAINED_ARRAY_TYPE that the
     back-end doesn't support, and for others if we don't need to.  */
  if (code == UNCONSTRAINED_ARRAY_TYPE || !debug_info_p)
    DECL_IGNORED_P (type_decl) = 1;

  return type_decl;
}

/* Return a VAR_DECL or CONST_DECL node.

   NAME gives the name of the variable.  ASM_NAME is its assembler name
   (if provided).  TYPE is its data type (a GCC ..._TYPE node).  INIT is
   the GCC tree for an optional initial expression; NULL_TREE if none.

   CONST_FLAG is true if this variable is constant, in which case we might
   return a CONST_DECL node unless CONST_DECL_ALLOWED_P is false.

   PUBLIC_FLAG is true if this is for a reference to a public entity or for a
   definition to be made visible outside of the current compilation unit, for
   instance variable definitions in a package specification.

   EXTERN_FLAG is true when processing an external variable declaration (as
   opposed to a definition: no storage is to be allocated for the variable).

   STATIC_FLAG is only relevant when not at top level and indicates whether
   to always allocate storage to the variable.

   VOLATILE_FLAG is true if this variable is declared as volatile.

   ARTIFICIAL_P is true if the variable was generated by the compiler.

   DEBUG_INFO_P is true if we need to write debug information for it.

   ATTR_LIST is the list of attributes to be attached to the variable.

   GNAT_NODE is used for the position of the decl.  */

tree
create_var_decl (tree name, tree asm_name, tree type, tree init,
		 bool const_flag, bool public_flag, bool extern_flag,
		 bool static_flag, bool volatile_flag, bool artificial_p,
		 bool debug_info_p, struct attrib *attr_list,
		 Node_Id gnat_node, bool const_decl_allowed_p)
{
  /* Whether the object has static storage duration, either explicitly or by
     virtue of being declared at the global level.  */
  const bool static_storage = static_flag || global_bindings_p ();

  /* Whether the initializer is constant: for an external object or an object
     with static storage duration, we check that the initializer is a valid
     constant expression for initializing a static variable; otherwise, we
     only check that it is constant.  */
  const bool init_const
    = (init
       && gnat_types_compatible_p (type, TREE_TYPE (init))
       && (extern_flag || static_storage
	   ? initializer_constant_valid_p (init, TREE_TYPE (init))
	     != NULL_TREE
	   : TREE_CONSTANT (init)));

  /* Whether we will make TREE_CONSTANT the DECL we produce here, in which
     case the initializer may be used in lieu of the DECL node (as done in
     Identifier_to_gnu).  This is useful to prevent the need of elaboration
     code when an identifier for which such a DECL is made is in turn used
     as an initializer.  We used to rely on CONST_DECL vs VAR_DECL for this,
     but extra constraints apply to this choice (see below) and they are not
     relevant to the distinction we wish to make.  */
  const bool constant_p = const_flag && init_const;

  /* The actual DECL node.  CONST_DECL was initially intended for enumerals
     and may be used for scalars in general but not for aggregates.  */
  tree var_decl
    = build_decl (input_location,
		  (constant_p
		   && const_decl_allowed_p
		   && !AGGREGATE_TYPE_P (type) ? CONST_DECL : VAR_DECL),
		  name, type);

  /* Detect constants created by the front-end to hold 'reference to function
     calls for stabilization purposes.  This is needed for renaming.  */
  if (const_flag && init && POINTER_TYPE_P (type))
    {
      tree inner = init;
      if (TREE_CODE (inner) == COMPOUND_EXPR)
	inner = TREE_OPERAND (inner, 1);
      inner = remove_conversions (inner, true);
      if (TREE_CODE (inner) == ADDR_EXPR
	  && ((TREE_CODE (TREE_OPERAND (inner, 0)) == CALL_EXPR
	       && !call_is_atomic_load (TREE_OPERAND (inner, 0)))
	      || (VAR_P (TREE_OPERAND (inner, 0))
		  && DECL_RETURN_VALUE_P (TREE_OPERAND (inner, 0)))))
	DECL_RETURN_VALUE_P (var_decl) = 1;
    }

  /* If this is external, throw away any initializations (they will be done
     elsewhere) unless this is a constant for which we would like to remain
     able to get the initializer.  If we are defining a global here, leave a
     constant initialization and save any variable elaborations for the
     elaboration routine.  If we are just annotating types, throw away the
     initialization if it isn't a constant.  */
  if ((extern_flag && !constant_p)
      || (type_annotate_only && init && !TREE_CONSTANT (init)))
    init = NULL_TREE;

  /* At the global level, a non-constant initializer generates elaboration
     statements.  Check that such statements are allowed, that is to say,
     not violating a No_Elaboration_Code restriction.  */
  if (init && !init_const && global_bindings_p ())
    Check_Elaboration_Code_Allowed (gnat_node);

  /* Attach the initializer, if any.  */
  DECL_INITIAL (var_decl) = init;

  /* Directly set some flags.  */
  DECL_ARTIFICIAL (var_decl) = artificial_p;
  DECL_EXTERNAL (var_decl) = extern_flag;

  TREE_CONSTANT (var_decl) = constant_p;
  TREE_READONLY (var_decl) = const_flag;

  /* The object is public if it is external or if it is declared public
     and has static storage duration.  */
  TREE_PUBLIC (var_decl) = extern_flag || (public_flag && static_storage);

  /* We need to allocate static storage for an object with static storage
     duration if it isn't external.  */
  TREE_STATIC (var_decl) = !extern_flag && static_storage;

  TREE_SIDE_EFFECTS (var_decl)
    = TREE_THIS_VOLATILE (var_decl)
    = TYPE_VOLATILE (type) | volatile_flag;

  if (TREE_SIDE_EFFECTS (var_decl))
    TREE_ADDRESSABLE (var_decl) = 1;

  /* Ada doesn't feature Fortran-like COMMON variables so we shouldn't
     try to fiddle with DECL_COMMON.  However, on platforms that don't
     support global BSS sections, uninitialized global variables would
     go in DATA instead, thus increasing the size of the executable.  */
  if (!flag_no_common
      && VAR_P (var_decl)
      && TREE_PUBLIC (var_decl)
      && !have_global_bss_p ())
    DECL_COMMON (var_decl) = 1;

  /* Do not emit debug info if not requested, or for an external constant whose
     initializer is not absolute because this would require a global relocation
     in a read-only section which runs afoul of the PE-COFF run-time relocation
     mechanism.  */
  if (!debug_info_p
      || (extern_flag
	  && constant_p
	  && init
	  && initializer_constant_valid_p (init, TREE_TYPE (init))
	     != null_pointer_node))
    DECL_IGNORED_P (var_decl) = 1;

  /* ??? Some attributes cannot be applied to CONST_DECLs.  */
  if (VAR_P (var_decl))
    process_attributes (&var_decl, &attr_list, true, gnat_node);

  /* Add this decl to the current binding level.  */
  gnat_pushdecl (var_decl, gnat_node);

  if (VAR_P (var_decl) && asm_name)
    {
      /* Let the target mangle the name if this isn't a verbatim asm.  */
      if (*IDENTIFIER_POINTER (asm_name) != '*')
	asm_name = targetm.mangle_decl_assembler_name (var_decl, asm_name);

      SET_DECL_ASSEMBLER_NAME (var_decl, asm_name);
    }

  return var_decl;
}

/* Return true if TYPE, an aggregate type, contains (or is) an array.  */

static bool
aggregate_type_contains_array_p (tree type)
{
  switch (TREE_CODE (type))
    {
    case RECORD_TYPE:
    case UNION_TYPE:
    case QUAL_UNION_TYPE:
      {
	tree field;
	for (field = TYPE_FIELDS (type); field; field = DECL_CHAIN (field))
	  if (AGGREGATE_TYPE_P (TREE_TYPE (field))
	      && aggregate_type_contains_array_p (TREE_TYPE (field)))
	    return true;
	return false;
      }

    case ARRAY_TYPE:
      return true;

    default:
      gcc_unreachable ();
    }
}

/* Return true if TYPE is a type with variable size or a padding type with a
   field of variable size or a record that has a field with such a type.  */

bool
type_has_variable_size (tree type)
{
  tree field;

  if (!TREE_CONSTANT (TYPE_SIZE (type)))
    return true;

  if (TYPE_IS_PADDING_P (type)
      && !TREE_CONSTANT (DECL_SIZE (TYPE_FIELDS (type))))
    return true;

  if (!RECORD_OR_UNION_TYPE_P (type))
    return false;

  for (field = TYPE_FIELDS (type); field; field = DECL_CHAIN (field))
    if (type_has_variable_size (TREE_TYPE (field)))
      return true;

  return false;
}

/* Return a FIELD_DECL node.  NAME is the field's name, TYPE is its type and
   RECORD_TYPE is the type of the enclosing record.  If SIZE is nonzero, it
   is the specified size of the field.  If POS is nonzero, it is the bit
   position.  PACKED is 1 if the enclosing record is packed, -1 if it has
   Component_Alignment of Storage_Unit.  If ADDRESSABLE is nonzero, it
   means we are allowed to take the address of the field; if it is negative,
   we should not make a bitfield, which is used by make_aligning_type.  */

tree
create_field_decl (tree name, tree type, tree record_type, tree size, tree pos,
		   int packed, int addressable)
{
  tree field_decl = build_decl (input_location, FIELD_DECL, name, type);

  DECL_CONTEXT (field_decl) = record_type;
  TREE_READONLY (field_decl) = TYPE_READONLY (type);

  /* If a size is specified, use it.  Otherwise, if the record type is packed
     compute a size to use, which may differ from the object's natural size.
     We always set a size in this case to trigger the checks for bitfield
     creation below, which is typically required when no position has been
     specified.  */
  if (size)
    size = convert (bitsizetype, size);
  else if (packed == 1)
    {
      size = rm_size (type);
      if (TYPE_MODE (type) == BLKmode)
	size = round_up (size, BITS_PER_UNIT);
    }

  /* If we may, according to ADDRESSABLE, then make a bitfield when the size
     is specified for two reasons: first, when it differs from the natural
     size; second, when the alignment is insufficient.

     We never make a bitfield if the type of the field has a nonconstant size,
     because no such entity requiring bitfield operations should reach here.

     We do *preventively* make a bitfield when there might be the need for it
     but we don't have all the necessary information to decide, as is the case
     of a field in a packed record.

     We also don't look at STRICT_ALIGNMENT here, and rely on later processing
     in layout_decl or finish_record_type to clear the bit_field indication if
     it is in fact not needed.  */
  if (addressable >= 0
      && size
      && TREE_CODE (size) == INTEGER_CST
      && TREE_CODE (TYPE_SIZE (type)) == INTEGER_CST
      && (packed
	  || !tree_int_cst_equal (size, TYPE_SIZE (type))
	  || (pos && !value_factor_p (pos, TYPE_ALIGN (type)))
	  || (TYPE_ALIGN (record_type)
	      && TYPE_ALIGN (record_type) < TYPE_ALIGN (type))))
    {
      DECL_BIT_FIELD (field_decl) = 1;
      DECL_SIZE (field_decl) = size;
      if (!packed && !pos)
	{
	  if (TYPE_ALIGN (record_type)
	      && TYPE_ALIGN (record_type) < TYPE_ALIGN (type))
	    SET_DECL_ALIGN (field_decl, TYPE_ALIGN (record_type));
	  else
	    SET_DECL_ALIGN (field_decl, TYPE_ALIGN (type));
	}
    }

  DECL_PACKED (field_decl) = pos ? DECL_BIT_FIELD (field_decl) : packed;

  /* If FIELD_TYPE has BLKmode, we must ensure this is aligned to at least
     a byte boundary since GCC cannot handle less aligned BLKmode bitfields.
     Likewise if it has a variable size and no specified position because
     variable-sized objects need to be aligned to at least a byte boundary.
     Likewise for an aggregate without specified position that contains an
     array because, in this case, slices of variable length of this array
     must be handled by GCC and have variable size.  */
  if (packed && (TYPE_MODE (type) == BLKmode
		 || (!pos && type_has_variable_size (type))
		 || (!pos
		     && AGGREGATE_TYPE_P (type)
		     && aggregate_type_contains_array_p (type))))
    SET_DECL_ALIGN (field_decl, BITS_PER_UNIT);

  /* Bump the alignment if need be, either for bitfield/packing purposes or
     to satisfy the type requirements if no such considerations apply.  When
     we get the alignment from the type, indicate if this is from an explicit
     user request, which prevents stor-layout from lowering it later on.  */
  else
    {
      const unsigned int field_align
	= DECL_BIT_FIELD (field_decl)
	  ? 1
	  : packed
	    ? BITS_PER_UNIT
	    : 0;

      if (field_align > DECL_ALIGN (field_decl))
	SET_DECL_ALIGN (field_decl, field_align);
      else if (!field_align && TYPE_ALIGN (type) > DECL_ALIGN (field_decl))
	{
	  SET_DECL_ALIGN (field_decl, TYPE_ALIGN (type));
	  DECL_USER_ALIGN (field_decl) = TYPE_USER_ALIGN (type);
	}
    }

  if (pos)
    {
      /* We need to pass in the alignment the DECL is known to have.
	 This is the lowest-order bit set in POS, but no more than
	 the alignment of the record, if one is specified.  Note
	 that an alignment of 0 is taken as infinite.  */
      unsigned int known_align;

      if (tree_fits_uhwi_p (pos))
	known_align = tree_to_uhwi (pos) & -tree_to_uhwi (pos);
      else
	known_align = BITS_PER_UNIT;

      if (TYPE_ALIGN (record_type)
	  && (known_align == 0 || known_align > TYPE_ALIGN (record_type)))
	known_align = TYPE_ALIGN (record_type);

      layout_decl (field_decl, known_align);
      SET_DECL_OFFSET_ALIGN (field_decl,
			     tree_fits_uhwi_p (pos)
			     ? BIGGEST_ALIGNMENT : BITS_PER_UNIT);
      pos_from_bit (&DECL_FIELD_OFFSET (field_decl),
		    &DECL_FIELD_BIT_OFFSET (field_decl),
		    DECL_OFFSET_ALIGN (field_decl), pos);
    }

  /* In addition to what our caller says, claim the field is addressable if we
     know that its type is not suitable.

     The field may also be "technically" nonaddressable, meaning that even if
     we attempt to take the field's address we will actually get the address
     of a copy.  This is the case for true bitfields, but the DECL_BIT_FIELD
     value we have at this point is not accurate enough, so we don't account
     for this here and let finish_record_type decide.  */
  if (!addressable && !type_for_nonaliased_component_p (type))
    addressable = 1;

  /* Note that there is a trade-off in making a field nonaddressable because
     this will cause type-based alias analysis to use the same alias set for
     accesses to the field as for accesses to the whole record: while doing
     so will make it more likely to disambiguate accesses to other objects
     and accesses to the field, it will make it less likely to disambiguate
     accesses to the other fields of the record and accesses to the field.
     If the record is fully static, then the trade-off is irrelevant since
     the fields of the record can always be disambiguated by their offsets
     but, if the record is dynamic, then it can become problematic.  */
  DECL_NONADDRESSABLE_P (field_decl) = !addressable;

  return field_decl;
}

/* Return a PARM_DECL node with NAME and TYPE.  */

tree
create_param_decl (tree name, tree type)
{
  tree param_decl = build_decl (input_location, PARM_DECL, name, type);
  DECL_ARG_TYPE (param_decl) = type;
  return param_decl;
}

/* Process the attributes in ATTR_LIST for NODE, which is either a DECL or
   a TYPE.  If IN_PLACE is true, the tree pointed to by NODE should not be
   changed.  GNAT_NODE is used for the position of error messages.  */

void
process_attributes (tree *node, struct attrib **attr_list, bool in_place,
		    Node_Id gnat_node)
{
  struct attrib *attr;

  for (attr = *attr_list; attr; attr = attr->next)
    switch (attr->type)
      {
      case ATTR_MACHINE_ATTRIBUTE:
	Sloc_to_locus (Sloc (gnat_node), &input_location);
	decl_attributes (node, tree_cons (attr->name, attr->args, NULL_TREE),
			 in_place ? ATTR_FLAG_TYPE_IN_PLACE : 0);
	break;

      case ATTR_LINK_ALIAS:
        if (!DECL_EXTERNAL (*node))
	  {
	    TREE_STATIC (*node) = 1;
	    assemble_alias (*node, attr->name);
	  }
	break;

      case ATTR_WEAK_EXTERNAL:
	if (SUPPORTS_WEAK)
	  declare_weak (*node);
	else
	  post_error ("?weak declarations not supported on this target",
		      attr->error_point);
	break;

      case ATTR_LINK_SECTION:
	if (targetm_common.have_named_sections)
	  {
	    set_decl_section_name (*node, IDENTIFIER_POINTER (attr->name));
	    DECL_COMMON (*node) = 0;
	  }
	else
	  post_error ("?section attributes are not supported for this target",
		      attr->error_point);
	break;

      case ATTR_LINK_CONSTRUCTOR:
	DECL_STATIC_CONSTRUCTOR (*node) = 1;
	TREE_USED (*node) = 1;
	break;

      case ATTR_LINK_DESTRUCTOR:
	DECL_STATIC_DESTRUCTOR (*node) = 1;
	TREE_USED (*node) = 1;
	break;

      case ATTR_THREAD_LOCAL_STORAGE:
	set_decl_tls_model (*node, decl_default_tls_model (*node));
	DECL_COMMON (*node) = 0;
	break;
      }

  *attr_list = NULL;
}

/* Return true if VALUE is a known to be a multiple of FACTOR, which must be
   a power of 2. */

bool
value_factor_p (tree value, unsigned HOST_WIDE_INT factor)
{
  gcc_checking_assert (pow2p_hwi (factor));

  if (tree_fits_uhwi_p (value))
    return (tree_to_uhwi (value) & (factor - 1)) == 0;

  if (TREE_CODE (value) == MULT_EXPR)
    return (value_factor_p (TREE_OPERAND (value, 0), factor)
            || value_factor_p (TREE_OPERAND (value, 1), factor));

  return false;
}

/* Defer the initialization of DECL's DECL_CONTEXT attribute, scheduling to
   feed it with the elaboration of GNAT_SCOPE.  */

static struct deferred_decl_context_node *
add_deferred_decl_context (tree decl, Entity_Id gnat_scope, int force_global)
{
  struct deferred_decl_context_node *new_node;

  new_node
    = (struct deferred_decl_context_node * ) xmalloc (sizeof (*new_node));
  new_node->decl = decl;
  new_node->gnat_scope = gnat_scope;
  new_node->force_global = force_global;
  new_node->types.create (1);
  new_node->next = deferred_decl_context_queue;
  deferred_decl_context_queue = new_node;
  return new_node;
}

/* Defer the initialization of TYPE's TYPE_CONTEXT attribute, scheduling to
   feed it with the DECL_CONTEXT computed as part of N as soon as it is
   computed.  */

static void
add_deferred_type_context (struct deferred_decl_context_node *n, tree type)
{
  n->types.safe_push (type);
}

/* Get the GENERIC node corresponding to GNAT_SCOPE, if available.  Return
   NULL_TREE if it is not available.  */

static tree
compute_deferred_decl_context (Entity_Id gnat_scope)
{
  tree context;

  if (present_gnu_tree (gnat_scope))
    context = get_gnu_tree (gnat_scope);
  else
    return NULL_TREE;

  if (TREE_CODE (context) == TYPE_DECL)
    {
      tree context_type = TREE_TYPE (context);

      /* Skip dummy types: only the final ones can appear in the context
	 chain.  */
      if (TYPE_DUMMY_P (context_type))
	return NULL_TREE;

      /* ..._TYPE nodes are more useful than TYPE_DECL nodes in the context
	 chain.  */
      else
	context = context_type;
    }

  return context;
}

/* Try to process all deferred nodes in the queue.  Keep in the queue the ones
   that cannot be processed yet, remove the other ones.  If FORCE is true,
   force the processing for all nodes, use the global context when nodes don't
   have a GNU translation.  */

void
process_deferred_decl_context (bool force)
{
  struct deferred_decl_context_node **it = &deferred_decl_context_queue;
  struct deferred_decl_context_node *node;

  while (*it)
    {
      bool processed = false;
      tree context = NULL_TREE;
      Entity_Id gnat_scope;

      node = *it;

      /* If FORCE, get the innermost elaborated scope.  Otherwise, just try to
	 get the first scope.  */
      gnat_scope = node->gnat_scope;
      while (Present (gnat_scope))
	{
	  context = compute_deferred_decl_context (gnat_scope);
	  if (!force || context)
	    break;
	  gnat_scope = get_debug_scope (gnat_scope, NULL);
	}

      /* Imported declarations must not be in a local context (i.e. not inside
	 a function).  */
      if (context && node->force_global > 0)
	{
	  tree ctx = context;

	  while (ctx)
	    {
	      gcc_assert (TREE_CODE (ctx) != FUNCTION_DECL);
	      ctx = DECL_P (ctx) ? DECL_CONTEXT (ctx) : TYPE_CONTEXT (ctx);
	    }
	}

      /* If FORCE, we want to get rid of all nodes in the queue: in case there
	 was no elaborated scope, use the global context.  */
      if (force && !context)
	context = get_global_context ();

      if (context)
	{
	  tree t;
	  int i;

	  DECL_CONTEXT (node->decl) = context;

	  /* Propagate it to the TYPE_CONTEXT attributes of the requested
	     ..._TYPE nodes.  */
	  FOR_EACH_VEC_ELT (node->types, i, t)
	    {
	      gnat_set_type_context (t, context);
	    }
	  processed = true;
	}

      /* If this node has been successfuly processed, remove it from the
	 queue.  Then move to the next node.  */
      if (processed)
	{
	  *it = node->next;
	  node->types.release ();
	  free (node);
	}
      else
	it = &node->next;
    }
}

/* Return VALUE scaled by the biggest power-of-2 factor of EXPR.  */

static unsigned int
scale_by_factor_of (tree expr, unsigned int value)
{
  unsigned HOST_WIDE_INT addend = 0;
  unsigned HOST_WIDE_INT factor = 1;

  /* Peel conversions around EXPR and try to extract bodies from function
     calls: it is possible to get the scale factor from size functions.  */
  expr = remove_conversions (expr, true);
  if (TREE_CODE (expr) == CALL_EXPR)
    expr = maybe_inline_call_in_expr (expr);

  /* Sometimes we get PLUS_EXPR (BIT_AND_EXPR (..., X), Y), where Y is a
     multiple of the scale factor we are looking for.  */
  if (TREE_CODE (expr) == PLUS_EXPR
      && TREE_CODE (TREE_OPERAND (expr, 1)) == INTEGER_CST
      && tree_fits_uhwi_p (TREE_OPERAND (expr, 1)))
    {
      addend = TREE_INT_CST_LOW (TREE_OPERAND (expr, 1));
      expr = TREE_OPERAND (expr, 0);
    }

  /* An expression which is a bitwise AND with a mask has a power-of-2 factor
     corresponding to the number of trailing zeros of the mask.  */
  if (TREE_CODE (expr) == BIT_AND_EXPR
      && TREE_CODE (TREE_OPERAND (expr, 1)) == INTEGER_CST)
    {
      unsigned HOST_WIDE_INT mask = TREE_INT_CST_LOW (TREE_OPERAND (expr, 1));
      unsigned int i = 0;

      while ((mask & 1) == 0 && i < HOST_BITS_PER_WIDE_INT)
	{
	  mask >>= 1;
	  factor *= 2;
	  i++;
	}
    }

  /* If the addend is not a multiple of the factor we found, give up.  In
     theory we could find a smaller common factor but it's useless for our
     needs.  This situation arises when dealing with a field F1 with no
     alignment requirement but that is following a field F2 with such
     requirements.  As long as we have F2's offset, we don't need alignment
     information to compute F1's.  */
  if (addend % factor != 0)
    factor = 1;

  return factor * value;
}

/* Return a LABEL_DECL with NAME.  GNAT_NODE is used for the position of
   the decl.  */

tree
create_label_decl (tree name, Node_Id gnat_node)
{
  tree label_decl
    = build_decl (input_location, LABEL_DECL, name, void_type_node);

  SET_DECL_MODE (label_decl, VOIDmode);

  /* Add this decl to the current binding level.  */
  gnat_pushdecl (label_decl, gnat_node);

  return label_decl;
}

/* Return a FUNCTION_DECL node.  NAME is the name of the subprogram, ASM_NAME
   its assembler name, TYPE its type (a FUNCTION_TYPE or METHOD_TYPE node),
   PARAM_DECL_LIST the list of its parameters (a list of PARM_DECL nodes
   chained through the DECL_CHAIN field).

   INLINE_STATUS describes the inline flags to be set on the FUNCTION_DECL.

   PUBLIC_FLAG is true if this is for a reference to a public entity or for a
   definition to be made visible outside of the current compilation unit.

   EXTERN_FLAG is true when processing an external subprogram declaration.

   ARTIFICIAL_P is true if the subprogram was generated by the compiler.

   DEBUG_INFO_P is true if we need to write debug information for it.

   DEFINITION is true if the subprogram is to be considered as a definition.

   ATTR_LIST is the list of attributes to be attached to the subprogram.

   GNAT_NODE is used for the position of the decl.  */

tree
create_subprog_decl (tree name, tree asm_name, tree type, tree param_decl_list,
		     enum inline_status_t inline_status, bool public_flag,
		     bool extern_flag, bool artificial_p, bool debug_info_p,
		     bool definition, struct attrib *attr_list,
		     Node_Id gnat_node)
{
  tree subprog_decl = build_decl (input_location, FUNCTION_DECL, name, type);
  DECL_ARGUMENTS (subprog_decl) = param_decl_list;

  DECL_ARTIFICIAL (subprog_decl) = artificial_p;
  DECL_EXTERNAL (subprog_decl) = extern_flag;
  DECL_FUNCTION_IS_DEF (subprog_decl) = definition;
  DECL_IGNORED_P (subprog_decl) = !debug_info_p;
  TREE_PUBLIC (subprog_decl) = public_flag;

  switch (inline_status)
    {
    case is_suppressed:
      DECL_UNINLINABLE (subprog_decl) = 1;
      break;

    case is_default:
      break;

    case is_required:
      if (Back_End_Inlining)
	{
	  decl_attributes (&subprog_decl,
			   tree_cons (get_identifier ("always_inline"),
				      NULL_TREE, NULL_TREE),
			   ATTR_FLAG_TYPE_IN_PLACE);

	  /* Inline_Always guarantees that every direct call is inlined and
	     that there is no indirect reference to the subprogram, so the
	     instance in the original package (as well as its clones in the
	     client packages created for inter-unit inlining) can be made
	     private, which causes the out-of-line body to be eliminated.  */
	  TREE_PUBLIC (subprog_decl) = 0;
	}

      /* ... fall through ... */

    case is_prescribed:
      DECL_DISREGARD_INLINE_LIMITS (subprog_decl) = 1;

      /* ... fall through ... */

    case is_requested:
      DECL_DECLARED_INLINE_P (subprog_decl) = 1;
      if (!Debug_Generated_Code)
	DECL_NO_INLINE_WARNING_P (subprog_decl) = artificial_p;
      break;

    default:
      gcc_unreachable ();
    }

  process_attributes (&subprog_decl, &attr_list, true, gnat_node);

  /* Once everything is processed, finish the subprogram declaration.  */
  finish_subprog_decl (subprog_decl, asm_name, type);

  /* Add this decl to the current binding level.  */
  gnat_pushdecl (subprog_decl, gnat_node);

  /* Output the assembler code and/or RTL for the declaration.  */
  rest_of_decl_compilation (subprog_decl, global_bindings_p (), 0);

  return subprog_decl;
}

/* Given a subprogram declaration DECL, its assembler name and its type,
   finish constructing the subprogram declaration from ASM_NAME and TYPE.  */

void
finish_subprog_decl (tree decl, tree asm_name, tree type)
{
  /* DECL_ARGUMENTS is set by the caller, but not its context.  */
  for (tree param_decl = DECL_ARGUMENTS (decl);
       param_decl;
       param_decl = DECL_CHAIN (param_decl))
    DECL_CONTEXT (param_decl) = decl;

  tree result_decl
    = build_decl (DECL_SOURCE_LOCATION (decl), RESULT_DECL, NULL_TREE,
		  TREE_TYPE (type));

  DECL_ARTIFICIAL (result_decl) = 1;
  DECL_IGNORED_P (result_decl) = 1;
  DECL_CONTEXT (result_decl) = decl;
  DECL_BY_REFERENCE (result_decl) = TREE_ADDRESSABLE (type);
  DECL_RESULT (decl) = result_decl;

  /* Propagate the "pure" property.  */
  DECL_PURE_P (decl) = TYPE_RESTRICT (type);

  /* Propagate the "noreturn" property.  */
  TREE_THIS_VOLATILE (decl) = TYPE_VOLATILE (type);

  if (asm_name)
    {
      /* Let the target mangle the name if this isn't a verbatim asm.  */
      if (*IDENTIFIER_POINTER (asm_name) != '*')
	asm_name = targetm.mangle_decl_assembler_name (decl, asm_name);

      SET_DECL_ASSEMBLER_NAME (decl, asm_name);

      /* The expand_main_function circuitry expects "main_identifier_node" to
	 designate the DECL_NAME of the 'main' entry point, in turn expected
	 to be declared as the "main" function literally by default.  Ada
	 program entry points are typically declared with a different name
	 within the binder generated file, exported as 'main' to satisfy the
	 system expectations.  Force main_identifier_node in this case.  */
      if (asm_name == main_identifier_node)
	DECL_NAME (decl) = main_identifier_node;
    }
}

/* Set up the framework for generating code for SUBPROG_DECL, a subprogram
   body.  This routine needs to be invoked before processing the declarations
   appearing in the subprogram.  */

void
begin_subprog_body (tree subprog_decl)
{
  announce_function (subprog_decl);

  /* This function is being defined.  */
  TREE_STATIC (subprog_decl) = 1;

  /* The failure of this assertion will likely come from a wrong context for
     the subprogram body, e.g. another procedure for a procedure declared at
     library level.  */
  gcc_assert (current_function_decl == decl_function_context (subprog_decl));

  current_function_decl = subprog_decl;

  /* Enter a new binding level and show that all the parameters belong to
     this function.  */
  gnat_pushlevel ();
}

/* Finish translating the current subprogram and set its BODY.  */

void
end_subprog_body (tree body)
{
  tree fndecl = current_function_decl;

  /* Attach the BLOCK for this level to the function and pop the level.  */
  BLOCK_SUPERCONTEXT (current_binding_level->block) = fndecl;
  DECL_INITIAL (fndecl) = current_binding_level->block;
  gnat_poplevel ();

  /* The body should be a BIND_EXPR whose BLOCK is the top-level one.  */
  if (TREE_CODE (body) == BIND_EXPR)
    {
      BLOCK_SUPERCONTEXT (BIND_EXPR_BLOCK (body)) = fndecl;
      DECL_INITIAL (fndecl) = BIND_EXPR_BLOCK (body);
    }

  DECL_SAVED_TREE (fndecl) = body;

  current_function_decl = decl_function_context (fndecl);
}

/* Wrap up compilation of SUBPROG_DECL, a subprogram body.  */

void
rest_of_subprog_body_compilation (tree subprog_decl)
{
  /* We cannot track the location of errors past this point.  */
  Current_Error_Node = Empty;

  /* If we're only annotating types, don't actually compile this function.  */
  if (type_annotate_only)
    return;

  /* Dump functions before gimplification.  */
  dump_function (TDI_original, subprog_decl);

  if (!decl_function_context (subprog_decl))
    cgraph_node::finalize_function (subprog_decl, false);
  else
    /* Register this function with cgraph just far enough to get it
       added to our parent's nested function list.  */
    (void) cgraph_node::get_create (subprog_decl);
}

tree
gnat_builtin_function (tree decl)
{
  gnat_pushdecl (decl, Empty);
  return decl;
}

/* Return an integer type with the number of bits of precision given by
   PRECISION.  UNSIGNEDP is nonzero if the type is unsigned; otherwise
   it is a signed type.  */

tree
gnat_type_for_size (unsigned precision, int unsignedp)
{
  tree t;
  char type_name[20];

  if (precision <= 2 * MAX_BITS_PER_WORD
      && signed_and_unsigned_types[precision][unsignedp])
    return signed_and_unsigned_types[precision][unsignedp];

 if (unsignedp)
    t = make_unsigned_type (precision);
  else
    t = make_signed_type (precision);
  TYPE_ARTIFICIAL (t) = 1;

  if (precision <= 2 * MAX_BITS_PER_WORD)
    signed_and_unsigned_types[precision][unsignedp] = t;

  if (!TYPE_NAME (t))
    {
      sprintf (type_name, "%sSIGNED_%u", unsignedp ? "UN" : "", precision);
      TYPE_NAME (t) = get_identifier (type_name);
    }

  return t;
}

/* Likewise for floating-point types.  */

static tree
float_type_for_precision (int precision, machine_mode mode)
{
  tree t;
  char type_name[20];

  if (float_types[(int) mode])
    return float_types[(int) mode];

  float_types[(int) mode] = t = make_node (REAL_TYPE);
  TYPE_PRECISION (t) = precision;
  layout_type (t);

  gcc_assert (TYPE_MODE (t) == mode);
  if (!TYPE_NAME (t))
    {
      sprintf (type_name, "FLOAT_%d", precision);
      TYPE_NAME (t) = get_identifier (type_name);
    }

  return t;
}

/* Return a data type that has machine mode MODE.  UNSIGNEDP selects
   an unsigned type; otherwise a signed type is returned.  */

tree
gnat_type_for_mode (machine_mode mode, int unsignedp)
{
  if (mode == BLKmode)
    return NULL_TREE;

  if (mode == VOIDmode)
    return void_type_node;

  if (COMPLEX_MODE_P (mode))
    return NULL_TREE;

  scalar_float_mode float_mode;
  if (is_a <scalar_float_mode> (mode, &float_mode))
    return float_type_for_precision (GET_MODE_PRECISION (float_mode),
				     float_mode);

  scalar_int_mode int_mode;
  if (is_a <scalar_int_mode> (mode, &int_mode))
    return gnat_type_for_size (GET_MODE_BITSIZE (int_mode), unsignedp);

  if (VECTOR_MODE_P (mode))
    {
      machine_mode inner_mode = GET_MODE_INNER (mode);
      tree inner_type = gnat_type_for_mode (inner_mode, unsignedp);
      if (inner_type)
	return build_vector_type_for_mode (inner_type, mode);
    }

  return NULL_TREE;
}

/* Return the signed or unsigned version of TYPE_NODE, a scalar type, the
   signedness being specified by UNSIGNEDP.  */

tree
gnat_signed_or_unsigned_type_for (int unsignedp, tree type_node)
{
  if (type_node == char_type_node)
    return unsignedp ? unsigned_char_type_node : signed_char_type_node;

  tree type = gnat_type_for_size (TYPE_PRECISION (type_node), unsignedp);

  if (TREE_CODE (type_node) == INTEGER_TYPE && TYPE_MODULAR_P (type_node))
    {
      type = copy_type (type);
      TREE_TYPE (type) = type_node;
    }
  else if (TREE_TYPE (type_node)
	   && TREE_CODE (TREE_TYPE (type_node)) == INTEGER_TYPE
	   && TYPE_MODULAR_P (TREE_TYPE (type_node)))
    {
      type = copy_type (type);
      TREE_TYPE (type) = TREE_TYPE (type_node);
    }

  return type;
}

/* Return 1 if the types T1 and T2 are compatible, i.e. if they can be
   transparently converted to each other.  */

int
gnat_types_compatible_p (tree t1, tree t2)
{
  enum tree_code code;

  /* This is the default criterion.  */
  if (TYPE_MAIN_VARIANT (t1) == TYPE_MAIN_VARIANT (t2))
    return 1;

  /* We only check structural equivalence here.  */
  if ((code = TREE_CODE (t1)) != TREE_CODE (t2))
    return 0;

  /* Vector types are also compatible if they have the same number of subparts
     and the same form of (scalar) element type.  */
  if (code == VECTOR_TYPE
      && known_eq (TYPE_VECTOR_SUBPARTS (t1), TYPE_VECTOR_SUBPARTS (t2))
      && TREE_CODE (TREE_TYPE (t1)) == TREE_CODE (TREE_TYPE (t2))
      && TYPE_PRECISION (TREE_TYPE (t1)) == TYPE_PRECISION (TREE_TYPE (t2)))
    return 1;

  /* Array types are also compatible if they are constrained and have the same
     domain(s), the same component type and the same scalar storage order.  */
  if (code == ARRAY_TYPE
      && (TYPE_DOMAIN (t1) == TYPE_DOMAIN (t2)
	  || (TYPE_DOMAIN (t1)
	      && TYPE_DOMAIN (t2)
	      && tree_int_cst_equal (TYPE_MIN_VALUE (TYPE_DOMAIN (t1)),
				     TYPE_MIN_VALUE (TYPE_DOMAIN (t2)))
	      && tree_int_cst_equal (TYPE_MAX_VALUE (TYPE_DOMAIN (t1)),
				     TYPE_MAX_VALUE (TYPE_DOMAIN (t2)))))
      && (TREE_TYPE (t1) == TREE_TYPE (t2)
	  || (TREE_CODE (TREE_TYPE (t1)) == ARRAY_TYPE
	      && gnat_types_compatible_p (TREE_TYPE (t1), TREE_TYPE (t2))))
      && TYPE_REVERSE_STORAGE_ORDER (t1) == TYPE_REVERSE_STORAGE_ORDER (t2))
    return 1;

  return 0;
}

/* Return true if EXPR is a useless type conversion.  */

bool
gnat_useless_type_conversion (tree expr)
{
  if (CONVERT_EXPR_P (expr)
      || TREE_CODE (expr) == VIEW_CONVERT_EXPR
      || TREE_CODE (expr) == NON_LVALUE_EXPR)
    return gnat_types_compatible_p (TREE_TYPE (expr),
				    TREE_TYPE (TREE_OPERAND (expr, 0)));

  return false;
}

/* Return true if T, a {FUNCTION,METHOD}_TYPE, has the specified flags.  */

bool
fntype_same_flags_p (const_tree t, tree cico_list, bool return_by_direct_ref_p,
		     bool return_by_invisi_ref_p)
{
  return TYPE_CI_CO_LIST (t) == cico_list
	 && TYPE_RETURN_BY_DIRECT_REF_P (t) == return_by_direct_ref_p
	 && TREE_ADDRESSABLE (t) == return_by_invisi_ref_p;
}

/* Try to compute the maximum (if MAX_P) or minimum (if !MAX_P) value for the
   expression EXP, for very simple expressions.  Substitute variable references
   with their respective type's min/max values.  Return the computed value if
   any, or EXP if no value can be computed. */

tree
max_value (tree exp, bool max_p)
{
  enum tree_code code = TREE_CODE (exp);
  tree type = TREE_TYPE (exp);
  tree op0, op1, op2;

  switch (TREE_CODE_CLASS (code))
    {
    case tcc_declaration:
      if (VAR_P (exp))
        return fold_convert (type,
                             max_p
                             ? TYPE_MAX_VALUE (type) : TYPE_MIN_VALUE (type));
      break;

    case tcc_vl_exp:
      if (code == CALL_EXPR)
	{
          tree t;

          t = maybe_inline_call_in_expr (exp);
          if (t)
            return max_value (t, max_p);
        }
      break;

    case tcc_comparison:
      return build_int_cst (type, max_p ? 1 : 0);

    case tcc_unary:
      op0 = TREE_OPERAND (exp, 0);

      if (code == NON_LVALUE_EXPR)
        return max_value (op0, max_p);

      if (code == NEGATE_EXPR)
        return max_value (op0, !max_p);

      if (code == NOP_EXPR)
	return fold_convert (type, max_value (op0, max_p));

      break;

    case tcc_binary:
      op0 = TREE_OPERAND (exp, 0);
      op1 = TREE_OPERAND (exp, 1);

      switch (code) {
      case PLUS_EXPR:
      case MULT_EXPR:
        return fold_build2 (code, type, max_value(op0, max_p),
                            max_value (op1, max_p));
      case MINUS_EXPR:
      case TRUNC_DIV_EXPR:
        return fold_build2 (code, type, max_value(op0, max_p),
                            max_value (op1, !max_p));
      default:
        break;
      }
      break;

    case tcc_expression:
      if (code == COND_EXPR)
        {
          op0 = TREE_OPERAND (exp, 0);
          op1 = TREE_OPERAND (exp, 1);
          op2 = TREE_OPERAND (exp, 2);

          if (!op1 || !op2)
            break;

          op1 = max_value (op1, max_p);
          op2 = max_value (op2, max_p);

          if (op1 == TREE_OPERAND (exp, 1) && op2 == TREE_OPERAND (exp, 2))
            break;

          return fold_build2 (max_p ? MAX_EXPR : MIN_EXPR, type, op1, op2);
	}
      break;

    default:
      break;
    }
  return exp;
}


/* EXP is an expression for the size of an object.  If this size contains
   discriminant references, replace them with the maximum (if MAX_P) or
   minimum (if !MAX_P) possible value of the discriminant.

   Note that the expression may have already been gimplified,in which case
   COND_EXPRs have VOID_TYPE and no operands, and this must be handled.  */

tree
max_size (tree exp, bool max_p)
{
  enum tree_code code = TREE_CODE (exp);
  tree type = TREE_TYPE (exp);
  tree op0, op1, op2;

  switch (TREE_CODE_CLASS (code))
    {
    case tcc_declaration:
    case tcc_constant:
      return exp;

    case tcc_exceptional:
      gcc_assert (code == SSA_NAME);
      return exp;

    case tcc_vl_exp:
      if (code == CALL_EXPR)
	{
	  tree t, *argarray;
	  int n, i;

	  t = maybe_inline_call_in_expr (exp);
	  if (t)
	    return max_size (t, max_p);

	  n = call_expr_nargs (exp);
	  gcc_assert (n > 0);
	  argarray = XALLOCAVEC (tree, n);
	  /* This is used to remove possible placeholder in call args.  */
	  for (i = 0; i < n; i++)
	    argarray[i] = max_size (CALL_EXPR_ARG (exp, i), max_p);
	  return build_call_array (type, CALL_EXPR_FN (exp), n, argarray);
	}
      break;

    case tcc_reference:
      /* If this contains a PLACEHOLDER_EXPR, it is the thing we want to
	 modify.  Otherwise, we treat it like a variable.  */
      if (CONTAINS_PLACEHOLDER_P (exp))
	{
	  tree base_type = get_base_type (TREE_TYPE (TREE_OPERAND (exp, 1)));
	  tree val
	    = fold_convert (base_type,
			    max_p
			    ? TYPE_MAX_VALUE (type) : TYPE_MIN_VALUE (type));

	  /* Walk down the extra subtypes to get more restrictive bounds.  */
	  while (TYPE_IS_EXTRA_SUBTYPE_P (type))
	    {
	      type = TREE_TYPE (type);
	      if (max_p)
		val = fold_build2 (MIN_EXPR, base_type, val,
				   fold_convert (base_type,
						 TYPE_MAX_VALUE (type)));
	      else
		val = fold_build2 (MAX_EXPR, base_type, val,
				   fold_convert (base_type,
						 TYPE_MIN_VALUE (type)));
	    }

	  return fold_convert (type, max_size (val, max_p));
	}

      return exp;

    case tcc_comparison:
      return build_int_cst (type, max_p ? 1 : 0);

    case tcc_unary:
      op0 = TREE_OPERAND (exp, 0);

      if (code == NON_LVALUE_EXPR)
	return max_size (op0, max_p);

      if (VOID_TYPE_P (TREE_TYPE (op0)))
	return max_p ? TYPE_MAX_VALUE (type) : TYPE_MIN_VALUE (type);

      op0 = max_size (op0, code == NEGATE_EXPR ? !max_p : max_p);

      if (op0 == TREE_OPERAND (exp, 0))
	return exp;

      return fold_build1 (code, type, op0);

    case tcc_binary:
      op0 = TREE_OPERAND (exp, 0);
      op1 = TREE_OPERAND (exp, 1);

      /* If we have a multiply-add with a "negative" value in an unsigned
	 type, do a multiply-subtract with the negated value, in order to
	 avoid creating a spurious overflow below.  */
      if (code == PLUS_EXPR
	  && TREE_CODE (op0) == MULT_EXPR
	  && TYPE_UNSIGNED (type)
	  && TREE_CODE (TREE_OPERAND (op0, 1)) == INTEGER_CST
	  && !TREE_OVERFLOW (TREE_OPERAND (op0, 1))
	  && tree_int_cst_sign_bit (TREE_OPERAND (op0, 1)))
	{
	  tree tmp = op1;
	  op1 = build2 (MULT_EXPR, type, TREE_OPERAND (op0, 0),
			fold_build1 (NEGATE_EXPR, type,
				    TREE_OPERAND (op0, 1)));
	  op0 = tmp;
	  code = MINUS_EXPR;
	}

      op0 = max_size (op0, max_p);
      op1 = max_size (op1, code == MINUS_EXPR ? !max_p : max_p);

      if ((code == MINUS_EXPR || code == PLUS_EXPR))
	{
	  /* If the op0 has overflowed and the op1 is a variable,
	     propagate the overflow by returning the op0.  */
	  if (TREE_CODE (op0) == INTEGER_CST
	      && TREE_OVERFLOW (op0)
	      && TREE_CODE (op1) != INTEGER_CST)
	    return op0;

	  /* If we have a "negative" value in an unsigned type, do the
	     opposite operation on the negated value, in order to avoid
	     creating a spurious overflow below.  */
	  if (TYPE_UNSIGNED (type)
	      && TREE_CODE (op1) == INTEGER_CST
	      && !TREE_OVERFLOW (op1)
	      && tree_int_cst_sign_bit (op1))
	    {
	      op1 = fold_build1 (NEGATE_EXPR, type, op1);
	      code = (code == MINUS_EXPR ? PLUS_EXPR : MINUS_EXPR);
	    }
	}

      if (op0 == TREE_OPERAND (exp, 0) && op1 == TREE_OPERAND (exp, 1))
	return exp;

      /* We need to detect overflows so we call size_binop here.  */
      return size_binop (code, op0, op1);

    case tcc_expression:
      switch (TREE_CODE_LENGTH (code))
	{
	case 1:
	  if (code == SAVE_EXPR)
	    return exp;

	  op0 = max_size (TREE_OPERAND (exp, 0),
			  code == TRUTH_NOT_EXPR ? !max_p : max_p);

	  if (op0 == TREE_OPERAND (exp, 0))
	    return exp;

	  return fold_build1 (code, type, op0);

	case 2:
	  if (code == COMPOUND_EXPR)
	    return max_size (TREE_OPERAND (exp, 1), max_p);

	  op0 = max_size (TREE_OPERAND (exp, 0), max_p);
	  op1 = max_size (TREE_OPERAND (exp, 1), max_p);

	  if (op0 == TREE_OPERAND (exp, 0) && op1 == TREE_OPERAND (exp, 1))
	    return exp;

	  return fold_build2 (code, type, op0, op1);

	case 3:
	  if (code == COND_EXPR)
	    {
	      op0 = TREE_OPERAND (exp, 0);
	      op1 = TREE_OPERAND (exp, 1);
	      op2 = TREE_OPERAND (exp, 2);

	      if (!op1 || !op2)
		return exp;

	      op1 = max_size (op1, max_p);
	      op2 = max_size (op2, max_p);

	      /* If we have the MAX of a "negative" value in an unsigned type
		 and zero for a length expression, just return zero.  */
	      if (max_p
		  && TREE_CODE (op0) == LE_EXPR
		  && TYPE_UNSIGNED (type)
		  && TREE_CODE (op1) == INTEGER_CST
		  && !TREE_OVERFLOW (op1)
		  && tree_int_cst_sign_bit (op1)
		  && integer_zerop (op2))
		return op2;

	      return fold_build2 (max_p ? MAX_EXPR : MIN_EXPR, type, op1, op2);
	    }
	  break;

	default:
	  break;
	}

      /* Other tree classes cannot happen.  */
    default:
      break;
    }

  gcc_unreachable ();
}

/* Build a template of type TEMPLATE_TYPE from the array bounds of ARRAY_TYPE.
   EXPR is an expression that we can use to locate any PLACEHOLDER_EXPRs.
   Return a constructor for the template.  */

tree
build_template (tree template_type, tree array_type, tree expr)
{
  vec<constructor_elt, va_gc> *template_elts = NULL;
  tree bound_list = NULL_TREE;
  tree field;

  while (TREE_CODE (array_type) == RECORD_TYPE
	 && (TYPE_PADDING_P (array_type)
	     || TYPE_JUSTIFIED_MODULAR_P (array_type)))
    array_type = TREE_TYPE (TYPE_FIELDS (array_type));

  if (TREE_CODE (array_type) == ARRAY_TYPE
      || (TREE_CODE (array_type) == INTEGER_TYPE
	  && TYPE_HAS_ACTUAL_BOUNDS_P (array_type)))
    bound_list = TYPE_ACTUAL_BOUNDS (array_type);

  /* First make the list for a CONSTRUCTOR for the template.  Go down
     the field list of the template instead of the type chain because
     this array might be an Ada array of array and we can't tell where
     the nested array stop being the underlying object.  */
  for (field = TYPE_FIELDS (template_type);
       field;
       field = DECL_CHAIN (DECL_CHAIN (field)))
    {
      tree bounds, min, max;

      /* If we have a bound list, get the bounds from there.  Likewise
	 for an ARRAY_TYPE.  Otherwise, if expr is a PARM_DECL with
	 DECL_BY_COMPONENT_PTR_P, use the bounds of the field in the
	 template, but this will only give us a maximum range.  */
      if (bound_list)
	{
	  bounds = TREE_VALUE (bound_list);
	  bound_list = TREE_CHAIN (bound_list);
	}
      else if (TREE_CODE (array_type) == ARRAY_TYPE)
	{
	  bounds = TYPE_INDEX_TYPE (TYPE_DOMAIN (array_type));
	  array_type = TREE_TYPE (array_type);
	}
      else if (expr && TREE_CODE (expr) == PARM_DECL
	       && DECL_BY_COMPONENT_PTR_P (expr))
	bounds = TREE_TYPE (field);
      else
	gcc_unreachable ();

      min = convert (TREE_TYPE (field), TYPE_MIN_VALUE (bounds));
      max = convert (TREE_TYPE (DECL_CHAIN (field)), TYPE_MAX_VALUE (bounds));

      /* If either MIN or MAX involve a PLACEHOLDER_EXPR, we must
	 substitute it from OBJECT.  */
      min = SUBSTITUTE_PLACEHOLDER_IN_EXPR (min, expr);
      max = SUBSTITUTE_PLACEHOLDER_IN_EXPR (max, expr);

      CONSTRUCTOR_APPEND_ELT (template_elts, field, min);
      CONSTRUCTOR_APPEND_ELT (template_elts, DECL_CHAIN (field), max);
    }

  return gnat_build_constructor (template_type, template_elts);
}

/* Return true if TYPE is suitable for the element type of a vector.  */

static bool
type_for_vector_element_p (tree type)
{
  machine_mode mode;

  if (!INTEGRAL_TYPE_P (type)
      && !SCALAR_FLOAT_TYPE_P (type)
      && !FIXED_POINT_TYPE_P (type))
    return false;

  mode = TYPE_MODE (type);
  if (GET_MODE_CLASS (mode) != MODE_INT
      && !SCALAR_FLOAT_MODE_P (mode)
      && !ALL_SCALAR_FIXED_POINT_MODE_P (mode))
    return false;

  return true;
}

/* Return a vector type given the SIZE and the INNER_TYPE, or NULL_TREE if
   this is not possible.  If ATTRIBUTE is non-zero, we are processing the
   attribute declaration and want to issue error messages on failure.  */

static tree
build_vector_type_for_size (tree inner_type, tree size, tree attribute)
{
  unsigned HOST_WIDE_INT size_int, inner_size_int;
  int nunits;

  /* Silently punt on variable sizes.  We can't make vector types for them,
     need to ignore them on front-end generated subtypes of unconstrained
     base types, and this attribute is for binding implementors, not end
     users, so we should never get there from legitimate explicit uses.  */
  if (!tree_fits_uhwi_p (size))
    return NULL_TREE;
  size_int = tree_to_uhwi (size);

  if (!type_for_vector_element_p (inner_type))
    {
      if (attribute)
	error ("invalid element type for attribute %qs",
	       IDENTIFIER_POINTER (attribute));
      return NULL_TREE;
    }
  inner_size_int = tree_to_uhwi (TYPE_SIZE_UNIT (inner_type));

  if (size_int % inner_size_int)
    {
      if (attribute)
	error ("vector size not an integral multiple of component size");
      return NULL_TREE;
    }

  if (size_int == 0)
    {
      if (attribute)
	error ("zero vector size");
      return NULL_TREE;
    }

  nunits = size_int / inner_size_int;
  if (nunits & (nunits - 1))
    {
      if (attribute)
	error ("number of components of vector not a power of two");
      return NULL_TREE;
    }

  return build_vector_type (inner_type, nunits);
}

/* Return a vector type whose representative array type is ARRAY_TYPE, or
   NULL_TREE if this is not possible.  If ATTRIBUTE is non-zero, we are
   processing the attribute and want to issue error messages on failure.  */

static tree
build_vector_type_for_array (tree array_type, tree attribute)
{
  tree vector_type = build_vector_type_for_size (TREE_TYPE (array_type),
						 TYPE_SIZE_UNIT (array_type),
						 attribute);
  if (!vector_type)
    return NULL_TREE;

  TYPE_REPRESENTATIVE_ARRAY (vector_type) = array_type;
  return vector_type;
}

/* Build a type to be used to represent an aliased object whose nominal type
   is an unconstrained array.  This consists of a RECORD_TYPE containing a
   field of TEMPLATE_TYPE and a field of OBJECT_TYPE, which is an ARRAY_TYPE.
   If ARRAY_TYPE is that of an unconstrained array, this is used to represent
   an arbitrary unconstrained object.  Use NAME as the name of the
   record.  ARTIFICIAL_P is true if the type was generated by the
   compiler, or false if the type came from source.  DEBUG_INFO_P is
   true if we need to write debug information for the type.  */

tree
build_unc_object_type (tree template_type, tree object_type, tree name,
		       bool artificial_p, bool debug_info_p)
{
  tree type = make_node (RECORD_TYPE);
  tree template_field
    = create_field_decl (get_identifier ("BOUNDS"), template_type, type,
			 NULL_TREE, NULL_TREE, 0, 1);
  tree array_field
    = create_field_decl (get_identifier ("ARRAY"), object_type, type,
			 NULL_TREE, NULL_TREE, 0, 1);

  TYPE_NAME (type) = name;
  TYPE_CONTAINS_TEMPLATE_P (type) = 1;
  DECL_CHAIN (template_field) = array_field;
  finish_record_type (type, template_field, 0, true);

  /* Declare it now since it will never be declared otherwise.  This is
     necessary to ensure that its subtrees are properly marked.  */
  create_type_decl (name, type, artificial_p, debug_info_p, Empty);

  return type;
}

/* Same, taking a thin or fat pointer type instead of a template type. */

tree
build_unc_object_type_from_ptr (tree thin_fat_ptr_type, tree object_type,
				tree name, bool debug_info_p)
{
  tree template_type;

  gcc_assert (TYPE_IS_FAT_OR_THIN_POINTER_P (thin_fat_ptr_type));

  template_type
    = (TYPE_IS_FAT_POINTER_P (thin_fat_ptr_type)
       ? TREE_TYPE (TREE_TYPE (DECL_CHAIN (TYPE_FIELDS (thin_fat_ptr_type))))
       : TREE_TYPE (TYPE_FIELDS (TREE_TYPE (thin_fat_ptr_type))));

  return
    build_unc_object_type (template_type, object_type, name, true,
			   debug_info_p);
}

/* Update anything previously pointing to OLD_TYPE to point to NEW_TYPE.
   In the normal case this is just two adjustments, but we have more to
   do if NEW_TYPE is an UNCONSTRAINED_ARRAY_TYPE.  */

void
update_pointer_to (tree old_type, tree new_type)
{
  const tree old_ptr = TYPE_POINTER_TO (old_type);
  const tree old_ref = TYPE_REFERENCE_TO (old_type);
  tree t;

  /* If this is the main variant, process all the other variants first.  */
  if (TYPE_MAIN_VARIANT (old_type) == old_type)
    for (t = TYPE_NEXT_VARIANT (old_type); t; t = TYPE_NEXT_VARIANT (t))
      update_pointer_to (t, new_type);

  /* If no pointers and no references, we are done.  */
  if (!old_ptr && !old_ref)
    return;

  /* Merge the old type qualifiers in the new type.

     Each old variant has qualifiers for specific reasons, and the new
     designated type as well.  Each set of qualifiers represents useful
     information grabbed at some point, and merging the two simply unifies
     these inputs into the final type description.

     Consider for instance a volatile type frozen after an access to constant
     type designating it; after the designated type's freeze, we get here with
     a volatile NEW_TYPE and a dummy OLD_TYPE with a readonly variant, created
     when the access type was processed.  We will make a volatile and readonly
     designated type, because that's what it really is.

     We might also get here for a non-dummy OLD_TYPE variant with different
     qualifiers than those of NEW_TYPE, for instance in some cases of pointers
     to private record type elaboration (see the comments around the call to
     this routine in gnat_to_gnu_entity <E_Access_Type>).  We have to merge
     the qualifiers in those cases too, to avoid accidentally discarding the
     initial set, and will often end up with OLD_TYPE == NEW_TYPE then.  */
  new_type
    = build_qualified_type (new_type,
			    TYPE_QUALS (old_type) | TYPE_QUALS (new_type));

  /* If old type and new type are identical, there is nothing to do.  */
  if (old_type == new_type)
    return;

  /* Otherwise, first handle the simple case.  */
  if (TREE_CODE (new_type) != UNCONSTRAINED_ARRAY_TYPE)
    {
      tree new_ptr, new_ref;
      tree ptr, ref;

      /* If pointer or reference already points to new type, nothing to do.
	 This can happen as update_pointer_to can be invoked multiple times
	 on the same couple of types because of the type variants.  */
      if ((old_ptr && TREE_TYPE (old_ptr) == new_type)
	  || (old_ref && TREE_TYPE (old_ref) == new_type))
	return;

      /* Chain PTR and its variants at the end.  */
      new_ptr = TYPE_POINTER_TO (new_type);
      if (new_ptr)
	{
	  while (TYPE_NEXT_PTR_TO (new_ptr))
	    new_ptr = TYPE_NEXT_PTR_TO (new_ptr);
	  TYPE_NEXT_PTR_TO (new_ptr) = old_ptr;
	}
      else
	TYPE_POINTER_TO (new_type) = old_ptr;

      /* Now adjust them.  */
      for (ptr = old_ptr; ptr; ptr = TYPE_NEXT_PTR_TO (ptr))
	for (t = TYPE_MAIN_VARIANT (ptr); t; t = TYPE_NEXT_VARIANT (t))
	  {
	    TREE_TYPE (t) = new_type;
	    if (TYPE_NULL_BOUNDS (t))
	      TREE_TYPE (TREE_OPERAND (TYPE_NULL_BOUNDS (t), 0)) = new_type;
	    TYPE_CANONICAL (t) = TYPE_CANONICAL (TYPE_POINTER_TO (new_type));
	  }

      /* Chain REF and its variants at the end.  */
      new_ref = TYPE_REFERENCE_TO (new_type);
      if (new_ref)
	{
	  while (TYPE_NEXT_REF_TO (new_ref))
	    new_ref = TYPE_NEXT_REF_TO (new_ref);
	  TYPE_NEXT_REF_TO (new_ref) = old_ref;
	}
      else
	TYPE_REFERENCE_TO (new_type) = old_ref;

      /* Now adjust them.  */
      for (ref = old_ref; ref; ref = TYPE_NEXT_REF_TO (ref))
	for (t = TYPE_MAIN_VARIANT (ref); t; t = TYPE_NEXT_VARIANT (t))
	  {
	    TREE_TYPE (t) = new_type;
	    TYPE_CANONICAL (t) = TYPE_CANONICAL (TYPE_REFERENCE_TO (new_type));
	  }

      TYPE_POINTER_TO (old_type) = NULL_TREE;
      TYPE_REFERENCE_TO (old_type) = NULL_TREE;
    }

  /* Now deal with the unconstrained array case.  In this case the pointer
     is actually a record where both fields are pointers to dummy nodes.
     Turn them into pointers to the correct types using update_pointer_to.
     Likewise for the pointer to the object record (thin pointer).  */
  else
    {
      tree new_ptr = TYPE_POINTER_TO (new_type);

      gcc_assert (TYPE_IS_FAT_POINTER_P (old_ptr));

      /* If PTR already points to NEW_TYPE, nothing to do.  This can happen
	 since update_pointer_to can be invoked multiple times on the same
	 couple of types because of the type variants.  */
      if (TYPE_UNCONSTRAINED_ARRAY (old_ptr) == new_type)
	return;

      update_pointer_to
	(TREE_TYPE (TREE_TYPE (TYPE_FIELDS (old_ptr))),
	 TREE_TYPE (TREE_TYPE (TYPE_FIELDS (new_ptr))));

      update_pointer_to
	(TREE_TYPE (TREE_TYPE (DECL_CHAIN (TYPE_FIELDS (old_ptr)))),
	 TREE_TYPE (TREE_TYPE (DECL_CHAIN (TYPE_FIELDS (new_ptr)))));

      update_pointer_to (TYPE_OBJECT_RECORD_TYPE (old_type),
			 TYPE_OBJECT_RECORD_TYPE (new_type));

      TYPE_POINTER_TO (old_type) = NULL_TREE;
      TYPE_REFERENCE_TO (old_type) = NULL_TREE;
    }
}

/* Convert EXPR, a pointer to a constrained array, into a pointer to an
   unconstrained one.  This involves making or finding a template.  */

static tree
convert_to_fat_pointer (tree type, tree expr)
{
  tree template_type = TREE_TYPE (TREE_TYPE (DECL_CHAIN (TYPE_FIELDS (type))));
  tree p_array_type = TREE_TYPE (TYPE_FIELDS (type));
  tree etype = TREE_TYPE (expr);
  tree template_addr;
  vec<constructor_elt, va_gc> *v;
  vec_alloc (v, 2);

  /* If EXPR is null, make a fat pointer that contains a null pointer to the
     array (compare_fat_pointers ensures that this is the full discriminant)
     and a valid pointer to the bounds.  This latter property is necessary
     since the compiler can hoist the load of the bounds done through it.  */
  if (integer_zerop (expr))
    {
      tree ptr_template_type = TREE_TYPE (DECL_CHAIN (TYPE_FIELDS (type)));
      tree null_bounds, t;

      if (TYPE_NULL_BOUNDS (ptr_template_type))
	null_bounds = TYPE_NULL_BOUNDS (ptr_template_type);
      else
	{
	  /* The template type can still be dummy at this point so we build an
	     empty constructor.  The middle-end will fill it in with zeros.  */
	  t = build_constructor (template_type, NULL);
	  TREE_CONSTANT (t) = TREE_STATIC (t) = 1;
	  null_bounds = build_unary_op (ADDR_EXPR, NULL_TREE, t);
	  SET_TYPE_NULL_BOUNDS (ptr_template_type, null_bounds);
	}

      CONSTRUCTOR_APPEND_ELT (v, TYPE_FIELDS (type),
			      fold_convert (p_array_type, null_pointer_node));
      CONSTRUCTOR_APPEND_ELT (v, DECL_CHAIN (TYPE_FIELDS (type)), null_bounds);
      t = build_constructor (type, v);
      /* Do not set TREE_CONSTANT so as to force T to static memory.  */
      TREE_CONSTANT (t) = 0;
      TREE_STATIC (t) = 1;

      return t;
    }

  /* If EXPR is a thin pointer, make template and data from the record.  */
  if (TYPE_IS_THIN_POINTER_P (etype))
    {
      tree field = TYPE_FIELDS (TREE_TYPE (etype));

      expr = gnat_protect_expr (expr);

      /* If we have a TYPE_UNCONSTRAINED_ARRAY attached to the RECORD_TYPE,
	 the thin pointer value has been shifted so we shift it back to get
	 the template address.  */
      if (TYPE_UNCONSTRAINED_ARRAY (TREE_TYPE (etype)))
	{
	  template_addr
	    = build_binary_op (POINTER_PLUS_EXPR, etype, expr,
			       fold_build1 (NEGATE_EXPR, sizetype,
					    byte_position
					    (DECL_CHAIN (field))));
	  template_addr
	    = fold_convert (TREE_TYPE (DECL_CHAIN (TYPE_FIELDS (type))),
			    template_addr);
	}

      /* Otherwise we explicitly take the address of the fields.  */
      else
	{
	  expr = build_unary_op (INDIRECT_REF, NULL_TREE, expr);
	  template_addr
	    = build_unary_op (ADDR_EXPR, NULL_TREE,
			      build_component_ref (expr, field, false));
	  expr = build_unary_op (ADDR_EXPR, NULL_TREE,
				 build_component_ref (expr, DECL_CHAIN (field),
						      false));
	}
    }

  /* Otherwise, build the constructor for the template.  */
  else
    template_addr
      = build_unary_op (ADDR_EXPR, NULL_TREE,
			build_template (template_type, TREE_TYPE (etype),
					expr));

  /* The final result is a constructor for the fat pointer.

     If EXPR is an argument of a foreign convention subprogram, the type it
     points to is directly the component type.  In this case, the expression
     type may not match the corresponding FIELD_DECL type at this point, so we
     call "convert" here to fix that up if necessary.  This type consistency is
     required, for instance because it ensures that possible later folding of
     COMPONENT_REFs against this constructor always yields something of the
     same type as the initial reference.

     Note that the call to "build_template" above is still fine because it
     will only refer to the provided TEMPLATE_TYPE in this case.  */
  CONSTRUCTOR_APPEND_ELT (v, TYPE_FIELDS (type), convert (p_array_type, expr));
  CONSTRUCTOR_APPEND_ELT (v, DECL_CHAIN (TYPE_FIELDS (type)), template_addr);
  return gnat_build_constructor (type, v);
}

/* Create an expression whose value is that of EXPR,
   converted to type TYPE.  The TREE_TYPE of the value
   is always TYPE.  This function implements all reasonable
   conversions; callers should filter out those that are
   not permitted by the language being compiled.  */

tree
convert (tree type, tree expr)
{
  tree etype = TREE_TYPE (expr);
  enum tree_code ecode = TREE_CODE (etype);
  enum tree_code code = TREE_CODE (type);

  /* If the expression is already of the right type, we are done.  */
  if (etype == type)
    return expr;

  /* If both input and output have padding and are of variable size, do this
     as an unchecked conversion.  Likewise if one is a mere variant of the
     other, so we avoid a pointless unpad/repad sequence.  */
  else if (code == RECORD_TYPE && ecode == RECORD_TYPE
	   && TYPE_PADDING_P (type) && TYPE_PADDING_P (etype)
	   && (!TREE_CONSTANT (TYPE_SIZE (type))
	       || !TREE_CONSTANT (TYPE_SIZE (etype))
	       || TYPE_MAIN_VARIANT (type) == TYPE_MAIN_VARIANT (etype)
	       || TYPE_NAME (TREE_TYPE (TYPE_FIELDS (type)))
		  == TYPE_NAME (TREE_TYPE (TYPE_FIELDS (etype)))))
    ;

  /* If the output type has padding, convert to the inner type and make a
     constructor to build the record, unless a variable size is involved.  */
  else if (code == RECORD_TYPE && TYPE_PADDING_P (type))
    {
      /* If we previously converted from another type and our type is
	 of variable size, remove the conversion to avoid the need for
	 variable-sized temporaries.  Likewise for a conversion between
	 original and packable version.  */
      if (TREE_CODE (expr) == VIEW_CONVERT_EXPR
	  && (!TREE_CONSTANT (TYPE_SIZE (type))
	      || (ecode == RECORD_TYPE
		  && TYPE_NAME (etype)
		     == TYPE_NAME (TREE_TYPE (TREE_OPERAND (expr, 0))))))
	expr = TREE_OPERAND (expr, 0);

      /* If we are just removing the padding from expr, convert the original
	 object if we have variable size in order to avoid the need for some
	 variable-sized temporaries.  Likewise if the padding is a variant
	 of the other, so we avoid a pointless unpad/repad sequence.  */
      if (TREE_CODE (expr) == COMPONENT_REF
	  && TYPE_IS_PADDING_P (TREE_TYPE (TREE_OPERAND (expr, 0)))
	  && (!TREE_CONSTANT (TYPE_SIZE (type))
	      || TYPE_MAIN_VARIANT (type)
		 == TYPE_MAIN_VARIANT (TREE_TYPE (TREE_OPERAND (expr, 0)))
	      || (ecode == RECORD_TYPE
		  && TYPE_NAME (etype)
		     == TYPE_NAME (TREE_TYPE (TYPE_FIELDS (type))))))
	return convert (type, TREE_OPERAND (expr, 0));

      /* If the inner type is of self-referential size and the expression type
	 is a record, do this as an unchecked conversion unless both types are
	 essentially the same.  */
      if (ecode == RECORD_TYPE
	  && CONTAINS_PLACEHOLDER_P (DECL_SIZE (TYPE_FIELDS (type)))
	  && TYPE_MAIN_VARIANT (etype)
	     != TYPE_MAIN_VARIANT (TREE_TYPE (TYPE_FIELDS (type))))
	return unchecked_convert (type, expr, false);

      /* If we are converting between array types with variable size, do the
	 final conversion as an unchecked conversion, again to avoid the need
	 for some variable-sized temporaries.  If valid, this conversion is
	 very likely purely technical and without real effects.  */
      if (ecode == ARRAY_TYPE
	  && TREE_CODE (TREE_TYPE (TYPE_FIELDS (type))) == ARRAY_TYPE
	  && !TREE_CONSTANT (TYPE_SIZE (etype))
	  && !TREE_CONSTANT (TYPE_SIZE (type)))
	return unchecked_convert (type,
				  convert (TREE_TYPE (TYPE_FIELDS (type)),
					   expr),
				  false);

      tree t = convert (TREE_TYPE (TYPE_FIELDS (type)), expr);

      /* If converting to the inner type has already created a CONSTRUCTOR with
         the right size, then reuse it instead of creating another one.  This
         can happen for the padding type built to overalign local variables.  */
      if (TREE_CODE (t) == VIEW_CONVERT_EXPR
	  && TREE_CODE (TREE_OPERAND (t, 0)) == CONSTRUCTOR
	  && TREE_CONSTANT (TYPE_SIZE (TREE_TYPE (TREE_OPERAND (t, 0))))
	  && tree_int_cst_equal (TYPE_SIZE (type),
				 TYPE_SIZE (TREE_TYPE (TREE_OPERAND (t, 0)))))
	return build1 (VIEW_CONVERT_EXPR, type, TREE_OPERAND (t, 0));

      vec<constructor_elt, va_gc> *v;
      vec_alloc (v, 1);
      CONSTRUCTOR_APPEND_ELT (v, TYPE_FIELDS (type), t);
      return gnat_build_constructor (type, v);
    }

  /* If the input type has padding, remove it and convert to the output type.
     The conditions ordering is arranged to ensure that the output type is not
     a padding type here, as it is not clear whether the conversion would
     always be correct if this was to happen.  */
  else if (ecode == RECORD_TYPE && TYPE_PADDING_P (etype))
    {
      tree unpadded;

      /* If we have just converted to this padded type, just get the
	 inner expression.  */
      if (TREE_CODE (expr) == CONSTRUCTOR)
	unpadded = CONSTRUCTOR_ELT (expr, 0)->value;

      /* Otherwise, build an explicit component reference.  */
      else
	unpadded = build_component_ref (expr, TYPE_FIELDS (etype), false);

      return convert (type, unpadded);
    }

  /* If the input is a biased type, convert first to the base type and add
     the bias.  Note that the bias must go through a full conversion to the
     base type, lest it is itself a biased value; this happens for subtypes
     of biased types.  */
  if (ecode == INTEGER_TYPE && TYPE_BIASED_REPRESENTATION_P (etype))
    return convert (type, fold_build2 (PLUS_EXPR, TREE_TYPE (etype),
				       fold_convert (TREE_TYPE (etype), expr),
				       convert (TREE_TYPE (etype),
						TYPE_MIN_VALUE (etype))));

  /* If the input is a justified modular type, we need to extract the actual
     object before converting it to an other type with the exceptions of an
     [unconstrained] array or a mere type variant.  It is useful to avoid
     the extraction and conversion in these cases because it could end up
     replacing a VAR_DECL by a constructor and we might be about the take
     the address of the result.  */
  if (ecode == RECORD_TYPE && TYPE_JUSTIFIED_MODULAR_P (etype)
      && code != ARRAY_TYPE
      && code != UNCONSTRAINED_ARRAY_TYPE
      && TYPE_MAIN_VARIANT (type) != TYPE_MAIN_VARIANT (etype))
    return
      convert (type, build_component_ref (expr, TYPE_FIELDS (etype), false));

  /* If converting to a type that contains a template, convert to the data
     type and then build the template. */
  if (code == RECORD_TYPE && TYPE_CONTAINS_TEMPLATE_P (type))
    {
      tree obj_type = TREE_TYPE (DECL_CHAIN (TYPE_FIELDS (type)));
      vec<constructor_elt, va_gc> *v;
      vec_alloc (v, 2);

      /* If the source already has a template, get a reference to the
	 associated array only, as we are going to rebuild a template
	 for the target type anyway.  */
      expr = maybe_unconstrained_array (expr);

      CONSTRUCTOR_APPEND_ELT (v, TYPE_FIELDS (type),
			      build_template (TREE_TYPE (TYPE_FIELDS (type)),
					      obj_type, NULL_TREE));
      if (expr)
	CONSTRUCTOR_APPEND_ELT (v, DECL_CHAIN (TYPE_FIELDS (type)),
				convert (obj_type, expr));
      return gnat_build_constructor (type, v);
    }

  /* There are some cases of expressions that we process specially.  */
  switch (TREE_CODE (expr))
    {
    case ERROR_MARK:
      return expr;

    case NULL_EXPR:
      /* Just set its type here.  For TRANSFORM_EXPR, we will do the actual
	 conversion in gnat_expand_expr.  NULL_EXPR does not represent
	 and actual value, so no conversion is needed.  */
      expr = copy_node (expr);
      TREE_TYPE (expr) = type;
      return expr;

    case STRING_CST:
      /* If we are converting a STRING_CST to another constrained array type,
	 just make a new one in the proper type.  */
      if (code == ecode
	  && !(TREE_CONSTANT (TYPE_SIZE (etype))
	       && !TREE_CONSTANT (TYPE_SIZE (type))))
	{
	  expr = copy_node (expr);
	  TREE_TYPE (expr) = type;
	  return expr;
	}
      break;

    case VECTOR_CST:
      /* If we are converting a VECTOR_CST to a mere type variant, just make
	 a new one in the proper type.  */
      if (code == ecode && gnat_types_compatible_p (type, etype))
	{
	  expr = copy_node (expr);
	  TREE_TYPE (expr) = type;
	  return expr;
	}
      break;

    case CONSTRUCTOR:
      /* If we are converting a CONSTRUCTOR to a mere type variant, or to
	 another padding type around the same type, just make a new one in
	 the proper type.  */
      if (code == ecode
	  && (gnat_types_compatible_p (type, etype)
	      || (code == RECORD_TYPE
		  && TYPE_PADDING_P (type) && TYPE_PADDING_P (etype)
		  && TREE_TYPE (TYPE_FIELDS (type))
		     == TREE_TYPE (TYPE_FIELDS (etype)))))
	{
	  expr = copy_node (expr);
	  TREE_TYPE (expr) = type;
	  CONSTRUCTOR_ELTS (expr) = vec_safe_copy (CONSTRUCTOR_ELTS (expr));
	  return expr;
	}

      /* Likewise for a conversion between original and packable version, or
	 conversion between types of the same size and with the same list of
	 fields, but we have to work harder to preserve type consistency.  */
      if (code == ecode
	  && code == RECORD_TYPE
	  && (TYPE_NAME (type) == TYPE_NAME (etype)
	      || tree_int_cst_equal (TYPE_SIZE (type), TYPE_SIZE (etype))))

	{
	  vec<constructor_elt, va_gc> *e = CONSTRUCTOR_ELTS (expr);
	  unsigned HOST_WIDE_INT len = vec_safe_length (e);
	  vec<constructor_elt, va_gc> *v;
	  vec_alloc (v, len);
	  tree efield = TYPE_FIELDS (etype), field = TYPE_FIELDS (type);
	  unsigned HOST_WIDE_INT idx;
	  tree index, value;

	  /* Whether we need to clear TREE_CONSTANT et al. on the output
	     constructor when we convert in place.  */
	  bool clear_constant = false;

	  FOR_EACH_CONSTRUCTOR_ELT(e, idx, index, value)
	    {
	      /* Skip the missing fields in the CONSTRUCTOR.  */
	      while (efield && field && !SAME_FIELD_P (efield, index))
	        {
		  efield = DECL_CHAIN (efield);
		  field = DECL_CHAIN (field);
		}
	      /* The field must be the same.  */
	      if (!(efield && field && SAME_FIELD_P (efield, field)))
		break;
	      constructor_elt elt
	        = {field, convert (TREE_TYPE (field), value)};
	      v->quick_push (elt);

	      /* If packing has made this field a bitfield and the input
		 value couldn't be emitted statically any more, we need to
		 clear TREE_CONSTANT on our output.  */
	      if (!clear_constant
		  && TREE_CONSTANT (expr)
		  && !CONSTRUCTOR_BITFIELD_P (efield)
		  && CONSTRUCTOR_BITFIELD_P (field)
		  && !initializer_constant_valid_for_bitfield_p (value))
		clear_constant = true;

	      efield = DECL_CHAIN (efield);
	      field = DECL_CHAIN (field);
	    }

	  /* If we have been able to match and convert all the input fields
	     to their output type, convert in place now.  We'll fallback to a
	     view conversion downstream otherwise.  */
	  if (idx == len)
	    {
	      expr = copy_node (expr);
	      TREE_TYPE (expr) = type;
	      CONSTRUCTOR_ELTS (expr) = v;
	      if (clear_constant)
		TREE_CONSTANT (expr) = TREE_STATIC (expr) = 0;
	      return expr;
	    }
	}

      /* Likewise for a conversion between array type and vector type with a
         compatible representative array.  */
      else if (code == VECTOR_TYPE
	       && ecode == ARRAY_TYPE
	       && gnat_types_compatible_p (TYPE_REPRESENTATIVE_ARRAY (type),
					   etype))
	{
	  vec<constructor_elt, va_gc> *e = CONSTRUCTOR_ELTS (expr);
	  unsigned HOST_WIDE_INT len = vec_safe_length (e);
	  vec<constructor_elt, va_gc> *v;
	  unsigned HOST_WIDE_INT ix;
	  tree value;

	  /* Build a VECTOR_CST from a *constant* array constructor.  */
	  if (TREE_CONSTANT (expr))
	    {
	      bool constant_p = true;

	      /* Iterate through elements and check if all constructor
		 elements are *_CSTs.  */
	      FOR_EACH_CONSTRUCTOR_VALUE (e, ix, value)
		if (!CONSTANT_CLASS_P (value))
		  {
		    constant_p = false;
		    break;
		  }

	      if (constant_p)
		return build_vector_from_ctor (type,
					       CONSTRUCTOR_ELTS (expr));
	    }

	  /* Otherwise, build a regular vector constructor.  */
	  vec_alloc (v, len);
	  FOR_EACH_CONSTRUCTOR_VALUE (e, ix, value)
	    {
	      constructor_elt elt = {NULL_TREE, value};
	      v->quick_push (elt);
	    }
	  expr = copy_node (expr);
	  TREE_TYPE (expr) = type;
	  CONSTRUCTOR_ELTS (expr) = v;
	  return expr;
	}
      break;

    case UNCONSTRAINED_ARRAY_REF:
      /* First retrieve the underlying array.  */
      expr = maybe_unconstrained_array (expr);
      etype = TREE_TYPE (expr);
      ecode = TREE_CODE (etype);
      break;

    case VIEW_CONVERT_EXPR:
      {
	/* GCC 4.x is very sensitive to type consistency overall, and view
	   conversions thus are very frequent.  Even though just "convert"ing
	   the inner operand to the output type is fine in most cases, it
	   might expose unexpected input/output type mismatches in special
	   circumstances so we avoid such recursive calls when we can.  */
	tree op0 = TREE_OPERAND (expr, 0);

	/* If we are converting back to the original type, we can just
	   lift the input conversion.  This is a common occurrence with
	   switches back-and-forth amongst type variants.  */
	if (type == TREE_TYPE (op0))
	  return op0;

	/* Otherwise, if we're converting between two aggregate or vector
	   types, we might be allowed to substitute the VIEW_CONVERT_EXPR
	   target type in place or to just convert the inner expression.  */
	if ((AGGREGATE_TYPE_P (type) && AGGREGATE_TYPE_P (etype))
	    || (VECTOR_TYPE_P (type) && VECTOR_TYPE_P (etype)))
	  {
	    /* If we are converting between mere variants, we can just
	       substitute the VIEW_CONVERT_EXPR in place.  */
	    if (gnat_types_compatible_p (type, etype))
	      return build1 (VIEW_CONVERT_EXPR, type, op0);

	    /* Otherwise, we may just bypass the input view conversion unless
	       one of the types is a fat pointer,  which is handled by
	       specialized code below which relies on exact type matching.  */
	    else if (!TYPE_IS_FAT_POINTER_P (type)
		     && !TYPE_IS_FAT_POINTER_P (etype))
	      return convert (type, op0);
	  }

	break;
      }

    default:
      break;
    }

  /* Check for converting to a pointer to an unconstrained array.  */
  if (TYPE_IS_FAT_POINTER_P (type) && !TYPE_IS_FAT_POINTER_P (etype))
    return convert_to_fat_pointer (type, expr);

  /* If we are converting between two aggregate or vector types that are mere
     variants, just make a VIEW_CONVERT_EXPR.  Likewise when we are converting
     to a vector type from its representative array type.  */
  else if ((code == ecode
	    && (AGGREGATE_TYPE_P (type) || VECTOR_TYPE_P (type))
	    && gnat_types_compatible_p (type, etype))
	   || (code == VECTOR_TYPE
	       && ecode == ARRAY_TYPE
	       && gnat_types_compatible_p (TYPE_REPRESENTATIVE_ARRAY (type),
					   etype)))
    return build1 (VIEW_CONVERT_EXPR, type, expr);

  /* If we are converting between tagged types, try to upcast properly.
     But don't do it if we are just annotating types since tagged types
     aren't fully laid out in this mode.  */
  else if (ecode == RECORD_TYPE && code == RECORD_TYPE
	   && TYPE_ALIGN_OK (etype) && TYPE_ALIGN_OK (type)
	   && !type_annotate_only)
    {
      tree child_etype = etype;
      do {
	tree field = TYPE_FIELDS (child_etype);
	if (DECL_NAME (field) == parent_name_id && TREE_TYPE (field) == type)
	  return build_component_ref (expr, field, false);
	child_etype = TREE_TYPE (field);
      } while (TREE_CODE (child_etype) == RECORD_TYPE);
    }

  /* If we are converting from a smaller form of record type back to it, just
     make a VIEW_CONVERT_EXPR.  But first pad the expression to have the same
     size on both sides.  */
  else if (ecode == RECORD_TYPE && code == RECORD_TYPE
	   && smaller_form_type_p (etype, type))
    {
      expr = convert (maybe_pad_type (etype, TYPE_SIZE (type), 0, Empty,
				      false, false, true),
		      expr);
      return build1 (VIEW_CONVERT_EXPR, type, expr);
    }

  /* In all other cases of related types, make a NOP_EXPR.  */
  else if (TYPE_MAIN_VARIANT (type) == TYPE_MAIN_VARIANT (etype))
    return fold_convert (type, expr);

  switch (code)
    {
    case VOID_TYPE:
      return fold_build1 (CONVERT_EXPR, type, expr);

    case INTEGER_TYPE:
      if (TYPE_HAS_ACTUAL_BOUNDS_P (type)
	  && (ecode == ARRAY_TYPE || ecode == UNCONSTRAINED_ARRAY_TYPE
	      || (ecode == RECORD_TYPE && TYPE_CONTAINS_TEMPLATE_P (etype))))
	return unchecked_convert (type, expr, false);

      /* If the output is a biased type, convert first to the base type and
	 subtract the bias.  Note that the bias itself must go through a full
	 conversion to the base type, lest it is a biased value; this happens
	 for subtypes of biased types.  */
      if (TYPE_BIASED_REPRESENTATION_P (type))
	return fold_convert (type,
			     fold_build2 (MINUS_EXPR, TREE_TYPE (type),
					  convert (TREE_TYPE (type), expr),
					  convert (TREE_TYPE (type),
						   TYPE_MIN_VALUE (type))));

      /* If we are converting an additive expression to an integer type
	 with lower precision, be wary of the optimization that can be
	 applied by convert_to_integer.  There are 2 problematic cases:
	   - if the first operand was originally of a biased type,
	     because we could be recursively called to convert it
	     to an intermediate type and thus rematerialize the
	     additive operator endlessly,
	   - if the expression contains a placeholder, because an
	     intermediate conversion that changes the sign could
	     be inserted and thus introduce an artificial overflow
	     at compile time when the placeholder is substituted.  */
      if (ecode == INTEGER_TYPE
	  && TYPE_PRECISION (type) < TYPE_PRECISION (etype)
	  && (TREE_CODE (expr) == PLUS_EXPR || TREE_CODE (expr) == MINUS_EXPR))
	{
	  tree op0 = get_unwidened (TREE_OPERAND (expr, 0), type);

	  if ((TREE_CODE (TREE_TYPE (op0)) == INTEGER_TYPE
	       && TYPE_BIASED_REPRESENTATION_P (TREE_TYPE (op0)))
	      || CONTAINS_PLACEHOLDER_P (expr))
	    return fold_convert (type, expr);
	}

      /* ... fall through ... */

    case ENUMERAL_TYPE:
      return fold (convert_to_integer (type, expr));

    case BOOLEAN_TYPE:
      /* Do not use convert_to_integer with boolean types.  */
      return fold_convert_loc (EXPR_LOCATION (expr), type, expr);

    case POINTER_TYPE:
    case REFERENCE_TYPE:
      /* If converting between two thin pointers, adjust if needed to account
	 for differing offsets from the base pointer, depending on whether
	 there is a TYPE_UNCONSTRAINED_ARRAY attached to the record type.  */
      if (TYPE_IS_THIN_POINTER_P (etype) && TYPE_IS_THIN_POINTER_P (type))
	{
	  tree etype_pos
	    = TYPE_UNCONSTRAINED_ARRAY (TREE_TYPE (etype))
	      ? byte_position (DECL_CHAIN (TYPE_FIELDS (TREE_TYPE (etype))))
	      : size_zero_node;
	  tree type_pos
	    = TYPE_UNCONSTRAINED_ARRAY (TREE_TYPE (type))
	      ? byte_position (DECL_CHAIN (TYPE_FIELDS (TREE_TYPE (type))))
	      : size_zero_node;
	  tree byte_diff = size_diffop (type_pos, etype_pos);

	  expr = fold_convert (type, expr);
	  if (integer_zerop (byte_diff))
	    return expr;

	  return build_binary_op (POINTER_PLUS_EXPR, type, expr,
				  fold_convert (sizetype, byte_diff));
	}

      /* If converting from a thin pointer with zero offset from the base to
	 a pointer to the array, add the offset of the array field.  */
      if (TYPE_IS_THIN_POINTER_P (etype)
	  && !TYPE_UNCONSTRAINED_ARRAY (TREE_TYPE (etype)))
	{
	  tree arr_field = DECL_CHAIN (TYPE_FIELDS (TREE_TYPE (etype)));

	  if (TREE_TYPE (type) == TREE_TYPE (arr_field))
	    {
	      expr = fold_convert (type, expr);
	      return build_binary_op (POINTER_PLUS_EXPR, type, expr,
				      byte_position (arr_field));
	    }
	}

      /* If converting fat pointer to normal or thin pointer, get the pointer
	 to the array and then convert it.  */
      if (TYPE_IS_FAT_POINTER_P (etype))
	expr = build_component_ref (expr, TYPE_FIELDS (etype), false);

      return fold (convert_to_pointer (type, expr));

    case REAL_TYPE:
      return fold (convert_to_real (type, expr));

    case RECORD_TYPE:
      /* Do a normal conversion between scalar and justified modular type.  */
      if (TYPE_JUSTIFIED_MODULAR_P (type) && !AGGREGATE_TYPE_P (etype))
	{
	  vec<constructor_elt, va_gc> *v;
	  vec_alloc (v, 1);

	  CONSTRUCTOR_APPEND_ELT (v, TYPE_FIELDS (type),
				  convert (TREE_TYPE (TYPE_FIELDS (type)),
					   expr));
	  return gnat_build_constructor (type, v);
	}

      /* In these cases, assume the front-end has validated the conversion.
	 If the conversion is valid, it will be a bit-wise conversion, so
	 it can be viewed as an unchecked conversion.  */
      return unchecked_convert (type, expr, false);

    case ARRAY_TYPE:
      /* Do a normal conversion between unconstrained and constrained array
	 type, assuming the latter is a constrained version of the former.  */
      if (TREE_CODE (expr) == INDIRECT_REF
	  && ecode == ARRAY_TYPE
	  && TREE_TYPE (etype) == TREE_TYPE (type))
	{
	  tree ptr_type = build_pointer_type (type);
	  tree t = build_unary_op (INDIRECT_REF, NULL_TREE,
				   fold_convert (ptr_type,
						 TREE_OPERAND (expr, 0)));
	  TREE_READONLY (t) = TREE_READONLY (expr);
	  TREE_THIS_NOTRAP (t) = TREE_THIS_NOTRAP (expr);
	  return t;
	}

      /* In these cases, assume the front-end has validated the conversion.
	 If the conversion is valid, it will be a bit-wise conversion, so
	 it can be viewed as an unchecked conversion.  */
      return unchecked_convert (type, expr, false);

    case UNION_TYPE:
      /* This is a either a conversion between a tagged type and some
	 subtype, which we have to mark as a UNION_TYPE because of
	 overlapping fields or a conversion of an Unchecked_Union.  */
      return unchecked_convert (type, expr, false);

    case UNCONSTRAINED_ARRAY_TYPE:
      /* If the input is a VECTOR_TYPE, convert to the representative
	 array type first.  */
      if (ecode == VECTOR_TYPE)
	{
	  expr = convert (TYPE_REPRESENTATIVE_ARRAY (etype), expr);
	  etype = TREE_TYPE (expr);
	  ecode = TREE_CODE (etype);
	}

      /* If EXPR is a constrained array, take its address, convert it to a
	 fat pointer, and then dereference it.  Likewise if EXPR is a
	 record containing both a template and a constrained array.
	 Note that a record representing a justified modular type
	 always represents a packed constrained array.  */
      if (ecode == ARRAY_TYPE
	  || (ecode == INTEGER_TYPE && TYPE_HAS_ACTUAL_BOUNDS_P (etype))
	  || (ecode == RECORD_TYPE && TYPE_CONTAINS_TEMPLATE_P (etype))
	  || (ecode == RECORD_TYPE && TYPE_JUSTIFIED_MODULAR_P (etype)))
	return
	  build_unary_op
	    (INDIRECT_REF, NULL_TREE,
	     convert_to_fat_pointer (TREE_TYPE (type),
				     build_unary_op (ADDR_EXPR,
						     NULL_TREE, expr)));

      /* Do something very similar for converting one unconstrained
	 array to another.  */
      else if (ecode == UNCONSTRAINED_ARRAY_TYPE)
	return
	  build_unary_op (INDIRECT_REF, NULL_TREE,
			  convert (TREE_TYPE (type),
				   build_unary_op (ADDR_EXPR,
						   NULL_TREE, expr)));
      else
	gcc_unreachable ();

    case COMPLEX_TYPE:
      return fold (convert_to_complex (type, expr));

    default:
      gcc_unreachable ();
    }
}

/* Create an expression whose value is that of EXPR converted to the common
   index type, which is sizetype.  EXPR is supposed to be in the base type
   of the GNAT index type.  Calling it is equivalent to doing

     convert (sizetype, expr)

   but we try to distribute the type conversion with the knowledge that EXPR
   cannot overflow in its type.  This is a best-effort approach and we fall
   back to the above expression as soon as difficulties are encountered.

   This is necessary to overcome issues that arise when the GNAT base index
   type and the GCC common index type (sizetype) don't have the same size,
   which is quite frequent on 64-bit architectures.  In this case, and if
   the GNAT base index type is signed but the iteration type of the loop has
   been forced to unsigned, the loop scalar evolution engine cannot compute
   a simple evolution for the general induction variables associated with the
   array indices, because it will preserve the wrap-around semantics in the
   unsigned type of their "inner" part.  As a result, many loop optimizations
   are blocked.

   The solution is to use a special (basic) induction variable that is at
   least as large as sizetype, and to express the aforementioned general
   induction variables in terms of this induction variable, eliminating
   the problematic intermediate truncation to the GNAT base index type.
   This is possible as long as the original expression doesn't overflow
   and if the middle-end hasn't introduced artificial overflows in the
   course of the various simplification it can make to the expression.  */

tree
convert_to_index_type (tree expr)
{
  enum tree_code code = TREE_CODE (expr);
  tree type = TREE_TYPE (expr);

  /* If the type is unsigned, overflow is allowed so we cannot be sure that
     EXPR doesn't overflow.  Keep it simple if optimization is disabled.  */
  if (TYPE_UNSIGNED (type) || !optimize || optimize_debug)
    return convert (sizetype, expr);

  switch (code)
    {
    case VAR_DECL:
      /* The main effect of the function: replace a loop parameter with its
	 associated special induction variable.  */
      if (DECL_LOOP_PARM_P (expr) && DECL_INDUCTION_VAR (expr))
	expr = DECL_INDUCTION_VAR (expr);
      break;

    CASE_CONVERT:
      {
	tree otype = TREE_TYPE (TREE_OPERAND (expr, 0));
	/* Bail out as soon as we suspect some sort of type frobbing.  */
	if (TYPE_PRECISION (type) != TYPE_PRECISION (otype)
	    || TYPE_UNSIGNED (type) != TYPE_UNSIGNED (otype))
	  break;
      }

      /* ... fall through ... */

    case NON_LVALUE_EXPR:
      return fold_build1 (code, sizetype,
			  convert_to_index_type (TREE_OPERAND (expr, 0)));

    case PLUS_EXPR:
    case MINUS_EXPR:
    case MULT_EXPR:
      return fold_build2 (code, sizetype,
			  convert_to_index_type (TREE_OPERAND (expr, 0)),
			  convert_to_index_type (TREE_OPERAND (expr, 1)));

    case COMPOUND_EXPR:
      return fold_build2 (code, sizetype, TREE_OPERAND (expr, 0),
			  convert_to_index_type (TREE_OPERAND (expr, 1)));

    case COND_EXPR:
      return fold_build3 (code, sizetype, TREE_OPERAND (expr, 0),
			  convert_to_index_type (TREE_OPERAND (expr, 1)),
			  convert_to_index_type (TREE_OPERAND (expr, 2)));

    default:
      break;
    }

  return convert (sizetype, expr);
}

/* Remove all conversions that are done in EXP.  This includes converting
   from a padded type or to a justified modular type.  If TRUE_ADDRESS
   is true, always return the address of the containing object even if
   the address is not bit-aligned.  */

tree
remove_conversions (tree exp, bool true_address)
{
  switch (TREE_CODE (exp))
    {
    case CONSTRUCTOR:
      if (true_address
	  && TREE_CODE (TREE_TYPE (exp)) == RECORD_TYPE
	  && TYPE_JUSTIFIED_MODULAR_P (TREE_TYPE (exp)))
	return
	  remove_conversions (CONSTRUCTOR_ELT (exp, 0)->value, true);
      break;

    case COMPONENT_REF:
      if (TYPE_IS_PADDING_P (TREE_TYPE (TREE_OPERAND (exp, 0))))
	return remove_conversions (TREE_OPERAND (exp, 0), true_address);
      break;

    CASE_CONVERT:
    case VIEW_CONVERT_EXPR:
    case NON_LVALUE_EXPR:
      return remove_conversions (TREE_OPERAND (exp, 0), true_address);

    default:
      break;
    }

  return exp;
}

/* If EXP's type is an UNCONSTRAINED_ARRAY_TYPE, return an expression that
   refers to the underlying array.  If it has TYPE_CONTAINS_TEMPLATE_P,
   likewise return an expression pointing to the underlying array.  */

tree
maybe_unconstrained_array (tree exp)
{
  enum tree_code code = TREE_CODE (exp);
  tree type = TREE_TYPE (exp);

  switch (TREE_CODE (type))
    {
    case UNCONSTRAINED_ARRAY_TYPE:
      if (code == UNCONSTRAINED_ARRAY_REF)
	{
	  const bool read_only = TREE_READONLY (exp);
	  const bool no_trap = TREE_THIS_NOTRAP (exp);

	  exp = TREE_OPERAND (exp, 0);
	  type = TREE_TYPE (exp);

	  if (TREE_CODE (exp) == COND_EXPR)
	    {
	      tree op1
		= build_unary_op (INDIRECT_REF, NULL_TREE,
				  build_component_ref (TREE_OPERAND (exp, 1),
						       TYPE_FIELDS (type),
						       false));
	      tree op2
		= build_unary_op (INDIRECT_REF, NULL_TREE,
				  build_component_ref (TREE_OPERAND (exp, 2),
						       TYPE_FIELDS (type),
						       false));

	      exp = build3 (COND_EXPR,
			    TREE_TYPE (TREE_TYPE (TYPE_FIELDS (type))),
			    TREE_OPERAND (exp, 0), op1, op2);
	    }
	  else
	    {
	      exp = build_unary_op (INDIRECT_REF, NULL_TREE,
				    build_component_ref (exp,
							 TYPE_FIELDS (type),
						         false));
	      TREE_READONLY (exp) = read_only;
	      TREE_THIS_NOTRAP (exp) = no_trap;
	    }
	}

      else if (code == LOAD_EXPR)
	{
	  const Entity_Id gnat_smo = tree_to_shwi (TREE_OPERAND (exp, 1));
	  tree t = maybe_unconstrained_array (TREE_OPERAND (exp, 0));
	  exp = build_storage_model_load (gnat_smo, t);
	}

      else if (code == NULL_EXPR)
	exp = build1 (NULL_EXPR,
		      TREE_TYPE (TREE_TYPE (TYPE_FIELDS (TREE_TYPE (type)))),
		      TREE_OPERAND (exp, 0));
      break;

    case RECORD_TYPE:
      /* If this is a padded type and it contains a template, convert to the
	 unpadded type first.  */
      if (TYPE_PADDING_P (type)
	  && TREE_CODE (TREE_TYPE (TYPE_FIELDS (type))) == RECORD_TYPE
	  && TYPE_CONTAINS_TEMPLATE_P (TREE_TYPE (TYPE_FIELDS (type))))
	{
	  exp = convert (TREE_TYPE (TYPE_FIELDS (type)), exp);
	  code = TREE_CODE (exp);
	  type = TREE_TYPE (exp);
	}

      if (TYPE_CONTAINS_TEMPLATE_P (type))
	{
	  /* If the array initializer is a box, return NULL_TREE.  */
	  if (code == CONSTRUCTOR && CONSTRUCTOR_NELTS (exp) < 2)
	    return NULL_TREE;

	  exp = build_component_ref (exp, DECL_CHAIN (TYPE_FIELDS (type)),
				     false);

	  /* If the array is padded, remove the padding.  */
	  exp = maybe_padded_object (exp);
	}
      break;

    default:
      break;
    }

  return exp;
}

/* Return true if EXPR is an expression that can be folded as an operand
   of a VIEW_CONVERT_EXPR.  See ada-tree.h for a complete rationale.  */

static bool
can_fold_for_view_convert_p (tree expr)
{
  tree t1, t2;

  /* The folder will fold NOP_EXPRs between integral types with the same
     precision (in the middle-end's sense).  We cannot allow it if the
     types don't have the same precision in the Ada sense as well.  */
  if (TREE_CODE (expr) != NOP_EXPR)
    return true;

  t1 = TREE_TYPE (expr);
  t2 = TREE_TYPE (TREE_OPERAND (expr, 0));

  /* Defer to the folder for non-integral conversions.  */
  if (!(INTEGRAL_TYPE_P (t1) && INTEGRAL_TYPE_P (t2)))
    return true;

  /* Only fold conversions that preserve both precisions.  */
  if (TYPE_PRECISION (t1) == TYPE_PRECISION (t2)
      && operand_equal_p (rm_size (t1), rm_size (t2), 0))
    return true;

  return false;
}

/* Return an expression that does an unchecked conversion of EXPR to TYPE.
   If NOTRUNC_P is true, truncation operations should be suppressed.

   Special care is required with (source or target) integral types whose
   precision is not equal to their size, to make sure we fetch or assign
   the value bits whose location might depend on the endianness, e.g.

     Rmsize : constant := 8;
     subtype Int is Integer range 0 .. 2 ** Rmsize - 1;

     type Bit_Array is array (1 .. Rmsize) of Boolean;
     pragma Pack (Bit_Array);

     function To_Bit_Array is new Unchecked_Conversion (Int, Bit_Array);

     Value : Int := 2#1000_0001#;
     Vbits : Bit_Array := To_Bit_Array (Value);

   we expect the 8 bits at Vbits'Address to always contain Value, while
   their original location depends on the endianness, at Value'Address
   on a little-endian architecture but not on a big-endian one.

   One pitfall is that we cannot use TYPE_UNSIGNED directly to decide how
   the bits between the precision and the size are filled, because of the
   trick used in the E_Signed_Integer_Subtype case of gnat_to_gnu_entity.
   So we use the special predicate type_unsigned_for_rm above.  */

tree
unchecked_convert (tree type, tree expr, bool notrunc_p)
{
  tree etype = TREE_TYPE (expr);
  enum tree_code ecode = TREE_CODE (etype);
  enum tree_code code = TREE_CODE (type);
  const bool ebiased
    = (ecode == INTEGER_TYPE && TYPE_BIASED_REPRESENTATION_P (etype));
  const bool biased
    = (code == INTEGER_TYPE && TYPE_BIASED_REPRESENTATION_P (type));
  const bool ereverse
    = (AGGREGATE_TYPE_P (etype) && TYPE_REVERSE_STORAGE_ORDER (etype));
  const bool reverse
    = (AGGREGATE_TYPE_P (type) && TYPE_REVERSE_STORAGE_ORDER (type));
  tree tem;
  int c = 0;

  /* If the expression is already of the right type, we are done.  */
  if (etype == type)
    return expr;

  /* If both types are integral or regular pointer, then just do a normal
     conversion.  Likewise for a conversion to an unconstrained array.  */
  if (((INTEGRAL_TYPE_P (type)
	|| (POINTER_TYPE_P (type) && !TYPE_IS_THIN_POINTER_P (type))
	|| (code == RECORD_TYPE && TYPE_JUSTIFIED_MODULAR_P (type)))
       && (INTEGRAL_TYPE_P (etype)
	   || (POINTER_TYPE_P (etype) && !TYPE_IS_THIN_POINTER_P (etype))
	   || (ecode == RECORD_TYPE && TYPE_JUSTIFIED_MODULAR_P (etype))))
      || code == UNCONSTRAINED_ARRAY_TYPE)
    {
      if (ebiased)
	{
	  tree ntype = copy_type (etype);
	  TYPE_BIASED_REPRESENTATION_P (ntype) = 0;
	  TYPE_MAIN_VARIANT (ntype) = ntype;
	  expr = build1 (NOP_EXPR, ntype, expr);
	}

      if (biased)
	{
	  tree rtype = copy_type (type);
	  TYPE_BIASED_REPRESENTATION_P (rtype) = 0;
	  TYPE_MAIN_VARIANT (rtype) = rtype;
	  expr = convert (rtype, expr);
	  expr = build1 (NOP_EXPR, type, expr);
	}
      else
	expr = convert (type, expr);
    }

  /* If we are converting to an integral type whose precision is not equal
     to its size, first unchecked convert to a record type that contains a
     field of the given precision.  Then extract the result from the field.

     There is a subtlety if the source type is an aggregate type with reverse
     storage order because its representation is not contiguous in the native
     storage order, i.e. a direct unchecked conversion to an integral type
     with N bits of precision cannot read the first N bits of the aggregate
     type.  To overcome it, we do an unchecked conversion to an integral type
     with reverse storage order and return the resulting value.  This also
     ensures that the result of the unchecked conversion doesn't depend on
     the endianness of the target machine, but only on the storage order of
     the aggregate type.

     Finally, for the sake of consistency, we do the unchecked conversion
     to an integral type with reverse storage order as soon as the source
     type is an aggregate type with reverse storage order, even if there
     are no considerations of precision or size involved.  Ultimately, we
     further extend this processing to any scalar type.  */
  else if ((INTEGRAL_TYPE_P (type)
	    && TYPE_RM_SIZE (type)
	    && ((c = tree_int_cst_compare (TYPE_RM_SIZE (type),
					   TYPE_SIZE (type))) < 0
		|| ereverse))
	   || (SCALAR_FLOAT_TYPE_P (type) && ereverse))
    {
      tree rec_type = make_node (RECORD_TYPE);
      tree field_type, field;

      TYPE_REVERSE_STORAGE_ORDER (rec_type) = ereverse;

      if (c < 0)
	{
	  const unsigned HOST_WIDE_INT prec
	    = TREE_INT_CST_LOW (TYPE_RM_SIZE (type));
	  if (type_unsigned_for_rm (type))
	    field_type = make_unsigned_type (prec);
	  else
	    field_type = make_signed_type (prec);
	  SET_TYPE_RM_SIZE (field_type, TYPE_RM_SIZE (type));
	}
      else
	field_type = type;

      field = create_field_decl (get_identifier ("OBJ"), field_type, rec_type,
				 NULL_TREE, bitsize_zero_node, c < 0, 0);

      finish_record_type (rec_type, field, 1, false);

      expr = unchecked_convert (rec_type, expr, notrunc_p);
      expr = build_component_ref (expr, field, false);
      expr = fold_build1 (NOP_EXPR, type, expr);
    }

  /* Similarly if we are converting from an integral type whose precision is
     not equal to its size, first copy into a field of the given precision
     and unchecked convert the record type.

     The same considerations as above apply if the target type is an aggregate
     type with reverse storage order and we also proceed similarly.  */
  else if ((INTEGRAL_TYPE_P (etype)
	    && TYPE_RM_SIZE (etype)
	    && ((c = tree_int_cst_compare (TYPE_RM_SIZE (etype),
					   TYPE_SIZE (etype))) < 0
		|| reverse))
	   || (SCALAR_FLOAT_TYPE_P (etype) && reverse))
    {
      tree rec_type = make_node (RECORD_TYPE);
      vec<constructor_elt, va_gc> *v;
      vec_alloc (v, 1);
      tree field_type, field;

      TYPE_REVERSE_STORAGE_ORDER (rec_type) = reverse;

      if (c < 0)
	{
	  const unsigned HOST_WIDE_INT prec
	    = TREE_INT_CST_LOW (TYPE_RM_SIZE (etype));
	  if (type_unsigned_for_rm (etype))
	    field_type = make_unsigned_type (prec);
	  else
	    field_type = make_signed_type (prec);
	  SET_TYPE_RM_SIZE (field_type, TYPE_RM_SIZE (etype));
	}
      else
	field_type = etype;

      field = create_field_decl (get_identifier ("OBJ"), field_type, rec_type,
				 NULL_TREE, bitsize_zero_node, c < 0, 0);

      finish_record_type (rec_type, field, 1, false);

      expr = fold_build1 (NOP_EXPR, field_type, expr);
      CONSTRUCTOR_APPEND_ELT (v, field, expr);
      expr = gnat_build_constructor (rec_type, v);
      expr = unchecked_convert (type, expr, notrunc_p);
    }

  /* If we are converting between fixed-size types with different sizes, we
     need to pad to have the same size on both sides.

     ??? We cannot do it unconditionally because unchecked conversions are
     used liberally by the front-end to implement interface thunks:

       type ada__tags__addr_ptr is access system.address;
       S191s : constant ada__tags__addr_ptr := ada__tags__addr_ptr!(S190s);
       return p___size__4 (p__object!(S191s.all));

     so we need to skip dereferences.  */
  else if (!INDIRECT_REF_P (expr)
	   && TREE_CODE (expr) != STRING_CST
	   && !(AGGREGATE_TYPE_P (etype) && AGGREGATE_TYPE_P (type))
	   && ecode != UNCONSTRAINED_ARRAY_TYPE
	   && TREE_CONSTANT (TYPE_SIZE (etype))
	   && TREE_CONSTANT (TYPE_SIZE (type))
	   && (c = tree_int_cst_compare (TYPE_SIZE (etype), TYPE_SIZE (type))))
    {
      if (c < 0)
	{
	  expr = convert (maybe_pad_type (etype, TYPE_SIZE (type), 0, Empty,
					  false, false, true),
			  expr);
	  expr = unchecked_convert (type, expr, notrunc_p);
	}
      else
	{
	  tree rec_type = maybe_pad_type (type, TYPE_SIZE (etype), 0, Empty,
					  false, false, true);
	  expr = unchecked_convert (rec_type, expr, notrunc_p);
	  expr = build3 (COMPONENT_REF, type, expr, TYPE_FIELDS (rec_type),
			 NULL_TREE);
	}
    }

  /* Likewise if we are converting from a fixed-size type to a type with self-
     referential size.  We use the max size to do the padding in this case.  */
  else if (!INDIRECT_REF_P (expr)
	   && TREE_CODE (expr) != STRING_CST
	   && !(AGGREGATE_TYPE_P (etype) && AGGREGATE_TYPE_P (type))
	   && ecode != UNCONSTRAINED_ARRAY_TYPE
	   && TREE_CONSTANT (TYPE_SIZE (etype))
	   && CONTAINS_PLACEHOLDER_P (TYPE_SIZE (type)))
    {
      tree new_size = max_size (TYPE_SIZE (type), true);
      c = tree_int_cst_compare (TYPE_SIZE (etype), new_size);
      if (c < 0)
	{
	  expr = convert (maybe_pad_type (etype, new_size, 0, Empty,
					  false, false, true),
			  expr);
	  expr = unchecked_convert (type, expr, notrunc_p);
	}
      else
	{
	  tree rec_type = maybe_pad_type (type, TYPE_SIZE (etype), 0, Empty,
					  false, false, true);
	  expr = unchecked_convert (rec_type, expr, notrunc_p);
	  expr = build3 (COMPONENT_REF, type, expr, TYPE_FIELDS (rec_type),
			 NULL_TREE);
	}
    }

  /* We have a special case when we are converting between two unconstrained
     array types.  In that case, take the address, convert the fat pointer
     types, and dereference.  */
  else if (ecode == code && code == UNCONSTRAINED_ARRAY_TYPE)
    expr = build_unary_op (INDIRECT_REF, NULL_TREE,
			   build1 (VIEW_CONVERT_EXPR, TREE_TYPE (type),
				   build_unary_op (ADDR_EXPR, NULL_TREE,
						   expr)));

  /* Another special case is when we are converting to a vector type from its
     representative array type; this a regular conversion.  */
  else if (code == VECTOR_TYPE
	   && ecode == ARRAY_TYPE
	   && gnat_types_compatible_p (TYPE_REPRESENTATIVE_ARRAY (type),
				       etype))
    expr = convert (type, expr);

  /* And, if the array type is not the representative, we try to build an
     intermediate vector type of which the array type is the representative
     and to do the unchecked conversion between the vector types, in order
     to enable further simplifications in the middle-end.  */
  else if (code == VECTOR_TYPE
	   && ecode == ARRAY_TYPE
	   && (tem = build_vector_type_for_array (etype, NULL_TREE)))
    {
      expr = convert (tem, expr);
      return unchecked_convert (type, expr, notrunc_p);
    }

  /* If we are converting a CONSTRUCTOR to a more aligned aggregate type, bump
     the alignment of the CONSTRUCTOR to speed up the copy operation.  But do
     not do it for a conversion between original and packable version to avoid
     an infinite recursion.  */
  else if (TREE_CODE (expr) == CONSTRUCTOR
	   && AGGREGATE_TYPE_P (type)
	   && TYPE_NAME (type) != TYPE_NAME (etype)
	   && TYPE_ALIGN (etype) < TYPE_ALIGN (type))
    {
      expr = convert (maybe_pad_type (etype, NULL_TREE, TYPE_ALIGN (type),
				      Empty, false, false, true),
		      expr);
      return unchecked_convert (type, expr, notrunc_p);
    }

  /* If we are converting a CONSTRUCTOR to a larger aggregate type, bump the
     size of the CONSTRUCTOR to make sure there are enough allocated bytes.
     But do not do it for a conversion between original and packable version
     to avoid an infinite recursion.  */
  else if (TREE_CODE (expr) == CONSTRUCTOR
	   && AGGREGATE_TYPE_P (type)
	   && TYPE_NAME (type) != TYPE_NAME (etype)
	   && TREE_CONSTANT (TYPE_SIZE (type))
	   && (!TREE_CONSTANT (TYPE_SIZE (etype))
	       || tree_int_cst_lt (TYPE_SIZE (etype), TYPE_SIZE (type))))
    {
      expr = convert (maybe_pad_type (etype, TYPE_SIZE (type), 0,
				      Empty, false, false, true),
		      expr);
      return unchecked_convert (type, expr, notrunc_p);
    }

  /* If we are converting a string constant to a pointer to character, make
     sure that the string is not folded into an integer constant.  */
  else if (TREE_CODE (expr) == STRING_CST
	   && POINTER_TYPE_P (type)
	   && TYPE_STRING_FLAG (TREE_TYPE (type)))
    expr = build1 (VIEW_CONVERT_EXPR, type, expr);

  /* Otherwise, just build a VIEW_CONVERT_EXPR of the expression.  */
  else
    {
      expr = maybe_unconstrained_array (expr);
      etype = TREE_TYPE (expr);
      ecode = TREE_CODE (etype);
      if (can_fold_for_view_convert_p (expr))
	expr = fold_build1 (VIEW_CONVERT_EXPR, type, expr);
      else
	expr = build1 (VIEW_CONVERT_EXPR, type, expr);
    }

  /* If the result is a non-biased integral type whose precision is not equal
     to its size, sign- or zero-extend the result.  But we need not do this
     if the input is also an integral type and both are unsigned or both are
     signed and have the same precision.  */
  tree type_rm_size;
  if (!notrunc_p
      && !biased
      && INTEGRAL_TYPE_P (type)
      && (type_rm_size = TYPE_RM_SIZE (type))
      && tree_int_cst_compare (type_rm_size, TYPE_SIZE (type)) < 0
      && !(INTEGRAL_TYPE_P (etype)
	   && type_unsigned_for_rm (type) == type_unsigned_for_rm (etype)
	   && (type_unsigned_for_rm (type)
	       || tree_int_cst_compare (type_rm_size,
					TYPE_RM_SIZE (etype)
					? TYPE_RM_SIZE (etype)
					: TYPE_SIZE (etype)) == 0)))
    {
      if (integer_zerop (type_rm_size))
	expr = build_int_cst (type, 0);
      else
	{
	  tree base_type
	    = gnat_type_for_size (TREE_INT_CST_LOW (TYPE_SIZE (type)),
				  type_unsigned_for_rm (type));
	  tree shift_expr
	    = convert (base_type,
		       size_binop (MINUS_EXPR,
				   TYPE_SIZE (type), type_rm_size));
	  expr
	    = convert (type,
		       build_binary_op (RSHIFT_EXPR, base_type,
				        build_binary_op (LSHIFT_EXPR, base_type,
							 convert (base_type,
								  expr),
							 shift_expr),
				        shift_expr));
	}
    }

  /* An unchecked conversion should never raise Constraint_Error.  The code
     below assumes that GCC's conversion routines overflow the same way that
     the underlying hardware does.  This is probably true.  In the rare case
     when it is false, we can rely on the fact that such conversions are
     erroneous anyway.  */
  if (TREE_CODE (expr) == INTEGER_CST)
    TREE_OVERFLOW (expr) = 0;

  /* If the sizes of the types differ and this is an VIEW_CONVERT_EXPR,
     show no longer constant.  */
  if (TREE_CODE (expr) == VIEW_CONVERT_EXPR
      && !operand_equal_p (TYPE_SIZE_UNIT (type), TYPE_SIZE_UNIT (etype),
			   OEP_ONLY_CONST))
    TREE_CONSTANT (expr) = 0;

  return expr;
}

/* Return the appropriate GCC tree code for the specified GNAT_TYPE,
   the latter being a record type as predicated by Is_Record_Type.  */

enum tree_code
tree_code_for_record_type (Entity_Id gnat_type)
{
  Node_Id component_list, component;

  /* Return UNION_TYPE if it's an Unchecked_Union whose non-discriminant
     fields are all in the variant part.  Otherwise, return RECORD_TYPE.  */
  if (!Is_Unchecked_Union (gnat_type))
    return RECORD_TYPE;

  gnat_type = Implementation_Base_Type (gnat_type);
  component_list
    = Component_List (Type_Definition (Declaration_Node (gnat_type)));

  for (component = First_Non_Pragma (Component_Items (component_list));
       Present (component);
       component = Next_Non_Pragma (component))
    if (Ekind (Defining_Entity (component)) == E_Component)
      return RECORD_TYPE;

  return UNION_TYPE;
}

/* Return true if GNAT_TYPE is a "double" floating-point type, i.e. whose
   size is equal to 64 bits, or an array of such a type.  Set ALIGN_CLAUSE
   according to the presence of an alignment clause on the type or, if it
   is an array, on the component type.  */

bool
is_double_float_or_array (Entity_Id gnat_type, bool *align_clause)
{
  gnat_type = Underlying_Type (gnat_type);

  *align_clause = Present (Alignment_Clause (gnat_type));

  if (Is_Array_Type (gnat_type))
    {
      gnat_type = Underlying_Type (Component_Type (gnat_type));
      if (Present (Alignment_Clause (gnat_type)))
	*align_clause = true;
    }

  if (!Is_Floating_Point_Type (gnat_type))
    return false;

  if (UI_To_Int (Esize (gnat_type)) != 64)
    return false;

  return true;
}

/* Return true if GNAT_TYPE is a "double" or larger scalar type, i.e. whose
   size is greater or equal to 64 bits, or an array of such a type.  Set
   ALIGN_CLAUSE according to the presence of an alignment clause on the
   type or, if it is an array, on the component type.  */

bool
is_double_scalar_or_array (Entity_Id gnat_type, bool *align_clause)
{
  gnat_type = Underlying_Type (gnat_type);

  *align_clause = Present (Alignment_Clause (gnat_type));

  if (Is_Array_Type (gnat_type))
    {
      gnat_type = Underlying_Type (Component_Type (gnat_type));
      if (Present (Alignment_Clause (gnat_type)))
	*align_clause = true;
    }

  if (!Is_Scalar_Type (gnat_type))
    return false;

  if (UI_To_Int (Esize (gnat_type)) < 64)
    return false;

  return true;
}

/* Return true if GNU_TYPE is suitable as the type of a non-aliased
   component of an aggregate type.  */

bool
type_for_nonaliased_component_p (tree gnu_type)
{
  /* If the type is passed by reference, we may have pointers to the
     component so it cannot be made non-aliased. */
  if (must_pass_by_ref (gnu_type) || default_pass_by_ref (gnu_type))
    return false;

  /* We used to say that any component of aggregate type is aliased
     because the front-end may take 'Reference of it.  The front-end
     has been enhanced in the meantime so as to use a renaming instead
     in most cases, but the back-end can probably take the address of
     such a component too so we go for the conservative stance.

     For instance, we might need the address of any array type, even
     if normally passed by copy, to construct a fat pointer if the
     component is used as an actual for an unconstrained formal.

     Likewise for record types: even if a specific record subtype is
     passed by copy, the parent type might be passed by ref (e.g. if
     it's of variable size) and we might take the address of a child
     component to pass to a parent formal.  We have no way to check
     for such conditions here.  */
  if (AGGREGATE_TYPE_P (gnu_type))
    return false;

  return true;
}

/* Return true if TYPE is a smaller form of ORIG_TYPE.  */

bool
smaller_form_type_p (tree type, tree orig_type)
{
  tree size, osize;

  /* We're not interested in variants here.  */
  if (TYPE_MAIN_VARIANT (type) == TYPE_MAIN_VARIANT (orig_type))
    return false;

  /* Like a variant, a packable version keeps the original TYPE_NAME.  */
  if (TYPE_NAME (type) != TYPE_NAME (orig_type))
    return false;

  size = TYPE_SIZE (type);
  osize = TYPE_SIZE (orig_type);

  if (!(TREE_CODE (size) == INTEGER_CST && TREE_CODE (osize) == INTEGER_CST))
    return false;

  return tree_int_cst_lt (size, osize) != 0;
}

/* Return whether EXPR, which is the renamed object in an object renaming
   declaration, can be materialized as a reference (with a REFERENCE_TYPE).
   This should be synchronized with Exp_Dbug.Debug_Renaming_Declaration.  */

bool
can_materialize_object_renaming_p (Node_Id expr)
{
  while (true)
    {
      expr = Original_Node (expr);

      switch (Nkind (expr))
	{
	case N_Identifier:
	case N_Expanded_Name:
	  if (!Present (Renamed_Object (Entity (expr))))
	    return true;
	  expr = Renamed_Object (Entity (expr));
	  break;

	case N_Selected_Component:
	  {
	    if (Is_Packed (Underlying_Type (Etype (Prefix (expr)))))
	      return false;

	    const Uint bitpos
	      = Normalized_First_Bit (Entity (Selector_Name (expr)));
	    if (bitpos != UI_No_Uint && bitpos != Uint_0)
	      return false;

	    expr = Prefix (expr);
	    break;
	  }

	case N_Indexed_Component:
	case N_Slice:
	  {
	    const Entity_Id t = Underlying_Type (Etype (Prefix (expr)));

	    if (Is_Array_Type (t) && Present (Packed_Array_Impl_Type (t)))
	      return false;

	    expr = Prefix (expr);
	    break;
	  }

	case N_Explicit_Dereference:
	  expr = Prefix (expr);
	  break;

	default:
	  return true;
	};
    }
}

/* Perform final processing on global declarations.  */

static GTY (()) tree dummy_global;

void
gnat_write_global_declarations (void)
{
  unsigned int i;
  tree iter;

  /* If we have declared types as used at the global level, insert them in
     the global hash table.  We use a dummy variable for this purpose, but
     we need to build it unconditionally to avoid -fcompare-debug issues.  */
  if (first_global_object_name)
    {
      struct varpool_node *node;
      char *label;

      ASM_FORMAT_PRIVATE_NAME (label, first_global_object_name, ULONG_MAX);
      dummy_global
	= build_decl (BUILTINS_LOCATION, VAR_DECL, get_identifier (label),
		      void_type_node);
      DECL_HARD_REGISTER (dummy_global) = 1;
      TREE_STATIC (dummy_global) = 1;
      node = varpool_node::get_create (dummy_global);
      node->definition = 1;
      node->force_output = 1;

      if (types_used_by_cur_var_decl)
	while (!types_used_by_cur_var_decl->is_empty ())
	  {
	    tree t = types_used_by_cur_var_decl->pop ();
	    types_used_by_var_decl_insert (t, dummy_global);
	  }
    }

  /* First output the integral global variables, so that they can be referenced
     as bounds by the global dynamic types.  Skip external variables, unless we
     really need to emit debug info for them:, e.g. imported variables.  */
  FOR_EACH_VEC_SAFE_ELT (global_decls, i, iter)
    if (TREE_CODE (iter) == VAR_DECL
	&& INTEGRAL_TYPE_P (TREE_TYPE (iter))
	&& (!DECL_EXTERNAL (iter) || !DECL_IGNORED_P (iter)))
      rest_of_decl_compilation (iter, true, 0);

  /* Now output debug information for the global type declarations.  This
     ensures that global types whose compilation hasn't been finalized yet,
     for example pointers to Taft amendment types, have their compilation
     finalized in the right context.  */
  FOR_EACH_VEC_SAFE_ELT (global_decls, i, iter)
    if (TREE_CODE (iter) == TYPE_DECL && !DECL_IGNORED_P (iter))
      debug_hooks->type_decl (iter, false);

  /* Then output the other global variables.  We need to do that after the
     information for global types is emitted so that they are finalized.  */
  FOR_EACH_VEC_SAFE_ELT (global_decls, i, iter)
    if (TREE_CODE (iter) == VAR_DECL
	&& !INTEGRAL_TYPE_P (TREE_TYPE (iter))
	&& (!DECL_EXTERNAL (iter) || !DECL_IGNORED_P (iter)))
      rest_of_decl_compilation (iter, true, 0);

  /* Output debug information for the global constants.  */
  FOR_EACH_VEC_SAFE_ELT (global_decls, i, iter)
    if (TREE_CODE (iter) == CONST_DECL && !DECL_IGNORED_P (iter))
      debug_hooks->early_global_decl (iter);

  /* Output it for the imported functions.  */
  FOR_EACH_VEC_SAFE_ELT (global_decls, i, iter)
    if (TREE_CODE (iter) == FUNCTION_DECL
	&& DECL_EXTERNAL (iter)
	&& DECL_INITIAL (iter) == NULL
	&& !DECL_IGNORED_P (iter)
	&& DECL_FUNCTION_IS_DEF (iter))
      debug_hooks->early_global_decl (iter);

  /* Output it for the imported modules/declarations.  In GNAT, these are only
     materializing subprogram.  */
  FOR_EACH_VEC_SAFE_ELT (global_decls, i, iter)
   if (TREE_CODE (iter) == IMPORTED_DECL && !DECL_IGNORED_P (iter))
     debug_hooks->imported_module_or_decl (iter, DECL_NAME (iter),
					   DECL_CONTEXT (iter), false, false);
}

/* ************************************************************************
 * *                           GCC builtins support                       *
 * ************************************************************************ */

/* The general scheme is fairly simple:

   For each builtin function/type to be declared, gnat_install_builtins calls
   internal facilities which eventually get to gnat_pushdecl, which in turn
   tracks the so declared builtin function decls in the 'builtin_decls' global
   datastructure. When an Intrinsic subprogram declaration is processed, we
   search this global datastructure to retrieve the associated BUILT_IN DECL
   node.  */

/* Search the chain of currently available builtin declarations for a node
   corresponding to function NAME (an IDENTIFIER_NODE).  Return the first node
   found, if any, or NULL_TREE otherwise.  */
tree
builtin_decl_for (tree name)
{
  unsigned i;
  tree decl;

  FOR_EACH_VEC_SAFE_ELT (builtin_decls, i, decl)
    if (DECL_NAME (decl) == name)
      return decl;

  return NULL_TREE;
}

/* The code below eventually exposes gnat_install_builtins, which declares
   the builtin types and functions we might need, either internally or as
   user accessible facilities.

   ??? This is a first implementation shot, still in rough shape.  It is
   heavily inspired from the "C" family implementation, with chunks copied
   verbatim from there.

   Two obvious improvement candidates are:
   o Use a more efficient name/decl mapping scheme
   o Devise a middle-end infrastructure to avoid having to copy
     pieces between front-ends.  */

/* ----------------------------------------------------------------------- *
 *                         BUILTIN ELEMENTARY TYPES                        *
 * ----------------------------------------------------------------------- */

/* Standard data types to be used in builtin argument declarations.  */

enum c_tree_index
{
    CTI_SIGNED_SIZE_TYPE, /* For format checking only.  */
    CTI_STRING_TYPE,
    CTI_CONST_STRING_TYPE,

    CTI_MAX
};

static tree c_global_trees[CTI_MAX];

#define signed_size_type_node	c_global_trees[CTI_SIGNED_SIZE_TYPE]
#define string_type_node	c_global_trees[CTI_STRING_TYPE]
#define const_string_type_node	c_global_trees[CTI_CONST_STRING_TYPE]

/* ??? In addition some attribute handlers, we currently don't support a
   (small) number of builtin-types, which in turns inhibits support for a
   number of builtin functions.  */
#define wint_type_node    void_type_node
#define intmax_type_node  void_type_node
#define uintmax_type_node void_type_node

/* Used to help initialize the builtin-types.def table.  When a type of
   the correct size doesn't exist, use error_mark_node instead of NULL.
   The later results in segfaults even when a decl using the type doesn't
   get invoked.  */

static tree
builtin_type_for_size (int size, bool unsignedp)
{
  tree type = gnat_type_for_size (size, unsignedp);
  return type ? type : error_mark_node;
}

/* Build/push the elementary type decls that builtin functions/types
   will need.  */

static void
install_builtin_elementary_types (void)
{
  signed_size_type_node = gnat_signed_type_for (size_type_node);
  pid_type_node = integer_type_node;

  string_type_node = build_pointer_type (char_type_node);
  const_string_type_node
    = build_pointer_type (build_qualified_type
			  (char_type_node, TYPE_QUAL_CONST));
}

/* ----------------------------------------------------------------------- *
 *                          BUILTIN FUNCTION TYPES                         *
 * ----------------------------------------------------------------------- */

/* Now, builtin function types per se.  */

enum c_builtin_type
{
#define DEF_PRIMITIVE_TYPE(NAME, VALUE) NAME,
#define DEF_FUNCTION_TYPE_0(NAME, RETURN) NAME,
#define DEF_FUNCTION_TYPE_1(NAME, RETURN, ARG1) NAME,
#define DEF_FUNCTION_TYPE_2(NAME, RETURN, ARG1, ARG2) NAME,
#define DEF_FUNCTION_TYPE_3(NAME, RETURN, ARG1, ARG2, ARG3) NAME,
#define DEF_FUNCTION_TYPE_4(NAME, RETURN, ARG1, ARG2, ARG3, ARG4) NAME,
#define DEF_FUNCTION_TYPE_5(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5) NAME,
#define DEF_FUNCTION_TYPE_6(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6) NAME,
#define DEF_FUNCTION_TYPE_7(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7) NAME,
#define DEF_FUNCTION_TYPE_8(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7, ARG8) NAME,
#define DEF_FUNCTION_TYPE_9(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7, ARG8, ARG9) NAME,
#define DEF_FUNCTION_TYPE_10(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			     ARG6, ARG7, ARG8, ARG9, ARG10) NAME,
#define DEF_FUNCTION_TYPE_11(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			     ARG6, ARG7, ARG8, ARG9, ARG10, ARG11) NAME,
#define DEF_FUNCTION_TYPE_VAR_0(NAME, RETURN) NAME,
#define DEF_FUNCTION_TYPE_VAR_1(NAME, RETURN, ARG1) NAME,
#define DEF_FUNCTION_TYPE_VAR_2(NAME, RETURN, ARG1, ARG2) NAME,
#define DEF_FUNCTION_TYPE_VAR_3(NAME, RETURN, ARG1, ARG2, ARG3) NAME,
#define DEF_FUNCTION_TYPE_VAR_4(NAME, RETURN, ARG1, ARG2, ARG3, ARG4) NAME,
#define DEF_FUNCTION_TYPE_VAR_5(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5) \
				NAME,
#define DEF_FUNCTION_TYPE_VAR_6(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
				ARG6) NAME,
#define DEF_FUNCTION_TYPE_VAR_7(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
				ARG6, ARG7) NAME,
#define DEF_POINTER_TYPE(NAME, TYPE) NAME,
#include "builtin-types.def"
#include "ada-builtin-types.def"
#undef DEF_PRIMITIVE_TYPE
#undef DEF_FUNCTION_TYPE_0
#undef DEF_FUNCTION_TYPE_1
#undef DEF_FUNCTION_TYPE_2
#undef DEF_FUNCTION_TYPE_3
#undef DEF_FUNCTION_TYPE_4
#undef DEF_FUNCTION_TYPE_5
#undef DEF_FUNCTION_TYPE_6
#undef DEF_FUNCTION_TYPE_7
#undef DEF_FUNCTION_TYPE_8
#undef DEF_FUNCTION_TYPE_9
#undef DEF_FUNCTION_TYPE_10
#undef DEF_FUNCTION_TYPE_11
#undef DEF_FUNCTION_TYPE_VAR_0
#undef DEF_FUNCTION_TYPE_VAR_1
#undef DEF_FUNCTION_TYPE_VAR_2
#undef DEF_FUNCTION_TYPE_VAR_3
#undef DEF_FUNCTION_TYPE_VAR_4
#undef DEF_FUNCTION_TYPE_VAR_5
#undef DEF_FUNCTION_TYPE_VAR_6
#undef DEF_FUNCTION_TYPE_VAR_7
#undef DEF_POINTER_TYPE
  BT_LAST
};

typedef enum c_builtin_type builtin_type;

/* A temporary array used in communication with def_fn_type.  */
static GTY(()) tree builtin_types[(int) BT_LAST + 1];

/* A helper function for install_builtin_types.  Build function type
   for DEF with return type RET and N arguments.  If VAR is true, then the
   function should be variadic after those N arguments.

   Takes special care not to ICE if any of the types involved are
   error_mark_node, which indicates that said type is not in fact available
   (see builtin_type_for_size).  In which case the function type as a whole
   should be error_mark_node.  */

static void
def_fn_type (builtin_type def, builtin_type ret, bool var, int n, ...)
{
  tree t;
  tree *args = XALLOCAVEC (tree, n);
  va_list list;
  int i;

  va_start (list, n);
  for (i = 0; i < n; ++i)
    {
      builtin_type a = (builtin_type) va_arg (list, int);
      t = builtin_types[a];
      if (t == error_mark_node)
	goto egress;
      args[i] = t;
    }

  t = builtin_types[ret];
  if (t == error_mark_node)
    goto egress;
  if (var)
    t = build_varargs_function_type_array (t, n, args);
  else
    t = build_function_type_array (t, n, args);

 egress:
  builtin_types[def] = t;
  va_end (list);
}

/* Build the builtin function types and install them in the builtin_types
   array for later use in builtin function decls.  */

static void
install_builtin_function_types (void)
{
  tree va_list_ref_type_node;
  tree va_list_arg_type_node;

  if (TREE_CODE (va_list_type_node) == ARRAY_TYPE)
    {
      va_list_arg_type_node = va_list_ref_type_node =
	build_pointer_type (TREE_TYPE (va_list_type_node));
    }
  else
    {
      va_list_arg_type_node = va_list_type_node;
      va_list_ref_type_node = build_reference_type (va_list_type_node);
    }

#define DEF_PRIMITIVE_TYPE(ENUM, VALUE) \
  builtin_types[ENUM] = VALUE;
#define DEF_FUNCTION_TYPE_0(ENUM, RETURN) \
  def_fn_type (ENUM, RETURN, 0, 0);
#define DEF_FUNCTION_TYPE_1(ENUM, RETURN, ARG1) \
  def_fn_type (ENUM, RETURN, 0, 1, ARG1);
#define DEF_FUNCTION_TYPE_2(ENUM, RETURN, ARG1, ARG2) \
  def_fn_type (ENUM, RETURN, 0, 2, ARG1, ARG2);
#define DEF_FUNCTION_TYPE_3(ENUM, RETURN, ARG1, ARG2, ARG3) \
  def_fn_type (ENUM, RETURN, 0, 3, ARG1, ARG2, ARG3);
#define DEF_FUNCTION_TYPE_4(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4) \
  def_fn_type (ENUM, RETURN, 0, 4, ARG1, ARG2, ARG3, ARG4);
#define DEF_FUNCTION_TYPE_5(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5)	\
  def_fn_type (ENUM, RETURN, 0, 5, ARG1, ARG2, ARG3, ARG4, ARG5);
#define DEF_FUNCTION_TYPE_6(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6)					\
  def_fn_type (ENUM, RETURN, 0, 6, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);
#define DEF_FUNCTION_TYPE_7(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7)					\
  def_fn_type (ENUM, RETURN, 0, 7, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7);
#define DEF_FUNCTION_TYPE_8(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7, ARG8)				\
  def_fn_type (ENUM, RETURN, 0, 8, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6,	\
	       ARG7, ARG8);
#define DEF_FUNCTION_TYPE_9(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7, ARG8, ARG9)			\
  def_fn_type (ENUM, RETURN, 0, 9, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6,	\
	       ARG7, ARG8, ARG9);
#define DEF_FUNCTION_TYPE_10(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5,\
			     ARG6, ARG7, ARG8, ARG9, ARG10)		\
  def_fn_type (ENUM, RETURN, 0, 10, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6,	\
	       ARG7, ARG8, ARG9, ARG10);
#define DEF_FUNCTION_TYPE_11(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5,\
			     ARG6, ARG7, ARG8, ARG9, ARG10, ARG11)	\
  def_fn_type (ENUM, RETURN, 0, 11, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6,	\
	       ARG7, ARG8, ARG9, ARG10, ARG11);
#define DEF_FUNCTION_TYPE_VAR_0(ENUM, RETURN) \
  def_fn_type (ENUM, RETURN, 1, 0);
#define DEF_FUNCTION_TYPE_VAR_1(ENUM, RETURN, ARG1) \
  def_fn_type (ENUM, RETURN, 1, 1, ARG1);
#define DEF_FUNCTION_TYPE_VAR_2(ENUM, RETURN, ARG1, ARG2) \
  def_fn_type (ENUM, RETURN, 1, 2, ARG1, ARG2);
#define DEF_FUNCTION_TYPE_VAR_3(ENUM, RETURN, ARG1, ARG2, ARG3) \
  def_fn_type (ENUM, RETURN, 1, 3, ARG1, ARG2, ARG3);
#define DEF_FUNCTION_TYPE_VAR_4(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4) \
  def_fn_type (ENUM, RETURN, 1, 4, ARG1, ARG2, ARG3, ARG4);
#define DEF_FUNCTION_TYPE_VAR_5(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5) \
  def_fn_type (ENUM, RETURN, 1, 5, ARG1, ARG2, ARG3, ARG4, ARG5);
#define DEF_FUNCTION_TYPE_VAR_6(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
				ARG6)				\
  def_fn_type (ENUM, RETURN, 1, 6, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);
#define DEF_FUNCTION_TYPE_VAR_7(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
				ARG6, ARG7)				\
  def_fn_type (ENUM, RETURN, 1, 7, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7);
#define DEF_POINTER_TYPE(ENUM, TYPE) \
  builtin_types[(int) ENUM] = build_pointer_type (builtin_types[(int) TYPE]);

#include "builtin-types.def"
#include "ada-builtin-types.def"

#undef DEF_PRIMITIVE_TYPE
#undef DEF_FUNCTION_TYPE_0
#undef DEF_FUNCTION_TYPE_1
#undef DEF_FUNCTION_TYPE_2
#undef DEF_FUNCTION_TYPE_3
#undef DEF_FUNCTION_TYPE_4
#undef DEF_FUNCTION_TYPE_5
#undef DEF_FUNCTION_TYPE_6
#undef DEF_FUNCTION_TYPE_7
#undef DEF_FUNCTION_TYPE_8
#undef DEF_FUNCTION_TYPE_9
#undef DEF_FUNCTION_TYPE_10
#undef DEF_FUNCTION_TYPE_11
#undef DEF_FUNCTION_TYPE_VAR_0
#undef DEF_FUNCTION_TYPE_VAR_1
#undef DEF_FUNCTION_TYPE_VAR_2
#undef DEF_FUNCTION_TYPE_VAR_3
#undef DEF_FUNCTION_TYPE_VAR_4
#undef DEF_FUNCTION_TYPE_VAR_5
#undef DEF_FUNCTION_TYPE_VAR_6
#undef DEF_FUNCTION_TYPE_VAR_7
#undef DEF_POINTER_TYPE
  builtin_types[(int) BT_LAST] = NULL_TREE;
}

/* ----------------------------------------------------------------------- *
 *                            BUILTIN ATTRIBUTES                           *
 * ----------------------------------------------------------------------- */

enum built_in_attribute
{
#define DEF_ATTR_NULL_TREE(ENUM) ENUM,
#define DEF_ATTR_INT(ENUM, VALUE) ENUM,
#define DEF_ATTR_STRING(ENUM, VALUE) ENUM,
#define DEF_ATTR_IDENT(ENUM, STRING) ENUM,
#define DEF_ATTR_TREE_LIST(ENUM, PURPOSE, VALUE, CHAIN) ENUM,
#include "builtin-attrs.def"
#undef DEF_ATTR_NULL_TREE
#undef DEF_ATTR_INT
#undef DEF_ATTR_STRING
#undef DEF_ATTR_IDENT
#undef DEF_ATTR_TREE_LIST
  ATTR_LAST
};

static GTY(()) tree built_in_attributes[(int) ATTR_LAST];

static void
install_builtin_attributes (void)
{
  /* Fill in the built_in_attributes array.  */
#define DEF_ATTR_NULL_TREE(ENUM)				\
  built_in_attributes[(int) ENUM] = NULL_TREE;
#define DEF_ATTR_INT(ENUM, VALUE)				\
  built_in_attributes[(int) ENUM] = build_int_cst (NULL_TREE, VALUE);
#define DEF_ATTR_STRING(ENUM, VALUE)				\
  built_in_attributes[(int) ENUM] = build_string (strlen (VALUE), VALUE);
#define DEF_ATTR_IDENT(ENUM, STRING)				\
  built_in_attributes[(int) ENUM] = get_identifier (STRING);
#define DEF_ATTR_TREE_LIST(ENUM, PURPOSE, VALUE, CHAIN)	\
  built_in_attributes[(int) ENUM]			\
    = tree_cons (built_in_attributes[(int) PURPOSE],	\
		 built_in_attributes[(int) VALUE],	\
		 built_in_attributes[(int) CHAIN]);
#include "builtin-attrs.def"
#undef DEF_ATTR_NULL_TREE
#undef DEF_ATTR_INT
#undef DEF_ATTR_STRING
#undef DEF_ATTR_IDENT
#undef DEF_ATTR_TREE_LIST
}

/* Handle a "const" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_const_attribute (tree *node, tree ARG_UNUSED (name),
			tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    TREE_READONLY (*node) = 1;
  else
    *no_add_attrs = true;

  return NULL_TREE;
}

/* Handle a "nothrow" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_nothrow_attribute (tree *node, tree ARG_UNUSED (name),
			  tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			  bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    TREE_NOTHROW (*node) = 1;
  else
    *no_add_attrs = true;

  return NULL_TREE;
}

/* Handle a "expected_throw" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_expected_throw_attribute (tree *node, tree ARG_UNUSED (name),
				 tree ARG_UNUSED (args), int ARG_UNUSED (flags),
				 bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    /* No flag to set here.  */;
  else
    *no_add_attrs = true;

  return NULL_TREE;
}

/* Handle a "pure" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_pure_attribute (tree *node, tree name, tree ARG_UNUSED (args),
		       int ARG_UNUSED (flags), bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    DECL_PURE_P (*node) = 1;
  /* TODO: support types.  */
  else
    {
      warning (OPT_Wattributes, "%qs attribute ignored",
	       IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "no vops" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_novops_attribute (tree *node, tree ARG_UNUSED (name),
			 tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			 bool *ARG_UNUSED (no_add_attrs))
{
  gcc_assert (TREE_CODE (*node) == FUNCTION_DECL);
  DECL_IS_NOVOPS (*node) = 1;
  return NULL_TREE;
}

/* Helper for nonnull attribute handling; fetch the operand number
   from the attribute argument list.  */

static bool
get_nonnull_operand (tree arg_num_expr, unsigned HOST_WIDE_INT *valp)
{
  /* Verify the arg number is a constant.  */
  if (!tree_fits_uhwi_p (arg_num_expr))
    return false;

  *valp = TREE_INT_CST_LOW (arg_num_expr);
  return true;
}

/* Handle the "nonnull" attribute.  */
static tree
handle_nonnull_attribute (tree *node, tree ARG_UNUSED (name),
			  tree args, int ARG_UNUSED (flags),
			  bool *no_add_attrs)
{
  tree type = *node;
  unsigned HOST_WIDE_INT attr_arg_num;

  /* If no arguments are specified, all pointer arguments should be
     non-null.  Verify a full prototype is given so that the arguments
     will have the correct types when we actually check them later.
     Avoid diagnosing type-generic built-ins since those have no
     prototype.  */
  if (!args)
    {
      if (!prototype_p (type)
	  && (!TYPE_ATTRIBUTES (type)
	      || !lookup_attribute ("type generic", TYPE_ATTRIBUTES (type))))
	{
	  error ("%qs attribute without arguments on a non-prototype",
		 "nonnull");
	  *no_add_attrs = true;
	}
      return NULL_TREE;
    }

  /* Argument list specified.  Verify that each argument number references
     a pointer argument.  */
  for (attr_arg_num = 1; args; args = TREE_CHAIN (args))
    {
      unsigned HOST_WIDE_INT arg_num = 0, ck_num;

      if (!get_nonnull_operand (TREE_VALUE (args), &arg_num))
	{
	  error ("%qs argument has invalid operand number (argument %lu)",
		 "nonnull", (unsigned long) attr_arg_num);
	  *no_add_attrs = true;
	  return NULL_TREE;
	}

      if (prototype_p (type))
	{
	  function_args_iterator iter;
	  tree argument;

	  function_args_iter_init (&iter, type);
	  for (ck_num = 1; ; ck_num++, function_args_iter_next (&iter))
	    {
	      argument = function_args_iter_cond (&iter);
	      if (!argument || ck_num == arg_num)
		break;
	    }

	  if (!argument
	      || TREE_CODE (argument) == VOID_TYPE)
	    {
	      error ("%qs argument with out-of-range operand number "
		     "(argument %lu, operand %lu)", "nonnull",
		     (unsigned long) attr_arg_num, (unsigned long) arg_num);
	      *no_add_attrs = true;
	      return NULL_TREE;
	    }

	  if (TREE_CODE (argument) != POINTER_TYPE)
	    {
	      error ("%qs argument references non-pointer operand "
		     "(argument %lu, operand %lu)", "nonnull",
		   (unsigned long) attr_arg_num, (unsigned long) arg_num);
	      *no_add_attrs = true;
	      return NULL_TREE;
	    }
	}
    }

  return NULL_TREE;
}

/* Handle a "sentinel" attribute.  */

static tree
handle_sentinel_attribute (tree *node, tree name, tree args,
			   int ARG_UNUSED (flags), bool *no_add_attrs)
{
  if (!prototype_p (*node))
    {
      warning (OPT_Wattributes,
	       "%qs attribute requires prototypes with named arguments",
	       IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }
  else
    {
      if (!stdarg_p (*node))
        {
	  warning (OPT_Wattributes,
		   "%qs attribute only applies to variadic functions",
		   IDENTIFIER_POINTER (name));
	  *no_add_attrs = true;
	}
    }

  if (args)
    {
      tree position = TREE_VALUE (args);

      if (TREE_CODE (position) != INTEGER_CST)
        {
	  warning (0, "requested position is not an integer constant");
	  *no_add_attrs = true;
	}
      else
        {
	  if (tree_int_cst_lt (position, integer_zero_node))
	    {
	      warning (0, "requested position is less than zero");
	      *no_add_attrs = true;
	    }
	}
    }

  return NULL_TREE;
}

/* Handle a "noreturn" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_noreturn_attribute (tree *node, tree name, tree ARG_UNUSED (args),
			   int ARG_UNUSED (flags), bool *no_add_attrs)
{
  tree type = TREE_TYPE (*node);

  /* See FIXME comment in c_common_attribute_table.  */
  if (TREE_CODE (*node) == FUNCTION_DECL)
    TREE_THIS_VOLATILE (*node) = 1;
  else if (TREE_CODE (type) == POINTER_TYPE
	   && TREE_CODE (TREE_TYPE (type)) == FUNCTION_TYPE)
    TREE_TYPE (*node)
      = build_pointer_type
	(change_qualified_type (TREE_TYPE (type), TYPE_QUAL_VOLATILE));
  else
    {
      warning (OPT_Wattributes, "%qs attribute ignored",
	       IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "stack_protect" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_stack_protect_attribute (tree *node, tree name, tree, int,
				bool *no_add_attrs)
{
  if (TREE_CODE (*node) != FUNCTION_DECL)
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "no_stack_protector" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_no_stack_protector_attribute (tree *node, tree name, tree, int,
				   bool *no_add_attrs)
{
  if (TREE_CODE (*node) != FUNCTION_DECL)
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "strub" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_strub_attribute (tree *node, tree name,
			tree args,
			int ARG_UNUSED (flags), bool *no_add_attrs)
{
  bool enable = true;

  if (args && FUNCTION_POINTER_TYPE_P (*node))
    *node = TREE_TYPE (*node);

  if (args && FUNC_OR_METHOD_TYPE_P (*node))
    {
      switch (strub_validate_fn_attr_parm (TREE_VALUE (args)))
	{
	case 1:
	case 2:
	  enable = true;
	  break;

	case 0:
	  warning (OPT_Wattributes,
		   "%qE attribute ignored because of argument %qE",
		   name, TREE_VALUE (args));
	  *no_add_attrs = true;
	  enable = false;
	  break;

	case -1:
	case -2:
	  enable = false;
	  break;

	default:
	  gcc_unreachable ();
	}

      args = TREE_CHAIN (args);
    }

  if (args)
    {
      warning (OPT_Wattributes,
	       "ignoring attribute %qE because of excess arguments"
	       " starting at %qE",
	       name, TREE_VALUE (args));
      *no_add_attrs = true;
      enable = false;
    }

  /* Warn about unmet expectations that the strub attribute works like a
     qualifier.  ??? Could/should we extend it to the element/field types
     here?  */
  if (TREE_CODE (*node) == ARRAY_TYPE
      || VECTOR_TYPE_P (*node)
      || TREE_CODE (*node) == COMPLEX_TYPE)
    warning (OPT_Wattributes,
	     "attribute %qE does not apply to elements"
	     " of non-scalar type %qT",
	     name, *node);
  else if (RECORD_OR_UNION_TYPE_P (*node))
    warning (OPT_Wattributes,
	     "attribute %qE does not apply to fields"
	     " of aggregate type %qT",
	     name, *node);

  /* If we see a strub-enabling attribute, and we're at the default setting,
     implicitly or explicitly, note that the attribute was seen, so that we can
     reduce the compile-time overhead to nearly zero when the strub feature is
     not used.  */
  if (enable && flag_strub < -2)
    flag_strub += 2;

  return NULL_TREE;
}

/* Handle a "noinline" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_noinline_attribute (tree *node, tree name,
			   tree ARG_UNUSED (args),
			   int ARG_UNUSED (flags), bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    DECL_UNINLINABLE (*node) = 1;
  else
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "noclone" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_noclone_attribute (tree *node, tree name,
			  tree ARG_UNUSED (args),
			  int ARG_UNUSED (flags), bool *no_add_attrs)
{
  if (TREE_CODE (*node) != FUNCTION_DECL)
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "no_icf" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_noicf_attribute (tree *node, tree name,
			tree ARG_UNUSED (args),
			int ARG_UNUSED (flags), bool *no_add_attrs)
{
  if (TREE_CODE (*node) != FUNCTION_DECL)
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "noipa" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_noipa_attribute (tree *node, tree name, tree, int, bool *no_add_attrs)
{
  if (TREE_CODE (*node) != FUNCTION_DECL)
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "leaf" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_leaf_attribute (tree *node, tree name, tree ARG_UNUSED (args),
		       int ARG_UNUSED (flags), bool *no_add_attrs)
{
  if (TREE_CODE (*node) != FUNCTION_DECL)
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }
  if (!TREE_PUBLIC (*node))
    {
      warning (OPT_Wattributes, "%qE attribute has no effect", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "always_inline" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_always_inline_attribute (tree *node, tree name, tree ARG_UNUSED (args),
				int ARG_UNUSED (flags), bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    {
      /* Set the attribute and mark it for disregarding inline limits.  */
      DECL_DISREGARD_INLINE_LIMITS (*node) = 1;
    }
  else
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "malloc" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_malloc_attribute (tree *node, tree name, tree ARG_UNUSED (args),
			 int ARG_UNUSED (flags), bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL
      && POINTER_TYPE_P (TREE_TYPE (TREE_TYPE (*node))))
    DECL_IS_MALLOC (*node) = 1;
  else
    {
      warning (OPT_Wattributes, "%qs attribute ignored",
	       IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Fake handler for attributes we don't properly support.  */

tree
fake_attribute_handler (tree * ARG_UNUSED (node),
			tree ARG_UNUSED (name),
			tree ARG_UNUSED (args),
			int  ARG_UNUSED (flags),
			bool * ARG_UNUSED (no_add_attrs))
{
  return NULL_TREE;
}

/* Handle a "type_generic" attribute.  */

static tree
handle_type_generic_attribute (tree *node, tree ARG_UNUSED (name),
			       tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			       bool * ARG_UNUSED (no_add_attrs))
{
  /* Ensure we have a function type.  */
  gcc_assert (TREE_CODE (*node) == FUNCTION_TYPE);

  /* Ensure we have a variadic function.  */
  gcc_assert (!prototype_p (*node) || stdarg_p (*node));

  return NULL_TREE;
}

/* Handle a "flatten" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_flatten_attribute (tree *node, tree name,
			  tree args ATTRIBUTE_UNUSED,
			  int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    /* Do nothing else, just set the attribute.  We'll get at
       it later with lookup_attribute.  */
    ;
  else
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "used" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_used_attribute (tree *pnode, tree name, tree ARG_UNUSED (args),
		       int ARG_UNUSED (flags), bool *no_add_attrs)
{
  tree node = *pnode;

  if (TREE_CODE (node) == FUNCTION_DECL
      || (VAR_P (node) && TREE_STATIC (node))
      || (TREE_CODE (node) == TYPE_DECL))
    {
      TREE_USED (node) = 1;
      DECL_PRESERVE_P (node) = 1;
      if (VAR_P (node))
	DECL_READ_P (node) = 1;
    }
  else
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle an "uninitialized" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_uninitialized_attribute (tree *node, tree name, tree ARG_UNUSED (args),
				int ARG_UNUSED (flags), bool *no_add_attrs)
{
  tree decl = *node;
  if (!VAR_P (decl))
    {
      warning (OPT_Wattributes, "%qE attribute ignored because %qD "
	       "is not a variable", name, decl);
      *no_add_attrs = true;
    }
  else if (TREE_STATIC (decl) || DECL_EXTERNAL (decl))
    {
      warning (OPT_Wattributes, "%qE attribute ignored because %qD "
	       "is not a local variable", name, decl);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "cold" and attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_cold_attribute (tree *node, tree name, tree ARG_UNUSED (args),
		       int ARG_UNUSED (flags), bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL
      || TREE_CODE (*node) == LABEL_DECL)
    {
      /* Attribute cold processing is done later with lookup_attribute.  */
    }
  else
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "hot" and attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_hot_attribute (tree *node, tree name, tree ARG_UNUSED (args),
		      int ARG_UNUSED (flags), bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL
      || TREE_CODE (*node) == LABEL_DECL)
    {
      /* Attribute hot processing is done later with lookup_attribute.  */
    }
  else
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "simd" attribute.  */

static tree
handle_simd_attribute (tree *node, tree name, tree args, int, bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    {
      tree t = get_identifier ("omp declare simd");
      tree attr = NULL_TREE;
      if (args)
	{
	  tree id = TREE_VALUE (args);

	  if (TREE_CODE (id) != STRING_CST)
	    {
	      error ("attribute %qE argument not a string", name);
	      *no_add_attrs = true;
	      return NULL_TREE;
	    }

	  if (strcmp (TREE_STRING_POINTER (id), "notinbranch") == 0)
	    attr = build_omp_clause (DECL_SOURCE_LOCATION (*node),
				     OMP_CLAUSE_NOTINBRANCH);
	  else if (strcmp (TREE_STRING_POINTER (id), "inbranch") == 0)
	    attr = build_omp_clause (DECL_SOURCE_LOCATION (*node),
				     OMP_CLAUSE_INBRANCH);
	  else
	    {
	      error ("only %<inbranch%> and %<notinbranch%> flags are "
		     "allowed for %<__simd__%> attribute");
	      *no_add_attrs = true;
	      return NULL_TREE;
	    }
	}

      DECL_ATTRIBUTES (*node)
	= tree_cons (t, build_tree_list (NULL_TREE, attr),
		     DECL_ATTRIBUTES (*node));
    }
  else
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "target" attribute.  */

static tree
handle_target_attribute (tree *node, tree name, tree args, int flags,
			 bool *no_add_attrs)
{
  /* Ensure we have a function type.  */
  if (TREE_CODE (*node) != FUNCTION_DECL)
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }
  else if (!targetm.target_option.valid_attribute_p (*node, name, args, flags))
    *no_add_attrs = true;

  /* Check that there's no empty string in values of the attribute.  */
  for (tree t = args; t != NULL_TREE; t = TREE_CHAIN (t))
    {
      tree value = TREE_VALUE (t);
      if (TREE_CODE (value) == STRING_CST
	  && TREE_STRING_LENGTH (value) == 1
	  && TREE_STRING_POINTER (value)[0] == '\0')
	{
	  warning (OPT_Wattributes, "empty string in attribute %<target%>");
	  *no_add_attrs = true;
	}
    }

  return NULL_TREE;
}

/* Handle a "target_clones" attribute.  */

static tree
handle_target_clones_attribute (tree *node, tree name, tree ARG_UNUSED (args),
			  int ARG_UNUSED (flags), bool *no_add_attrs)
{
  /* Ensure we have a function type.  */
  if (TREE_CODE (*node) == FUNCTION_DECL)
    /* Do not inline functions with multiple clone targets.  */
    DECL_UNINLINABLE (*node) = 1;
  else
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }
  return NULL_TREE;
}

/* Handle a "vector_size" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_vector_size_attribute (tree *node, tree name, tree args,
			      int ARG_UNUSED (flags), bool *no_add_attrs)
{
  tree type = *node;
  tree vector_type;

  *no_add_attrs = true;

  /* We need to provide for vector pointers, vector arrays, and
     functions returning vectors.  For example:

       __attribute__((vector_size(16))) short *foo;

     In this case, the mode is SI, but the type being modified is
     HI, so we need to look further.  */
  while (POINTER_TYPE_P (type)
	 || TREE_CODE (type) == FUNCTION_TYPE
	 || TREE_CODE (type) == ARRAY_TYPE)
    type = TREE_TYPE (type);

  vector_type = build_vector_type_for_size (type, TREE_VALUE (args), name);
  if (!vector_type)
    return NULL_TREE;

  /* Build back pointers if needed.  */
  *node = reconstruct_complex_type (*node, vector_type);

  return NULL_TREE;
}

/* Handle a "vector_type" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_vector_type_attribute (tree *node, tree name, tree ARG_UNUSED (args),
			      int ARG_UNUSED (flags), bool *no_add_attrs)
{
  tree type = *node;
  tree vector_type;

  *no_add_attrs = true;

  if (TREE_CODE (type) != ARRAY_TYPE)
    {
      error ("attribute %qs applies to array types only",
	     IDENTIFIER_POINTER (name));
      return NULL_TREE;
    }

  vector_type = build_vector_type_for_array (type, name);
  if (!vector_type)
    return NULL_TREE;

  TYPE_REPRESENTATIVE_ARRAY (vector_type) = type;
  *node = vector_type;

  return NULL_TREE;
}

/* Handle a "zero_call_used_regs" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_zero_call_used_regs_attribute (tree *node, tree name, tree args,
				      int ARG_UNUSED (flags),
				      bool *no_add_attrs)
{
  tree decl = *node;
  tree id = TREE_VALUE (args);

  if (TREE_CODE (decl) != FUNCTION_DECL)
    {
      error_at (DECL_SOURCE_LOCATION (decl),
		"%qE attribute applies only to functions", name);
      *no_add_attrs = true;
      return NULL_TREE;
    }

  /* pragma Machine_Attribute turns string arguments into identifiers.
     Reverse it.  */
  if (TREE_CODE (id) == IDENTIFIER_NODE)
    id = TREE_VALUE (args) = build_string
      (IDENTIFIER_LENGTH (id), IDENTIFIER_POINTER (id));

  if (TREE_CODE (id) != STRING_CST)
    {
      error_at (DECL_SOURCE_LOCATION (decl),
		"%qE argument not a string", name);
      *no_add_attrs = true;
      return NULL_TREE;
    }

  bool found = false;
  for (unsigned int i = 0; zero_call_used_regs_opts[i].name != NULL; ++i)
    if (strcmp (TREE_STRING_POINTER (id),
		zero_call_used_regs_opts[i].name) == 0)
      {
	found = true;
	break;
      }

  if (!found)
    {
      error_at (DECL_SOURCE_LOCATION (decl),
		"unrecognized %qE attribute argument %qs",
		name, TREE_STRING_POINTER (id));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* ----------------------------------------------------------------------- *
 *                              BUILTIN FUNCTIONS                          *
 * ----------------------------------------------------------------------- */

/* Worker for DEF_BUILTIN.  Possibly define a builtin function with one or two
   names.  Does not declare a non-__builtin_ function if flag_no_builtin, or
   if nonansi_p and flag_no_nonansi_builtin.  */

static void
def_builtin_1 (enum built_in_function fncode,
	       const char *name,
	       enum built_in_class fnclass,
	       tree fntype, tree libtype,
	       bool both_p, bool fallback_p,
	       bool nonansi_p ATTRIBUTE_UNUSED,
	       tree fnattrs, bool implicit_p)
{
  tree decl;
  const char *libname;

  /* Preserve an already installed decl.  It most likely was setup in advance
     (e.g. as part of the internal builtins) for specific reasons.  */
  if (builtin_decl_explicit (fncode))
    return;

  if (fntype == error_mark_node)
    return;

  gcc_assert ((!both_p && !fallback_p)
	      || startswith (name, "__builtin_"));

  libname = name + strlen ("__builtin_");
  decl = add_builtin_function (name, fntype, fncode, fnclass,
			       (fallback_p ? libname : NULL),
			       fnattrs);
  if (both_p)
    /* ??? This is normally further controlled by command-line options
       like -fno-builtin, but we don't have them for Ada.  */
    add_builtin_function (libname, libtype, fncode, fnclass,
			  NULL, fnattrs);

  set_builtin_decl (fncode, decl, implicit_p);
}

static int flag_isoc94 = 0;
static int flag_isoc99 = 0;
static int flag_isoc11 = 0;
static int flag_isoc23 = 0;
static int flag_isoc2y = 0;

/* Install what the common builtins.def offers plus our local additions.

   Note that ada-builtins.def is included first so that locally redefined
   built-in functions take precedence over the commonly defined ones.  */

static void
install_builtin_functions (void)
{
#define DEF_BUILTIN(ENUM, NAME, CLASS, TYPE, LIBTYPE, BOTH_P, FALLBACK_P, \
		    NONANSI_P, ATTRS, IMPLICIT, COND)			\
  if (NAME && COND)							\
    def_builtin_1 (ENUM, NAME, CLASS,                                   \
                   builtin_types[(int) TYPE],                           \
                   builtin_types[(int) LIBTYPE],                        \
                   BOTH_P, FALLBACK_P, NONANSI_P,                       \
                   built_in_attributes[(int) ATTRS], IMPLICIT);
#define DEF_ADA_BUILTIN(ENUM, NAME, TYPE, ATTRS)		\
  DEF_BUILTIN (ENUM, "__builtin_" NAME, BUILT_IN_FRONTEND, TYPE, BT_LAST, \
	       false, false, false, ATTRS, true, true)
#include "ada-builtins.def"
#include "builtins.def"
}

/* ----------------------------------------------------------------------- *
 *                              BUILTIN FUNCTIONS                          *
 * ----------------------------------------------------------------------- */

/* Install the builtin functions we might need.  */

void
gnat_install_builtins (void)
{
  install_builtin_elementary_types ();
  install_builtin_function_types ();
  install_builtin_attributes ();

  /* Install builtins used by generic middle-end pieces first.  Some of these
     know about internal specificities and control attributes accordingly, for
     instance __builtin_alloca vs no-throw and -fstack-check.  We will ignore
     the generic definition from builtins.def.  */
  build_common_builtin_nodes ();

  /* Now, install the target specific builtins, such as the AltiVec family on
     ppc, and the common set as exposed by builtins.def.  */
  targetm.init_builtins ();
  install_builtin_functions ();
}

#include "gt-ada-utils.h"
#include "gtype-ada.h"
