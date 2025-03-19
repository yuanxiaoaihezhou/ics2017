#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum
{
	TK_NOTYPE = 256, 
	TK_DEREF_, TK_NEG_, // *, -
	TK_EQ, TK_NEQ, TK_LE, TK_GE, TK_LT, TK_GT, TK_AND, TK_OR, // ==, !=, <=, >=, <, >, &&, ||
	TK_PLUS, TK_SUB, TK_LPARE, TK_RPARE, TK_MUL, TK_DIV, TK_NOT, // +, -, (, ), *, /, !
	TK_EOS_, TK_REG, TK_HEX, TK_OCT, TK_DEC, // reg, hex, oct, dec

	/* TODO: Add more token types */

};

static int8_t preced[][TK_EOS_ - TK_NOTYPE] = {
	/* TK_DEREF */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '>', '>', '>', '<', '>'},
	/* TK_NEG_  */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '>', '>', '>', '<', '>'},
	/* TK_EQ    */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '>', '<', '<', '<', '>'},
	/* TK_NEQ   */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '>', '<', '<', '<', '>'},
	/* TK_LE    */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '>', '<', '<', '<', '>'},
	/* TK_GE    */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '>', '<', '<', '<', '>'},
	/* TK_LT    */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '>', '<', '<', '<', '>'},
	/* TK_GT    */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '>', '<', '<', '<', '>'},
	/* TK_AND   */ {'<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '<', '<', '<', '>', '<', '<', '<', '>'},
	/* TK_OR    */ {'<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '<', '<', '<', '>', '<', '<', '<', '>'},
	/* TK_PLUS  */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '>', '<', '<', '<', '>'},
	/* TK_SUB   */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '>', '<', '<', '<', '>'},
	/* TK_LPARE */ {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '=', '<', '<', '<',  0,},
	/* TK_RPARE */ {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '=', '>', '>', '>', '>', '>'},
	/* TK_MUL   */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '>', '>', '>', '<', '>'},
	/* TK_DIV   */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '>', '>', '>', '<', '>'},
	/* TK_NOT   */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '>', '>', '>', '<', '>'},
	/* TK_EOS_  */ {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',  0,  '<', '<', '<', '='},
};

static int8_t op_preced(int op1, int op2)
{
	return preced[op1 - TK_NOTYPE - 1][op2 - TK_NOTYPE - 1];
}

static struct rule
{
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

	{" +", TK_NOTYPE},
	{"\\(", TK_LPARE},
	{"\\)", TK_RPARE},
	{"\\+", TK_PLUS},
	{"-", TK_SUB},
	{"\\*", TK_MUL},
	{"/", TK_DIV},
	{"==", TK_EQ},
	{"!=", TK_NEQ},
	{"<=", TK_LE},
	{">=", TK_GE},
	{">", TK_GT},
	{"<", TK_LT},
	{"&&", TK_AND},
	{"\\|\\|", TK_OR},
	{"!", TK_NOT},
	{"\\$[a-zA-Z]+", TK_REG},
	{"0x[0-9a-fA-F]+", TK_HEX},
	{"0[0-7]+", TK_OCT},
	{"[0-9]+", TK_DEC},
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
	int i;
	char error_msg[128];
	int ret;

	for (i = 0; i < NR_REGEX; i++)
	{
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if (ret != 0)
		{
			regerror(ret, &re[i], error_msg, 128);
			panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token
{
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e)
{
	int position = 0;
	int i;
	regmatch_t pmatch;

	nr_token = 0;

	while (e[position] != '\0')
	{
		/* Try all rules one by one. */
		for (i = 0; i < NR_REGEX; i++)
		{
			if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
			{
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
					i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

				switch (rules[i].token_type)
				{
				case TK_NOTYPE:
					break;
				default:
					{
						tokens[nr_token].type = rules[i].token_type;
						size_t len_to_copy = (substr_len < sizeof(tokens[nr_token].str))
												 ? substr_len
												 : sizeof(tokens[nr_token].str) - 1; // Reserve space for null terminator
						strncpy(tokens[nr_token].str, substr_start, len_to_copy);
						tokens[nr_token].str[len_to_copy] = '\0';
						nr_token++;
					}
					break;
				}
				break; // Move break here as it's always executed after a match
			}
		}

		if (i == NR_REGEX)
		{
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true;
}

static bool is_op(int type)
{
	if (type > TK_NOTYPE && type <= TK_EOS_)
		return true;
	return false;
}

static uint32_t operate(int op, uint32_t obj1, uint32_t obj2)
{
	switch (op)
	{
	case TK_DEREF_:
		return vaddr_read(obj1, 4);
	case TK_NEG_:
		return -obj1;
	case TK_NOT:
		return !obj1;
	case TK_MUL:
		return obj1 * obj2;
	case TK_DIV:
		return obj1 / obj2;
	case TK_PLUS:
		return obj1 + obj2;
	case TK_SUB:
		return obj1 - obj2;
	case TK_EQ:
		return obj1 == obj2;
	case TK_NEQ:
		return obj1 != obj2;
	case TK_LE:
		return obj1 <= obj2;
	case TK_GE:
		return obj1 >= obj2;
	case TK_LT:
		return obj1 < obj2;
	case TK_GT:
		return obj1 > obj2;
	case TK_AND:
		return obj1 && obj2;
	case TK_OR:
		return obj1 || obj2;
	default:
		return 0;
	}
}

static uint32_t eval(bool *success)
{
	static int op_stack[32];
	static uint32_t obj_stack[32];
	int obj_i = 0, op_i = 0;
	int token_type, i;
	int op;
	uint32_t o1, o2;

	*success = true;

	op_stack[op_i++] = TK_EOS_;

	tokens[nr_token].type = TK_EOS_;
	nr_token++;

	for (i = 0;
		 (op_stack[op_i - 1] != TK_EOS_) || tokens[i].type != TK_EOS_;)
	{
		if (is_op((token_type = tokens[i].type)))
		{

			if (i == 0 || is_op(tokens[i - 1].type))
			{
				if (token_type == TK_SUB && tokens[i - 1].type != TK_RPARE)
				{
					token_type = TK_NEG_;
				}
				else if (token_type == TK_MUL && tokens[i - 1].type != TK_RPARE)
				{
					token_type = TK_DEREF_;
				}
				else if (token_type != TK_LPARE && tokens[i - 1].type != TK_RPARE)
				{
					*success = false;
					return -1;
				}
			}

			switch (op_preced(op_stack[op_i - 1], token_type))
			{
			case '<':
				op_stack[op_i++] = token_type;
				i++;
				break;
			case '>':
				op = op_stack[--op_i];
				if (op == TK_NOT || op == TK_NEG_ || op == TK_DEREF_)
				{
					o1 = obj_stack[--obj_i];
					obj_stack[obj_i++] = operate(op, o1, 0);
				}
				else
				{
					o2 = obj_stack[--obj_i];
					o1 = obj_stack[--obj_i];
					obj_stack[obj_i++] = operate(op, o1, o2);
				}
				break;
			case '=':
				--op_i;
				i++;
				break;
			default:
				*success = false;
				return -1;
			}
		}
		else if (token_type == TK_REG)
		{
			int j;
			const char *reg = tokens[i++].str + 1; /* skip '$' */
			/* GPR */
			for (j = R_EAX; j <= R_EDI; j++)
			{
				if (strcmp(regsl[j], reg) == 0)
				{ /* skip '$' */
					obj_stack[obj_i++] = reg_l(j);
					break;
				}
			}
			if (j <= R_EDI)
				continue;
			for (j = R_AX; j <= R_DI; j++)
			{
				if (strcmp(regsw[j], reg) == 0)
				{ /* skip '$' */
					obj_stack[obj_i++] = reg_w(j);
					break;
				}
			}
			if (j <= R_DI)
				continue;
			for (j = R_AL; j <= R_BH; j++)
			{
				if (strcmp(regsb[j], reg) == 0)
				{ /* skip '$' */
					obj_stack[obj_i++] = reg_b(j);
					break;
				}
			}
			if (j <= R_DI)
				continue;
			/* EIP */
			if (strcmp("eip", reg) == 0)
			{
				obj_stack[obj_i++] = cpu.eip;
				continue;
			}

			*success = false;
			return 0;
		}
		else
		{
			int j;
			sscanf(tokens[i].str, "%i", &j);
			obj_stack[obj_i++] = j;
			i++;
		}
	}
	return obj_stack[0];
}

uint32_t expr(char *e, bool *success)
{
	if (!make_token(e))
	{
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	return eval(success);
}
