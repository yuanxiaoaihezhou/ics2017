#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum
{
	TK_NOTYPE = 256,
	TK_DEREF_,
	TK_NEG_, // *, -
	TK_EQ,
	TK_NEQ,
	TK_LE,
	TK_GE,
	TK_LT,
	TK_GT,
	TK_AND,
	TK_OR, // ==, !=, <=, >=, <, >, &&, ||
	TK_PLUS,
	TK_SUB,
	TK_LPARE,
	TK_RPARE,
	TK_MUL,
	TK_DIV,
	TK_NOT, // +, -, (, ), *, /, !
	TK_EOS_,
	TK_REG,
	TK_HEX,
	TK_OCT,
	TK_DEC, // reg, hex, oct, dec

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
	/* TK_LPARE */ {
		'<',
		'<',
		'<',
		'<',
		'<',
		'<',
		'<',
		'<',
		'<',
		'<',
		'<',
		'<',
		'<',
		'=',
		'<',
		'<',
		'<',
		0,
	},
	/* TK_RPARE */ {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '=', '>', '>', '>', '>', '>'},
	/* TK_MUL   */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '>', '>', '>', '<', '>'},
	/* TK_DIV   */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '>', '>', '>', '<', '>'},
	/* TK_NOT   */ {'<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '>', '>', '>', '<', '>'},
	/* TK_EOS_  */ {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', 0, '<', '<', '<', '='},
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
					tokens[nr_token].str[0] = '\0';
					strncat(tokens[nr_token].str, substr_start,
							(substr_len < sizeof(tokens[nr_token].str))
								? substr_len
								: sizeof(tokens[nr_token].str));
					/* through down */
				case TK_EQ:
				case TK_NEQ:
				case TK_LE:
				case TK_GE:
				case TK_AND:
				case TK_OR:
				case TK_PLUS:
				case TK_SUB:
				case TK_LPARE:
				case TK_RPARE:
				case TK_MUL:
				case TK_DIV:
				case TK_LT:
				case TK_GT:
				case TK_NOT:
				case TK_EOS_:
					tokens[nr_token].type = rules[i].token_type;
					nr_token++;
				}

				break;
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
	static int op_stack[32]; // 运算符栈
	static uint32_t obj_stack[32]; // 操作数栈
	int obj_i = 0, op_i = 0;
	int token_type, i = 0;
	int op;
	uint32_t o1, o2;

	if (success == NULL) {
		return 0; // success 指针无效
	}
	*success = true;

	op_stack[op_i++] = TK_EOS_;

	if (tokens == NULL || nr_token < 0) {
		*success = false;
		return 0; // tokens 无效或 nr_token 无效
	}
	tokens[nr_token].type = TK_EOS_;
	nr_token++;

	for (i = 0;
		 (op_stack[op_i - 1] != TK_EOS_) || (i < nr_token && tokens[i].type != TK_EOS_);) // 确保 i 在 tokens 的有效范围内
	{
		if (i >= nr_token) {
			*success = false;
			return 0; // i 超出 tokens 范围
		}

		token_type = tokens[i].type;

		if (is_op(token_type))
		{
			if (i == 0 || (i > 0 && is_op(tokens[i - 1].type)))
			{
				if (token_type == TK_SUB && (i > 0 && tokens[i - 1].type != TK_RPARE))
				{
					token_type = TK_NEG_;
				}
				else if (token_type == TK_MUL && (i > 0 && tokens[i - 1].type != TK_RPARE))
				{
					token_type = TK_DEREF_;
				}
				else if (token_type != TK_LPARE && (i > 0 && tokens[i - 1].type != TK_RPARE))
				{
					*success = false;
					return -1;
				}
				else if (i == 0 && token_type != TK_SUB && token_type != TK_MUL && token_type != TK_LPARE) {
					*success = false;
					return -1; // 第一个 token 不能是此运算符
				}
			}

			if (op_i >= sizeof(op_stack) / sizeof(op_stack[0])) {
				*success = false;
				return -1; // 运算符栈溢出
			}

			switch (op_preced(op_stack[op_i - 1], token_type))
			{
			case '<':
				op_stack[op_i++] = token_type;
				i++;
				break;
			case '>':
				if (op_i <= 0) {
					*success = false;
					return -1; // 运算符栈下溢
				}
				op = op_stack[--op_i];
				if (op == TK_NOT || op == TK_NEG_ || op == TK_DEREF_)
				{
					if (obj_i <= 0) {
						*success = false;
						return -1; // 操作数栈下溢
					}
					o1 = obj_stack[--obj_i];
					if (obj_i >= sizeof(obj_stack) / sizeof(obj_stack[0])) {
						*success = false;
						return -1; // 操作数栈溢出
					}
					obj_stack[obj_i++] = operate(op, o1, 0);
				}
				else
				{
					if (obj_i <= 1) {
						*success = false;
						return -1; // 操作数栈下溢
					}
					o2 = obj_stack[--obj_i];
					o1 = obj_stack[--obj_i];
					if (obj_i >= sizeof(obj_stack) / sizeof(obj_stack[0])) {
						*success = false;
						return -1; // 操作数栈溢出
					}
					obj_stack[obj_i++] = operate(op, o1, o2);
				}
				break;
			case '=':
				if (op_i <= 0) {
					*success = false;
					return -1; // 运算符栈下溢
				}
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
			if (i >= nr_token || tokens[i].str == NULL) {
				*success = false;
				return 0; // 无效的 token 或 token 字符串
			}
			const char *reg = tokens[i++].str + 1; /* 跳过 '$' */
			/* GPR */
			for (j = R_EAX; j <= R_EDI; j++)
			{
				if (regsl[j] != NULL && strcmp(regsl[j], reg) == 0)
				{
					if (obj_i >= sizeof(obj_stack) / sizeof(obj_stack[0])) {
						*success = false;
						return -1; // 操作数栈溢出
					}
					obj_stack[obj_i++] = reg_l(j);
					break;
				}
			}
			if (j <= R_EDI)
				continue;
			for (j = R_AX; j <= R_DI; j++)
			{
				if (regsw[j] != NULL && strcmp(regsw[j], reg) == 0)
				{
					if (obj_i >= sizeof(obj_stack) / sizeof(obj_stack[0])) {
						*success = false;
						return -1; // 操作数栈溢出
					}
					obj_stack[obj_i++] = reg_w(j);
					break;
				}
			}
			if (j <= R_DI)
				continue;
			for (j = R_AL; j <= R_BH; j++)
			{
				if (regsb[j] != NULL && strcmp(regsb[j], reg) == 0)
				{
					if (obj_i >= sizeof(obj_stack) / sizeof(obj_stack[0])) {
						*success = false;
						return -1; // 操作数栈溢出
					}
					obj_stack[obj_i++] = reg_b(j);
					break;
				}
			}
			if (j <= R_DI)
				continue;
			/* EIP */
			if (strcmp("eip", reg) == 0)
			{
				if (obj_i >= sizeof(obj_stack) / sizeof(obj_stack[0])) {
					*success = false;
					return -1; // 操作数栈溢出
				}
				obj_stack[obj_i++] = cpu.eip; // 假设 cpu 是一个有效的指针
				continue;
			}

			*success = false;
			return 0;
		}
		else
		{
			int j;
			if (i >= nr_token || tokens[i].str == NULL) {
				*success = false;
				return 0; // 无效的 token 或 token 字符串
			}
			if (sscanf(tokens[i].str, "%i", &j) != 1) {
				*success = false;
				return 0; // 解析失败
			}
			if (obj_i >= sizeof(obj_stack) / sizeof(obj_stack[0])) {
				*success = false;
				return -1; // 操作数栈溢出
			}
			obj_stack[obj_i++] = j;
			i++;
		}
	}

	if (obj_i != 1) {
		*success = false;
		return 0; // 无效的表达式
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
