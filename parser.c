#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/query.h"

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
   stepWhereEquality,
   stepWhereIn,
   stepWhereLike,
   stepWhereBetween,
   stepWhereEqualityValue,
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

query_t* parse(char *inputQuerry) {
   char* ptr = strtok(inputQuerry, delim);

   query_t *query = (query_t *) calloc(1, sizeof(query_t));
	state_t step = stepType;
	list_node_t *current_field_ptr = NULL;

	condition_t *current_condition_ptr = NULL;
   condition_t *current_and_condition_ptr = NULL;
   condition_t *current_or_condition_ptr = NULL;

   simple_condition_t *current_euqality_condition = NULL;
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
            current_field_ptr->val = ptr;
            step = stepSelectFrom;

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
            step = stepSelectFromTable;
            ptr = strtok(NULL, delim);
            break;
         }
         case stepSelectFromTable: 
         {
            query->table_name = ptr;
            ptr = strtok(NULL, delim);

            if (ptr != NULL && strcmp(ptr, "WHERE") == 0)
            {
               step = stepWhere;
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
            current_condition_ptr->operand1 = ptr;
            current_condition_ptr->not = false;

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
               step = stepWhereEquality;
            }
            else if (strcmp(ptr, ">") == 0)
            {
               current_condition_ptr->operator = GT;
               step = stepWhereEquality;
            }
            else if (strcmp(ptr, "<") == 0)
            {
               current_condition_ptr->operator = LT;
               step = stepWhereEquality;
            }
            else if (strcmp(ptr, ">=") == 0)
            {
               current_condition_ptr->operator = GTE;
               step = stepWhereEquality;
            }
            else if (strcmp(ptr, "<=") == 0)
            {
               current_condition_ptr->operator = LTE;
               step = stepWhereEquality;
            }
            else if (strcmp(ptr, "!=") == 0)
            {
               current_condition_ptr->operator = NE;
               step = stepWhereEquality;
            }

            ptr = strtok(NULL, delim);
            break;
         }
         case stepWhereEquality:
         {
            current_euqality_condition = (simple_condition_t *) calloc(1, sizeof(simple_condition_t));

            current_condition_ptr->operand2 = current_euqality_condition;
            step = stepWhereEqualityValue;
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
         case stepWhereEqualityValue:
         {
            current_euqality_condition->value = ptr;
            current_euqality_condition->value_type = SINGLE;

            ptr = strtok(NULL, delim);
            step = stepWhereValueType;
            break;
         }
         case stepWhereLikeValue: 
         {
            char *required_ex = malloc(strlen(ptr));
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
            char *between_value = malloc(strlen(ptr));
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
               current_between_condition->min_value_type = SINGLE;
            }
            else if (current_between_condition->max_value == NULL) {
               current_between_condition->max_value = ptr;
               current_between_condition->max_value_type = SINGLE;
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
            char *in_value = malloc(strlen(ptr));
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
                        athm_condition->operand1 = (char *) current_euqality_condition->value;
                        
                        current_euqality_condition->value = athm_condition;
                        current_euqality_condition->value_type = ARITHMETIC;

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
//   char inputQuerry[] = "SELECT QUANTITY , TAX , DISCOUNT , RETURNFLAG , SHIPDATE FROM table WHERE SHIPDATE LIKE '1998-09%' AND RETURNFLAG IN ('A' , 'B') OR DISCOUNT > 100";
  char inputQuerry[] = "SELECT QUANTITY , TAX , DISCOUNT , RETURNFLAG , SHIPDATE FROM table WHERE DISCOUNT BETWEEN 0 + 100 AND 500";

  query_t *query = parse(inputQuerry);
  
  printf("Query object result\n");
  print_query_object(query);

    return 0;
}