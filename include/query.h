#include <stdbool.h>
#include <stdlib.h>
#include <regex.h>

char delim[] = " ";

typedef struct node 
{
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
   SINGLE,
   ARITHMETIC
} value_t;

typedef struct arithmetic_condition
{
   char *operand1;
   char *operand2;
   char *operator;
} arithmetic_condition_t;

typedef struct simple_condition
{
   void *value;
   value_t value_type;
} simple_condition_t;

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

typedef struct condition
{
   char *operand1;
   operator_t operator;
   void *operand2;
   struct condition *next_condition;
   bool not;
} condition_t;

typedef struct query
{
   type_t type;
   char *table_name;
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
   char operand_print[512];
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
         case SINGLE:
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
         case SINGLE:
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
      simple_condition_t *equality_condition = (simple_condition_t *) cond->operand2;
      switch (equality_condition->value_type)
      {
         case SINGLE:
         {
            strcpy(operand_print, (char *)(equality_condition->value));
            break;
         }
         case ARITHMETIC: 
         {
            arithmetic_condition_t *athm_cond = (arithmetic_condition_t *)(equality_condition->value);
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

   if (cond->not) 
   {
      printf("\"%s NOT %s %s\", ", cond->operand1, operator_to_print, operand_print);
   }
   else 
   {
      printf("\"%s %s %s\", ", cond->operand1, operator_to_print, operand_print);
   }
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
      printf("\"%s\", ", current_field->val);
      current_field = current_field->next;
   }
   printf("\n");

   printf("The tablename is \"%s\" \n", query->table_name);

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