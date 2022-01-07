#include <stdbool.h>
#include <stdlib.h>
#include <regex.h>
#include "ht.h"

char delim[] = " ";

ht *sql_tables_columns;

typedef struct node 
{
   char *key;
   char *val;
   struct node *next;
} list_node_t;

typedef enum type 
{
   SELECT
} type_t;

typedef enum operator 
{
   EQ,
   NE,
   GT,
   LT,
   GTE,
   LTE,
   LIKE,
   IN,
   BETWEEN,
} operator_t;

typedef enum value_type 
{
   CONSTANT,
   ARITHMETIC,
   COLUMN_FIELD
} value_t;

typedef struct arithmetic_condition
{
   char *operand1;
   char *operand2;
   char *operator;
} arithmetic_condition_t;

typedef struct comparison_condition
{
   void *value;
   value_t value_type;
   char *table;
} comparison_condition_t;

typedef struct like_condition
{
   char *ex;
   regex_t regex;
} like_condition_t;

typedef struct in_condition
{
   list_node_t *match_ptr;
} in_condition_t;

typedef struct between_condition
{
   void *min_value;
   value_t min_value_type;
   void *max_value;
   value_t max_value_type;
} between_condition_t;

typedef struct field_operand
{
   char *name;
   char *table;
} field_operand_t;

typedef struct condition
{
   field_operand_t *operand1;
   operator_t operator;
   void *operand2;
   struct condition *next_condition;
   bool not;
} condition_t;

typedef struct table_name
{
   char *name;
   char alt_name[256];
   struct table_name *next_table;
} table_name_t;

typedef struct query
{
   type_t type;
   table_name_t *table_name_ptr;
   list_node_t *field_ptr;
   condition_t *condition_ptr;
   condition_t *and_condition_ptr;
   condition_t *or_condition_ptr;
} query_t;

char* check_operator(operator_t op) 
{
   switch (op)
   {
   case EQ:
      return "=";
   case GT:
      return ">";
   case LT:
      return "<";
   case GTE:
      return ">=";
   case LTE:
      return "<=";
   case NE:
      return "!=";
   case LIKE:
      return "LIKE";
   case BETWEEN:
      return "BETWEEN";
   case IN:
      return "IN";
   default:
      break;
   }

   return NULL;
}

void print_where_condition(condition_t *cond) 
{
   char *operator_to_print = check_operator(cond->operator);
   char *operand_print;
   if (strcmp(operator_to_print, "LIKE") == 0) 
   {
      strcpy(operand_print, ((like_condition_t *) cond->operand2)->ex);
   }
   else if (strcmp(operator_to_print, "BETWEEN") == 0)
   {
      strcpy(operand_print, "(");
      between_condition_t *between_condition = (between_condition_t *) cond->operand2;
      switch (between_condition->min_value_type)
      {
         case CONSTANT:
         {
            strcat(operand_print, (char *)(between_condition->min_value));
            break;
         }
         case ARITHMETIC:
         {
            arithmetic_condition_t *athm_cond = (arithmetic_condition_t *)(between_condition->min_value);
            strcat(operand_print, athm_cond->operand1);
            strcat(operand_print, " ");
            strcat(operand_print, athm_cond->operator);
            strcat(operand_print, " ");
            strcat(operand_print, athm_cond->operand2);
            break;
         }
         default:
         {
            break;
         }
      }
      strcat(operand_print, " AND ");
      switch (between_condition->max_value_type)
      {
         case CONSTANT:
         {
            strcat(operand_print, (char *)(between_condition->max_value));
            break;
         }
         case ARITHMETIC:
         {
            arithmetic_condition_t *athm_cond = (arithmetic_condition_t *)(between_condition->max_value);
            strcat(operand_print, athm_cond->operand1);
            strcat(operand_print, " ");
            strcat(operand_print, athm_cond->operator);
            strcat(operand_print, " ");
            strcat(operand_print, athm_cond->operand2);
            break;
         }
         default:
         {
            break;
         }
      }
      strcat(operand_print, ")");
   }
   else if (strcmp(operator_to_print, "IN") == 0) 
   {
      strcpy(operand_print, "(");
      list_node_t *cur_node = ((in_condition_t *) cond->operand2)->match_ptr;
      while (cur_node != NULL)
      {
         strcat(operand_print, cur_node->val);
         cur_node = cur_node->next;

         if (cur_node != NULL) 
         {
            strcat(operand_print, ", ");
         }
      }
      strcat(operand_print, ")");
   }
   else {
      comparison_condition_t *comparison_condition = (comparison_condition_t *) cond->operand2;
      switch (comparison_condition->value_type)
      {
         case CONSTANT:
         {
            strcpy(operand_print, (char *)(comparison_condition->value));
            break;
         }
         case COLUMN_FIELD:
         {
            strcpy(operand_print, comparison_condition->table);
            strcat(operand_print, "_");
            strcat(operand_print, comparison_condition->value);
            break;
         }
         case ARITHMETIC: 
         {
            arithmetic_condition_t *athm_cond = (arithmetic_condition_t *)(comparison_condition->value);
            strcpy(operand_print, athm_cond->operand1);
            strcat(operand_print, " ");
            strcat(operand_print, athm_cond->operator);
            strcat(operand_print, " ");
            strcat(operand_print, athm_cond->operand2);
            break;
         }
         default:
         {
            break;
         }
      }
   }

   char *not = cond->not ? "NOT" : " \b\b";
   char *table = cond->operand1->table == NULL ? "" : cond->operand1->table;
   char table_delim = cond->operand1->table == NULL ? '\0' : '_';

   printf("\"%s%c%s %s %s %s\", ", table, table_delim, cond->operand1->name, not, operator_to_print, operand_print);
}

void print_query_object(query_t *query) 
{
   if (query->type == SELECT) 
   {
      printf("The query type is \"SELECT\" \n");
   }

   list_node_t *current_field = query->field_ptr;
   printf("The select fields are ");
   while (current_field != NULL) 
   {
      char *table = current_field->key == NULL ? "" : current_field->key;
      char table_delim = current_field->key == NULL ? '\0' : '_';
      printf("\"%s%c%s\", ", table, table_delim, current_field->val);
      current_field = current_field->next;
   }
   printf("\n");

   table_name_t *current_table_name = query->table_name_ptr;
   printf("The tablename are ");
   while (current_table_name != NULL) {
      printf("\"%s (%s)\", ", current_table_name->name, current_table_name->alt_name);
      current_table_name = current_table_name->next_table;
   }
   printf("\n");

   condition_t *current_condition = query->condition_ptr;
   printf("The condition is ");

   if (current_condition != NULL) 
   {
      print_where_condition(current_condition);
   }
   printf("\n");

   current_condition = query->and_condition_ptr;
   printf("The AND conditions are ");
   while (current_condition != NULL) 
   {   
      print_where_condition(current_condition);
      current_condition = current_condition->next_condition;
   }
   printf("\n");

   current_condition = query->or_condition_ptr;
   printf("The OR conditions are ");
   while (current_condition != NULL) 
   {
      print_where_condition(current_condition);
      current_condition = current_condition->next_condition;
   }
   printf("\n");
}