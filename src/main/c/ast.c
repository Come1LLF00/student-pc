#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

extern int is_verbose;

static int nr_spaces = 0;

const char* ot_symbol[] = {
  [ OT_UN_NOT ]      = "not",
  [ OT_BIN_PLUS ]    = "+",
  [ OT_BIN_MUL ]     = "*",
  [ OT_BIN_DIV ]     = "/",
  [ OT_BIN_POW ]     = "^",
  [ OT_BIN_LESS ]    = "<",
  [ OT_BIN_GREATER ] = ">",
  [ OT_BIN_EQUALS ]  = "==",
  [ OT_MINUS ]       = "-",
  [ OT_ASSIGN ]      = "="
};

#define ANT_PROGRAM_FMT    "[ child: 0x%lx ]>"
#define ANT_STMTS_LIST_FMT "[ current: 0x%lx, next: 0x%lx ]>"
#define ANT_REPEAT_FMT     "[ test: 0x%lx, body: 0x%lx ]>"
#define ANT_BRANCH_FMT     "[ test: 0x%lx, then: 0x%lx, else: 0x%lx ]>"
#define ANT_UNEXPR_FMT     "[ oper: %s, argument: 0x%lx ]>"
#define ANT_EXPR_FMT       "[ left: 0x%lx, oper: %s, right: 0x%lx ]>"
#define ANT_CONST_FMT      "[ value: %d ]>"
#define ANT_IDENT_FMT      "[ name: %s ]>"
#define ANT_COMMON_FMT     "<%s:0x%lx"

static struct ast_node* make_node( enum ast_node_type type ) {
  struct ast_node* node = ( struct ast_node* ) malloc( sizeof( struct ast_node ) );

  if ( node == NULL ) {
    fprintf( stderr, "%s: can't allocate memory for new node\n", __func__ );
    exit(1);
  }

  node->type = type;
  node->is_visited = false;
  is_verbose && fprintf( stderr, "%s: " ANT_COMMON_FMT, __func__, ant_names[ type ], ( long unsigned int ) node );
  return node;
}

struct ast_node* make_ident( const char* name ) {
  struct ast_node* ident = make_node( ANT_IDENT );
  strncpy( ident->as_ident.name, name, MAXIMUM_IDENTIFIER_LENGTH );
  
  is_verbose && fprintf( stderr, ANT_IDENT_FMT "\n", name );
  return ident;
}

struct ast_node* make_const( int value ) {
  struct ast_node* constant = make_node( ANT_CONST );

  constant->as_const.value = value;
  is_verbose && fprintf( stderr, ANT_CONST_FMT "\n", value );
  return constant;
}

struct ast_node* make_expr( struct ast_node* left, enum operation_type oper, struct ast_node* right ) {
  struct ast_node* expr = make_node( ANT_EXPR );

  expr->as_expr.left  = left;
  expr->as_expr.oper  = oper;
  expr->as_expr.right = right;
  is_verbose && fprintf( stderr, ANT_EXPR_FMT "\n", ( long unsigned int ) left, ot_symbol[ oper ], ( long unsigned int ) right );
  return expr;
}

struct ast_node* make_unexpr( enum operation_type oper, struct ast_node* arg ) {
  struct ast_node* unexpr = make_node( ANT_UNEXPR );

  unexpr->as_unexpr.oper     = oper;
  unexpr->as_unexpr.argument = arg;
  is_verbose && fprintf( stderr, ANT_UNEXPR_FMT "\n", ot_symbol[ oper ], ( long unsigned int ) arg );
  return unexpr;
}

struct ast_node* make_branch( struct ast_node* test, struct ast_node* consequent, struct ast_node* alternate ) {
  struct ast_node* branch = make_node( ANT_BRANCH );

  branch->as_branch.test       = test;
  branch->as_branch.consequent = consequent;
  branch->as_branch.alternate  = alternate;
  is_verbose && fprintf( stderr, ANT_BRANCH_FMT "\n", ( long unsigned int ) test, ( long unsigned int ) consequent, ( long unsigned int ) alternate );
  return branch;
}

struct ast_node* make_repeat( struct ast_node* test, struct ast_node* body ) {
  struct ast_node* repeat = make_node( ANT_REPEAT );

  repeat->as_repeat.test = test;
  repeat->as_repeat.body = body;
  is_verbose && fprintf( stderr, ANT_REPEAT_FMT "\n", ( long unsigned int ) test, ( long unsigned int ) body );
  return repeat;
}

struct ast_node* make_stmts_list( struct ast_node* head, struct ast_node* next ) {
  struct ast_node* list = make_node( ANT_STMTS_LIST );

  list->as_stmts_list.current = head;
  list->as_stmts_list.next    = next;
  is_verbose && fprintf( stderr, ANT_STMTS_LIST_FMT "\n",
    ( long unsigned int ) list->as_stmts_list.current,
    ( long unsigned int ) list->as_stmts_list.next );
  return list;
}

void stmts_list_insert( struct ast_node** head_p, struct ast_node* first ) {
  if ( *head_p ) {
    struct ast_node* const oldstart = make_stmts_list( (*head_p)->as_stmts_list.current, (*head_p)->as_stmts_list.next );
    free( *head_p );
    (*head_p) = make_stmts_list( first, oldstart );
  } else *head_p = make_stmts_list( first, NULL );
}

struct ast_node* make_program( struct ast_node* child ) {
  struct ast_node* program = make_node( ANT_PROGRAM );

  program->as_program.child = child;
  is_verbose && fprintf( stderr, ANT_PROGRAM_FMT "\n", ( long unsigned int ) child );
  return program;
}

static void dump_cb( struct ast_node* node )   { return;       }

static void free_node( struct ast_node* node ) {
  is_verbose && fprintf( stderr, "free: freeing %lx\n", ( long unsigned int ) node );
  free( node );
}

static void preprint_ast( struct ast_node* node ) {
  nr_spaces += 2;
  for ( size_t i = 0; i < nr_spaces; ++i )
    fprintf( stderr, " " );

  fprintf( stderr, ANT_COMMON_FMT, ant_names[ node->type ], ( long unsigned int ) node );
  switch ( node->type ) {
    case ANT_PROGRAM:
      fprintf( stderr, ANT_PROGRAM_FMT "\n",
        ( long unsigned int ) node->as_program.child );
      break;
    case ANT_STMTS_LIST:
      fprintf( stderr, ANT_STMTS_LIST_FMT "\n",
        ( long unsigned int ) node->as_stmts_list.current,
        ( long unsigned int ) node->as_stmts_list.next );
      break;
    case ANT_REPEAT:
      fprintf( stderr, ANT_REPEAT_FMT "\n",
        ( long unsigned int ) node->as_repeat.test,
        ( long unsigned int ) node->as_repeat.body );
      break;
    case ANT_BRANCH:
      fprintf( stderr, ANT_BRANCH_FMT "\n",
        ( long unsigned int ) node->as_branch.test,
        ( long unsigned int ) node->as_branch.consequent,
        ( long unsigned int ) node->as_branch.alternate );
      break;
    case ANT_UNEXPR:
      fprintf( stderr, ANT_UNEXPR_FMT "\n",
        ot_symbol[ node->as_unexpr.oper ],
        ( long unsigned int ) node->as_unexpr.argument );
      break;
    case ANT_EXPR:
      fprintf( stderr, ANT_EXPR_FMT "\n",
        ( long unsigned int ) node->as_expr.left,
        ot_symbol[ node->as_expr.oper ],
        ( long unsigned int ) node->as_expr.right );
      break;
    case ANT_CONST: fprintf( stderr, ANT_CONST_FMT "\n", node->as_const.value ); break;
    case ANT_IDENT: fprintf( stderr, ANT_IDENT_FMT "\n", node->as_ident.name ); break;
    default: fprintf( stderr, "unknown-node>\n" ); break;
  }
}

static void postprint_ast( struct ast_node* node ) {
  nr_spaces -= 2;
}

void dfs_traverse( struct ast_node* node, process_cb preproccess_cb, process_cb postprocess_cb ) {
  if ( node == NULL ) return;
  node->is_visited = true;

  preproccess_cb( node );
  
  switch ( node->type ) {
    case ANT_PROGRAM: dfs_traverse( node->as_program.child, preproccess_cb, postprocess_cb ); break;
    case ANT_STMTS_LIST: {
      struct ast_node* iter = node;
      while ( iter != NULL ) {
        dfs_traverse( iter->as_stmts_list.current, preproccess_cb, postprocess_cb );
        struct ast_node* temp = iter;
        iter = iter->as_stmts_list.next;
        postprocess_cb( temp ); // for good freeing
      }
      return;
    }
    case ANT_REPEAT: {
      dfs_traverse( node->as_repeat.body, preproccess_cb, postprocess_cb );
      dfs_traverse( node->as_repeat.test, preproccess_cb, postprocess_cb );
      break;
    }
    case ANT_BRANCH: {
      dfs_traverse( node->as_branch.test, preproccess_cb, postprocess_cb );
      dfs_traverse( node->as_branch.consequent, preproccess_cb, postprocess_cb );
      dfs_traverse( node->as_branch.alternate, preproccess_cb, postprocess_cb );
      break;
    }
    case ANT_UNEXPR: dfs_traverse( node->as_unexpr.argument, preproccess_cb, postprocess_cb ); break;
    case ANT_EXPR: {
      dfs_traverse( node->as_expr.left, preproccess_cb, postprocess_cb );
      dfs_traverse( node->as_expr.right, preproccess_cb, postprocess_cb );
      break;
    }
    default: break;
  }

  postprocess_cb( node );
  return;
}

void free_ast( struct ast_node* root ) {
  dfs_traverse( root, dump_cb, free_node );
}

int print_ast( struct ast_node* root ) {
  dfs_traverse( root, preprint_ast, postprint_ast );
  return 1;
}