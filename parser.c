#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "ht.c"
#include "hashset.c"
#include "hashset_itr.c"

typedef enum state
{
   stepType,
   stepSelectField,
   stepSelectFrom,
   stepSelectComma,
   stepSelectFromTable,
   stepWhere,
   stepWhereField,
   stepWhereOperator,
   stepWhereNot,
   stepWhereComparison,
   stepWhereIn,
   stepWhereLike,
   stepWhereBetween,
   stepWhereComparisonValue,
   stepWhereLikeValue,
   stepWhereInValue,
   stepWhereBetweenValue,
   stepWhereValueType,
   stepWhereContinue,
   stepWhereAnd,
   stepWhereOr,
} state_t;

void str_replace(char *target, const char *needle, const char *replacement)
{
   char buffer[1024] = { 0 };
   char *insert_point = &buffer[0];
   const char *tmp = target;
   size_t needle_len = strlen(needle);
   size_t repl_len = strlen(replacement);

   while (1) 
   {
      const char *p = strstr(tmp, needle);

      // walked past last occurrence of needle; copy remaining part
      if (p == NULL) 
      {
         strcpy(insert_point, tmp);
         break;
      }

      // copy part before needle
      memcpy(insert_point, tmp, p - tmp);
      insert_point += p - tmp;

      // copy replacement string
      memcpy(insert_point, replacement, repl_len);
      insert_point += repl_len;

      // adjust pointers, move on
      tmp = p + needle_len;
   }

   // write altered string back to target
   strcpy(target, buffer);
}

void parse_string(char *p, char *str, char *correspond_table) {
    char *c;
    char t[256];

    int i=0;
    for (c=p; *c!='\0'; c++) {
        if (*c == '_') {
            t[i] = '\0';
            strcpy(correspond_table, t);
            t[0] = 0;
            i=0;
        }
        else {
            t[i] = *c;
            i++;
        }
    }

    t[i] = '\0';
    strcpy(str, t);
}

void add_str_to_set(hashset_t set, char *str)
{
   bool has_value = false;
   hashset_itr_t iter = hashset_iterator(set);
   
   while(hashset_iterator_has_next(iter))
   {
      if (strcmp((char *) hashset_iterator_value(iter), str) == 0)
      {
         has_value = true;
         break;
      }
      hashset_iterator_next(iter);
   }

   if (!has_value)
   {
      hashset_add(set, str);
   }
}

query_t* parse(char *inputQuerry) {
   char* ptr = strtok(inputQuerry, delim);

   query_t *query = (query_t *) calloc(1, sizeof(query_t));
	state_t step = stepType;

   table_name_t *current_table_name = NULL;

	list_node_t *current_field_ptr = NULL;

	condition_t *current_condition_ptr = NULL;
   condition_t *current_and_condition_ptr = NULL;
   condition_t *current_or_condition_ptr = NULL;

   comparison_condition_t *current_comparison_condition = NULL;
   like_condition_t *curent_like_condition = NULL;
   between_condition_t *current_between_condition = NULL;
   in_condition_t *current_in_condition = NULL;
   list_node_t *current_in_value_ptr = NULL;

   while (ptr != NULL)
   {
      switch (step)
      {
         case stepType: 
         {
            if (strcmp(ptr, "SELECT") == 0)
            {
               query->type = SELECT;
               step = stepSelectField;

               query->field_ptr = (list_node_t *) calloc(1, sizeof(list_node_t));
               current_field_ptr = query->field_ptr;
            }

            ptr = strtok(NULL, delim);
            break;
         }
         case stepSelectField: 
         {
            // current_field_ptr->val = ptr;
            char *field = calloc(1, sizeof(char *));
            char *correspond_table = calloc(1, sizeof(char *));
            parse_string(ptr, field, correspond_table);

            current_field_ptr->val = field;
            if (strcmp(correspond_table, "") != 0)
            {
               current_field_ptr->key = correspond_table;

               hashset_t table_columns = (hashset_t) ht_get(sql_tables_columns, correspond_table);
               if (table_columns == NULL) {
                  table_columns = hashset_create();
                  hashset_add(table_columns, "");
                  ht_set(sql_tables_columns, correspond_table, table_columns);
               }
               add_str_to_set(table_columns, field);
            }

            ptr = strtok(NULL, delim);
            if (strcmp(ptr, ",") == 0) 
            {
               step = stepSelectComma;
            }
            else 
            {
               step = stepSelectFrom;
            }

            break;
         }
         case stepSelectComma: 
         {
            current_field_ptr->next = (list_node_t *) calloc(1, sizeof(list_node_t));
            current_field_ptr = current_field_ptr->next;

            step = stepSelectField;
            ptr = strtok(NULL, delim);
            break;
         }
         case stepSelectFrom: 
         {
            query->table_name_ptr = (table_name_t *) calloc(1, sizeof(table_name_t));
            current_table_name = query->table_name_ptr;
            step = stepSelectFromTable;
            ptr = strtok(NULL, delim);
            break;
         }
         case stepSelectFromTable: 
         {
            current_table_name->name = ptr;
            current_table_name->alt_name[0] = ptr[0];
            ptr = strtok(NULL, delim);

            if (ptr != NULL)
            {
               if (strcmp(ptr, "WHERE") == 0)
               {
                  step = stepWhere;
               }
               else if (strcmp(ptr, ",") == 0)
               {
                  current_table_name->next_table = (table_name_t *) calloc(1, sizeof(table_name_t));
                  current_table_name = current_table_name->next_table;

                  ptr = strtok(NULL, delim);
                  step = stepSelectFromTable;
               }
            }

            break;
         }
         case stepWhere: 
         {
            ptr = strtok(NULL, delim);

            if (query->condition_ptr == NULL) 
            {
               query->condition_ptr = (condition_t *) calloc(1, sizeof(condition_t));
               current_condition_ptr = query->condition_ptr;
            }

            step = stepWhereField;
            break;
         }
         case stepWhereField: 
         {
            char *field = calloc(1, sizeof(char *));
            char *correspond_table = calloc(1, sizeof(char *));
            parse_string(ptr, field, correspond_table);

            current_condition_ptr->operand1 = calloc(1, sizeof(field_operand_t));
            current_condition_ptr->operand1->name = field;
            current_condition_ptr->not = false;

            if (strcmp(correspond_table, "") != 0) 
            {
               current_condition_ptr->operand1->table = correspond_table;

               hashset_t table_columns = (hashset_t) ht_get(sql_tables_columns, correspond_table);
               if (table_columns == NULL) {
                  table_columns = hashset_create();
                  hashset_add(table_columns, "");
                  ht_set(sql_tables_columns, correspond_table, table_columns);
               }
               add_str_to_set(table_columns, field);
            }

            ptr = strtok(NULL, delim);

            if (strcmp(ptr, "NOT") == 0) 
            {
               step = stepWhereNot;
            }
            else 
            {
               step = stepWhereOperator;
            }

            break;
         }
         case stepWhereNot: 
         {
            current_condition_ptr->not = true;
            ptr = strtok(NULL, delim);
            step = stepWhereOperator;
            break;
         }
         case stepWhereOperator: 
         {
            if (strcmp(ptr, "LIKE") == 0) 
            {
               current_condition_ptr->operator = LIKE;
               step = stepWhereLike;
            }
            else if (strcmp(ptr, "IN") == 0) {
               current_condition_ptr->operator = IN;
               step = stepWhereIn;
            }
            else if (strcmp(ptr, "BETWEEN") == 0) 
            {
               current_condition_ptr->operator = BETWEEN;
               step = stepWhereBetween;
            }
            else if (strcmp(ptr, "=") == 0) 
            {
               current_condition_ptr->operator = EQ;
               step = stepWhereComparison;
            }
            else if (strcmp(ptr, ">") == 0)
            {
               current_condition_ptr->operator = GT;
               step = stepWhereComparison;
            }
            else if (strcmp(ptr, "<") == 0)
            {
               current_condition_ptr->operator = LT;
               step = stepWhereComparison;
            }
            else if (strcmp(ptr, ">=") == 0)
            {
               current_condition_ptr->operator = GTE;
               step = stepWhereComparison;
            }
            else if (strcmp(ptr, "<=") == 0)
            {
               current_condition_ptr->operator = LTE;
               step = stepWhereComparison;
            }
            else if (strcmp(ptr, "!=") == 0)
            {
               current_condition_ptr->operator = NE;
               step = stepWhereComparison;
            }

            ptr = strtok(NULL, delim);
            break;
         }
         case stepWhereComparison:
         {
            current_comparison_condition = (comparison_condition_t *) calloc(1, sizeof(comparison_condition_t));

            current_condition_ptr->operand2 = current_comparison_condition;
            step = stepWhereComparisonValue;
            break;
         }
         case stepWhereLike: 
         {
            curent_like_condition = (like_condition_t *) calloc(1, sizeof(like_condition_t));
            curent_like_condition->ex = ptr;

            current_condition_ptr->operand2 = curent_like_condition;
            step = stepWhereLikeValue;
            break;
         }
         case stepWhereBetween:
         {
            current_between_condition = (between_condition_t *) calloc(1, sizeof(between_condition_t));

            current_condition_ptr->operand2 = current_between_condition;
            step = stepWhereBetweenValue;
            break;
         }
         case stepWhereIn: 
         {
            current_in_condition = (in_condition_t *) calloc(1, sizeof(in_condition_t));
            current_in_condition->match_ptr = (list_node_t *) calloc(1, sizeof(list_node_t));
            current_in_value_ptr = current_in_condition->match_ptr;

            current_condition_ptr->operand2 = current_in_condition;
            step = stepWhereInValue;
            break;
         }
         case stepWhereComparisonValue:
         {
            char *value = calloc(1, sizeof(char *));
            char *correspond_table = calloc(1, sizeof(char *));
            parse_string(ptr, value, correspond_table);

            current_comparison_condition->value = value;
            if (strcmp(correspond_table, "") != 0)
            {
               current_comparison_condition->value_type = COLUMN_FIELD;
               current_comparison_condition->table = correspond_table;
               hashset_t table_columns = (hashset_t) ht_get(sql_tables_columns, correspond_table);
               if (table_columns == NULL) {
                  table_columns = hashset_create();
                  hashset_add(table_columns, "");
                  ht_set(sql_tables_columns, correspond_table, table_columns);
               }
               add_str_to_set(table_columns, value);
            }
            else 
            {
               current_comparison_condition->value_type = CONSTANT;
            }

            ptr = strtok(NULL, delim);
            step = stepWhereValueType;
            break;
         }
         case stepWhereLikeValue: 
         {
            char *required_ex = calloc(1, strlen(ptr));
            strcpy(required_ex, ptr);
            required_ex[0] = '^';
            required_ex[strlen(required_ex) - 1] = '$';

            str_replace(required_ex, "%", ".*");

            regcomp(&(curent_like_condition->regex), required_ex, 0);

            ptr = strtok(NULL, delim);
            step = stepWhereContinue;

            break;
         }
         case stepWhereBetweenValue: 
         {
            char *between_value = calloc(1, strlen(ptr));
            strcpy(between_value, ptr);

            // remove '()'
            if (between_value[0] == '(') 
            {
               between_value += 1;
            }
            if (between_value[strlen(between_value) - 1] == ')') 
            {
               between_value[strlen(between_value) - 1] = '\0';
            }

            // remove ''
            if (between_value[0] == '\'') 
            {
               between_value += 1;
            }
            if (between_value[strlen(between_value) - 1] == '\'') 
            {
               between_value[strlen(between_value) - 1] = '\0';
            }

            if (current_between_condition->min_value == NULL) 
            {
               current_between_condition->min_value = ptr;
               current_between_condition->min_value_type = CONSTANT;
            }
            else if (current_between_condition->max_value == NULL) {
               current_between_condition->max_value = ptr;
               current_between_condition->max_value_type = CONSTANT;
            }

            ptr = strtok(NULL, delim);
            if (ptr != NULL) {
               if (strcmp(ptr, "AND") == 0) {
                  ptr = strtok(NULL, delim);
                  step = stepWhereBetweenValue;
               }
               else {
                  step = stepWhereValueType;
               }
            }
            break;
         }
         case stepWhereInValue: 
         {
            char *in_value = calloc(1, strlen(ptr));
            strcpy(in_value, ptr);

            // remove '()'
            if (in_value[0] == '(') 
            {
               in_value += 1;
            }
            if (in_value[strlen(in_value) - 1] == ')') 
            {
               in_value[strlen(in_value) - 1] = '\0';
            }

            // remove ''
            if (in_value[0] == '\'') 
            {
               in_value += 1;
            }
            if (in_value[strlen(in_value) - 1] == '\'') 
            {
               in_value[strlen(in_value) - 1] = '\0';
            }

            current_in_value_ptr->val = in_value;

            ptr = strtok(NULL, delim);
            if (ptr != NULL && strcmp(ptr, ",") == 0) 
            {
               current_in_value_ptr->next = (list_node_t *) calloc(1, sizeof(list_node_t));
               current_in_value_ptr = current_in_value_ptr->next;

               ptr = strtok(NULL, delim);
               step = stepWhereInValue;
            }
            else 
            {
               step = stepWhereContinue;
            }

            break;
         }
         case stepWhereValueType:
         {
            if (ptr != NULL) {
               if (strcmp(ptr, "+") == 0 || strcmp(ptr, "-") == 0 || strcmp(ptr, "*") == 0 || strcmp(ptr, "/") == 0) 
               {
                  arithmetic_condition_t *athm_condition = (arithmetic_condition_t *) calloc(1, sizeof(arithmetic_condition_t));
                  athm_condition->operator = ptr;
                  ptr = strtok(NULL, delim);
                  athm_condition->operand2 = ptr;
                  switch (current_condition_ptr->operator)
                  {
                     case EQ:
                     case GT:
                     case LT:
                     case GTE:
                     case LTE:
                     case NE:
                     {
                        athm_condition->operand1 = (char *) current_comparison_condition->value;
                        
                        current_comparison_condition->value = athm_condition;
                        current_comparison_condition->value_type = ARITHMETIC;

                        ptr = strtok(NULL, delim);
                        step = stepWhereContinue;
                        break;
                     }
                     case BETWEEN:
                     {
                        if (current_between_condition->max_value == NULL) {
                           athm_condition->operand1 = current_between_condition->min_value;

                           current_between_condition->min_value = athm_condition;
                           current_between_condition->min_value_type = ARITHMETIC;

                           ptr = strtok(NULL, delim);
                           if (strcmp(ptr, "AND") == 0) {
                              ptr = strtok(NULL, delim);
                              step = stepWhereBetweenValue;
                           }
                        }
                        else 
                        {
                           athm_condition->operand1 = current_between_condition->max_value;

                           current_between_condition->max_value = athm_condition;
                           current_between_condition->max_value_type = ARITHMETIC;

                           ptr = strtok(NULL, delim);
                           step = stepWhereContinue;
                        }
                        break;
                     }
                     default:
                     {
                        break;
                     }
                  }
               }
            }  
            break;
         }
         case stepWhereContinue: 
         {
            if (ptr != NULL) {
               if (strcmp(ptr, "AND") == 0) 
               {
                  step = stepWhereAnd;
               }
               else if (strcmp(ptr, "OR") == 0)
               {
                  step = stepWhereOr;
               }
               break;
            }
         }
         case stepWhereAnd: 
         {
            if (current_and_condition_ptr == NULL)
            {
               query->and_condition_ptr = (condition_t *) calloc(1, sizeof(condition_t));
               current_and_condition_ptr = query->and_condition_ptr;
            }
            else 
            {
               current_and_condition_ptr->next_condition = (condition_t *) calloc(1, sizeof(condition_t));
               current_and_condition_ptr = current_and_condition_ptr->next_condition;
            }

            current_condition_ptr = current_and_condition_ptr;
            ptr = strtok(NULL, delim);
            step = stepWhereField;
            break;
         }
         case stepWhereOr: 
         {
            if (query->or_condition_ptr == NULL) 
            {
               query->or_condition_ptr = (condition_t *) calloc(1, sizeof(condition_t));
               current_or_condition_ptr = query->or_condition_ptr;
            }
            else
            {
               current_or_condition_ptr->next_condition = (condition_t *) calloc(1, sizeof(condition_t));
               current_or_condition_ptr = current_or_condition_ptr->next_condition;
            }

            current_condition_ptr = current_or_condition_ptr;
            ptr = strtok(NULL, delim);
            step = stepWhereField;
            break;
         }
         default: 
         {
            ptr = strtok(NULL, delim);
            break;
         }
      }
   }

   return query;
}

int main() 
{
   sql_tables_columns = ht_create();
   // char inputQuerry[] = "SELECT QUANTITY , TAX , DISCOUNT , RETURNFLAG , SHIPDATE FROM table WHERE SHIPDATE LIKE '1998-09%' AND RETURNFLAG IN ('A' , 'B') OR DISCOUNT > 100";
   char inputQuerry[] = "SELECT a_QUANTITY , a_TAX , a_DISCOUNT , a_RETURNFLAG FROM a_t1 , b_t2 WHERE a_DISCOUNT = b_DISCOUNT";

   query_t *query = parse(inputQuerry);

   printf("Query object result\n");
   print_query_object(query);

   printf("\ntable columns object result\n");
   hti iterator = ht_iterator(sql_tables_columns);
   while (ht_next(&iterator)) {
      char *selected_table = (char *) iterator.key;
      hashset_t table_columns = (hashset_t) iterator.value;
      hashset_itr_t iter = hashset_iterator(table_columns);

      printf("%s ->", selected_table);
      while(hashset_iterator_has_next(iter))
      {
         printf(" %s", (char *) hashset_iterator_value(iter));
         hashset_iterator_next(iter);
      }
      hashset_destroy(table_columns);
      printf("\n");
   }

   ht_destroy(sql_tables_columns);
   return 0;
}