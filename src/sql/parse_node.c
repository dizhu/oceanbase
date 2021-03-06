#include "parse_node.h"
#include <stdio.h>
#include "parse_malloc.h"

extern const char* get_type_name(int type);

int count_child(ParseNode* root);
void merge_child(ParseNode* node, ParseNode* source_tree, int* index);

void destroy_tree(ParseNode* root)
{
  int i;
  if(root == 0)
    return;
  for(i = 0; i < root->num_child_; ++i)
  {
    destroy_tree(root->children_[i]);
  }
  if(root->str_value_ != NULL)
  {
    parse_free((char*)root->str_value_);
  }
  if(root->num_child_)
  {
    parse_free(root->children_);
  }
  parse_free(root);
}

void print_tree(ParseNode* root, int level)
{
  int i;
  for(i = 0; i < level; ++i) 
    printf("    ");
  if(root == 0)
  {
    printf("NULL\n");
    return;
  }

  printf("%s", get_type_name(root->type_));
  switch(root->type_)
  {
  case T_BOOL:
    if (root->value_ == 1)
      printf(" : TRUE\n");
    else if (root->value_ == 0)
      printf(" : FALSE\n");
    else
      printf(" : UNKNOWN\n");
    break;
  case T_BINARY:
    printf(" : \\x");
    for(i = 0; i < root->value_; ++i)
    {
      printf("%02x", (unsigned char)root->str_value_[i]);
    }
    printf("\n");
    break;
  case T_INT:
  case T_FLOAT:
  case T_DOUBLE:
  case T_DECIMAL:
  case T_STRING:
  case T_DATE:
  case T_HINT:
  case T_IDENT:
    printf(" : %s\n", root->str_value_);
    break;
  case T_FUN_SYS:
    printf(" : %s\n", root->str_value_);
    break;
  default:
    printf("\n");
    break;
  }
  for(i = 0; i < root->num_child_; ++i)
  {
    if (root->children_[i])
      print_tree(root->children_[i], level+1);
  }
}

ParseNode* new_node(void *malloc_pool, ObItemType type, int num)
{
  //ParseNode* node = (ParseNode*)malloc(sizeof(ParseNode));
  ParseNode* node = (ParseNode*)parse_malloc(sizeof(ParseNode), malloc_pool);
  memset(node, 0 , sizeof(ParseNode));

  node->type_ = type;
  node->num_child_ = num;
  if(num > 0)
  {
    int64_t alloc_size = sizeof(ParseNode*) * num ;
    //node->children_ = (ParseNode**)malloc(alloc_size);
    node->children_ = (ParseNode**)parse_malloc(alloc_size, malloc_pool);
    memset(node->children_, 0, alloc_size);
  }
  else
  {
    node->children_ = 0;
  }
  return node;
}

int count_child(ParseNode* root)
{
  if(root == 0) return 0;

  int count = 0;
  if(root->type_ != T_LINK_NODE)
  {
    return 1;
  }
  int i;
  for(i = 0; i < root->num_child_; ++i)
  {
    count += count_child(root->children_[i]);  
  }
  return count;
}

void merge_child(ParseNode* node, ParseNode* source_tree, int* index)
{
  // assert(node);
  if (node == NULL || source_tree == NULL)
    return;

  if(source_tree->type_ == T_LINK_NODE)
  {
    int i;
    for(i = 0; i < source_tree->num_child_; ++i)
    {
      merge_child(node, source_tree->children_[i], index);  
      source_tree->children_[i] = 0;
    }
    destroy_tree(source_tree);
  }
  else
  {
    assert(*index >= 0 && *index < node->num_child_);
    node->children_[*index] = source_tree;
    ++(*index);
  }
}

ParseNode* merge_tree(void *malloc_pool, ObItemType node_tag, ParseNode* source_tree)
{
  int index = 0;
  int num = 0;
  ParseNode* node;
  if(source_tree == NULL)
    return NULL;
  num = count_child(source_tree);
  node = new_node(malloc_pool, node_tag, num);
  merge_child(node, source_tree, &index);
  assert(index == num);
  return node;
}

ParseNode* new_non_terminal_node(void *malloc_pool, ObItemType node_tag, int num, ...)
{
  assert(num>0);
  va_list va;
  int i;
  
  ParseNode* node = new_node(malloc_pool, node_tag, num);
  va_start(va, num);
  for( i = 0; i < num; ++i)
  {
    node->children_[i] = va_arg(va, ParseNode*);
  }
  va_end(va);
  return node;
}

char* copy_expr_string(ParseResult* p, int expr_start, int expr_end)
{
  char *expr_string = NULL;
  if (p->input_sql_ != NULL && expr_start >= 0 && expr_end >= 0 && expr_end >= expr_start)
  {
    int len = expr_end - expr_start + 1;
    expr_string = (char*)parse_malloc(len + 1, p->malloc_pool_);
    if (expr_string != NULL)
    {
      memmove(expr_string, p->input_sql_ + expr_start - 1, len);
      expr_string[len] = '\0';
    }
  }
  return expr_string;
}


