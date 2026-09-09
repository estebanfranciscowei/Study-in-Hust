#include "parser.h"
#include "lexer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*  全局变量定义  */
int w;
int parse_err = 0;
int parse_error_total = 0;  /* 语法错误总数 */

/* ---表达式分析：运算符栈 + 操作数栈 + 优先关系矩阵 --- */

#define STACK_SIZE 256

/* 表达式分析上下文 */
typedef struct {
    AstNode *operand_stack[STACK_SIZE];
    int      op_stack[STACK_SIZE];
    int      operand_top;
    int      op_top;
    int      expect_operand;   /* 1=期望操作数, 0=期望运算符 */
} ExprContext;

/* 运算符优先级：数值越大优先级越高 */
static int op_priority(int op)
{
    switch (op) {
        case LP:            return 0;   /* 左括号特殊处理 */
        case ASSIGN:        return 1;   /* 赋值，右结合 */
        case OR:            return 2;   /* || */
        case AND:           return 3;   /* && */
        case EQ: case NEQ:  return 4;   /* == != */
        case LT: case LE: case GT: case GE: return 5; /* < <= > >= */
        case PLUS: case MINUS: return 6; /* + - */
        case MUL: case DIV: case MOD: return 7; /* * / % */
        case NOT:           return 8;   /* ! 单目 */
        case TOKEN_EOF:     return -1;  /* 栈底标记，最低 */
        default:            return -1;
    }
}

/* 是否右结合 */
static int is_right_assoc(int op)
{
    return op == ASSIGN;
}

/*
 * 优先关系比较：op1是栈顶运算符，op2是当前运算符
 * 返回 '>'：归约op1；'<': op2入栈；'=': 括号配对
 */
static char precede(int op1, int op2)
{
    if (op1 == LP) {
        if (op2 == RP) return '=';  /* 括号配对 */
        return '<';                   /* 左括号后任何运算符都入栈 */
    }
    if (op2 == LP) return '<';       /* 当前是左括号，入栈 */
    if (op2 == RP) return '>';       /* 当前是右括号，归约直到左括号 */

    int p1 = op_priority(op1);
    int p2 = op_priority(op2);

    if (p1 > p2) return '>';
    if (p1 < p2) return '<';
    /* 优先级相等 */
    if (is_right_assoc(op1)) return '<';  /* 右结合：新运算符入栈 */
    return '>';                              /* 左结合：归约栈顶 */
}

/* 归约：弹出一个运算符，弹出操作数，生成AST结点，压回 */
static void reduce(ExprContext *ctx)
{
    if (ctx->op_top <= 1) return;  /* 栈底标记不弹出 */
    int op = ctx->op_stack[--ctx->op_top];

    if (op == NOT) {
        /* 单目运算符 */
        if (ctx->operand_top < 1) { parse_err = 1; parse_error_total++; return; }
        AstNode *operand = ctx->operand_stack[--ctx->operand_top];
        ctx->operand_stack[ctx->operand_top++] = ast_new_unary_op(op, operand);
    } else {
        /* 双目运算符 */
        if (ctx->operand_top < 2) { parse_err = 1; parse_error_total++; return; }
        AstNode *right = ctx->operand_stack[--ctx->operand_top];
        AstNode *left  = ctx->operand_stack[--ctx->operand_top];
        ctx->operand_stack[ctx->operand_top++] = ast_new_expr_op(op, left, right);
    }
}

/* 判断是否为运算符 */
static int is_operator(int tk)
{
    return (tk == PLUS || tk == MINUS || tk == MUL || tk == DIV || tk == MOD ||
            tk == ASSIGN || tk == EQ || tk == NEQ ||
            tk == GT || tk == GE || tk == LT || tk == LE ||
            tk == AND || tk == OR || tk == NOT);
}

/* 判断是否为操作数开头（不包括LP，LP由主循环处理） */
static int is_operand_start(int tk)
{
    return (tk == IDENT || tk == INT_CONST || tk == LONG_CONST || tk == FLOAT_CONST ||
            tk == CHAR_CONST || tk == STRING_CONST || tk == MINUS || tk == NOT);
}

/* 解析函数调用参数列表（进入时w为LP后第一个token） */
static AstNode* parse_arg_list(void)
{
    AstNode *first = NULL;
    AstNode *last = NULL;

    if (w == RP) {
        return ast_new_arg_list(NULL);  /* 无参数 */
    }

    while (w != RP && w != TOKEN_EOF && !parse_err) {
        AstNode *arg = Exp(COMMA);  /* 解析到逗号 */
        if (!arg) break;
        if (!first) first = arg;
        else last->next_sibling = arg;
        last = arg;

        if (w == COMMA) {
            w = gettoken();  /* 跳过逗号 */
        } else {
            break;
        }
    }

    return ast_new_arg_list(first);
}

/* 解析单个操作数（处理完后w指向下一个token）
 * 注意：LP括号表达式不在此处理，由Exp主循环处理 */
static AstNode* parse_operand(void)
{
    if (w == IDENT) {
        char name[128];
        strncpy(name, token_text, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        w = gettoken();

        if (w == LP) {
            /* 函数调用 */
            w = gettoken();  /* 跳过LP */
            AstNode *arg_list = parse_arg_list();
            if (w == RP) {
                w = gettoken();  /* 跳过RP */
            } else {
                printf("[语法错误 line:%d] 函数调用缺少右括号\n", prev_token_line);
                parse_err = 1; parse_error_total++;
            }
            return ast_new_func_call(name, arg_list);
        }
        if (w == LSQUARE) {
            /* 数组下标访问 a[i] */
            w = gettoken();  /* 跳过 [ */
            AstNode *index = Exp(RSQUARE);  /* 解析下标表达式到 ] */
            if (w == RSQUARE) {
                w = gettoken();  /* 跳过 ] */
            } else {
                printf("[语法错误 line:%d] 数组下标缺少右中括号\n", prev_token_line);
                parse_err = 1; parse_error_total++;
            }
            return ast_new_array_access(name, index);
        }
        /* 普通标识符，w已指向下一个token */
        return ast_new_ident(name);
    }

    if (w == INT_CONST) {
        char text_buf[128];
        strncpy(text_buf, token_text, sizeof(text_buf) - 1);
        text_buf[sizeof(text_buf) - 1] = '\0';
        long v = atol(token_text);
        w = gettoken();
        return ast_new_int_const(v, text_buf);
    }

    if (w == LONG_CONST) {
        char text_buf[128];
        strncpy(text_buf, token_text, sizeof(text_buf) - 1);
        text_buf[sizeof(text_buf) - 1] = '\0';
        long v = atol(token_text);
        w = gettoken();
        return ast_new_int_const(v, text_buf);
    }


    if (w == FLOAT_CONST) {
        char text_buf[128];
        strncpy(text_buf, token_text, sizeof(text_buf) - 1);
        text_buf[sizeof(text_buf) - 1] = '\0';
        double v = atof(token_text);
        w = gettoken();
        return ast_new_float_const(v, text_buf);
    }

    if (w == CHAR_CONST) {
        AstNode *n = ast_new_char_const(token_text);
        w = gettoken();
        return n;
    }

    if (w == STRING_CONST) {
        AstNode *n = ast_new_string_const(token_text);
        w = gettoken();
        return n;
    }

    if (w == MINUS || w == NOT) {
        /* 单目运算符 */
        int op = w;
        w = gettoken();
        AstNode *operand = parse_operand();
        return ast_new_unary_op(op, operand);
    }

    printf("[语法错误 line:%d] 意外的token '%s'，期望操作数\n", token_line, token_text);
    parse_err = 1; parse_error_total++;
    return NULL;
}

/*
 * endsym: 表达式结束符号（如SEMI、RP、COMMA）
 * 遇到 endsym / RP / TOKEN_EOF / RB / SEMI 时停止
 * 使用运算符优先分析法解析表达式，生成表达式AST
 */
AstNode* Exp(int endsym)
{
    /* 使用局部栈，避免递归调用（括号表达式、函数参数）时互相覆盖 */
    ExprContext ctx;
    ctx.operand_top = 0;
    ctx.op_top = 0;
    ctx.expect_operand = 1;   /* 表达式以操作数开头 */
    ctx.op_stack[ctx.op_top++] = TOKEN_EOF;  /* 栈底标记 */

    while (!parse_err) {
        /* 停止条件：
         * - TOKEN_EOF/RB/SEMI 直接停止
         * - endsym为RP时，不在这里停止，由下面的RP分支根据栈中是否有LP判断
         *   （栈中有LP说明是内层括号，应归约处理；无LP说明是外层结束符，停止）
         * - endsym为其他（如COMMA）时，w==endsym停止 */
        if (w == TOKEN_EOF || w == RB || w == SEMI) {
            break;
        }
        if (w == endsym && endsym != RP) {
            break;
        }
        /* 表达式结束场景（endsym==SEMI或RP等）：刚解析完一个完整操作数
         * （expect_operand==0）又遇到新的操作数开头，说明表达式缺少结束符
         * （缺分号或缺右括号），应停止让上层报错。
         * 注意：MINUS/NOT 在操作数栈非空时是双目运算符，不在此列 */
        if (!ctx.expect_operand &&
            (w == IDENT || w == INT_CONST || w == LONG_CONST || w == FLOAT_CONST ||
             w == CHAR_CONST || w == STRING_CONST ||
             (w == MINUS && ctx.operand_top == 0) ||
             (w == NOT && ctx.operand_top == 0))) {
            break;
        }

        if (w == LP) {
            /* 左括号：直接入运算符栈 */
            if (ctx.op_top >= STACK_SIZE) {
                printf("[语法错误] 表达式过于复杂\n");
                parse_err = 1; parse_error_total++;
                break;
            }
            ctx.op_stack[ctx.op_top++] = LP;
            ctx.expect_operand = 1;   /* 括号内以操作数开头 */
            w = gettoken();
        } else if (w == RP) {
            /* 右括号：先检查栈中是否有本层的LP */
            int has_lp = 0;
            for (int i = 0; i < ctx.op_top; i++) {
                if (ctx.op_stack[i] == LP) { has_lp = 1; break; }
            }
            if (!has_lp) {
                /* 栈中无LP，这个RP属于外层（函数调用/外层括号），停止Exp由上层处理 */
                break;
            }
            /* 栈中有LP，归约直到左括号 */
            while (ctx.op_stack[ctx.op_top - 1] != LP) {
                if (ctx.op_stack[ctx.op_top - 1] == TOKEN_EOF) {
                    printf("[语法错误 line:%d] 缺少左括号\n", token_line);
                    parse_err = 1; parse_error_total++;
                    break;
                }
                reduce(&ctx);
                if (parse_err) break;
            }
            if (parse_err) break;
            ctx.op_top--;  /* 弹出LP */
            ctx.expect_operand = 0;   /* 括号表达式结束，期望运算符 */
            w = gettoken();
        } else if (is_operand_start(w)) {
            /* MINUS/NOT 在操作数栈非空时是双目运算符，不是单目 */
            if ((w == MINUS || w == NOT) && ctx.operand_top > 0) {
                while (precede(ctx.op_stack[ctx.op_top - 1], w) == '>') {
                    reduce(&ctx);
                    if (parse_err) break;
                }
                if (parse_err) break;
                if (ctx.op_top >= STACK_SIZE) {
                    printf("[语法错误] 表达式过于复杂\n");
                    parse_err = 1; parse_error_total++;
                    break;
                }
                ctx.op_stack[ctx.op_top++] = w;
                ctx.expect_operand = 1;   /* 双目运算符后期望操作数 */
                w = gettoken();
            } else {
                AstNode *operand = parse_operand();
                if (!operand) break;
                if (ctx.operand_top >= STACK_SIZE) {
                    printf("[语法错误] 表达式过于复杂\n");
                    parse_err = 1; parse_error_total++;
                    break;
                }
                ctx.operand_stack[ctx.operand_top++] = operand;
                ctx.expect_operand = 0;   /* 操作数结束，期望运算符 */
            }
        } else if (is_operator(w)) {
            while (precede(ctx.op_stack[ctx.op_top - 1], w) == '>') {
                reduce(&ctx);
                if (parse_err) break;
            }
            if (parse_err) break;
            if (ctx.op_top >= STACK_SIZE) {
                printf("[语法错误] 表达式过于复杂\n");
                parse_err = 1; parse_error_total++;
                break;
            }
            ctx.op_stack[ctx.op_top++] = w;
            ctx.expect_operand = 1;   /* 运算符后期望操作数 */
            w = gettoken();
        } else {
            /* 意外token：通常是缺少分号或右括号，设置错误标志，由上层报告具体错误 */
            parse_err = 1;
            break;
        }
    }

    /* 归约剩余运算符 */
    while (!parse_err && ctx.op_stack[ctx.op_top - 1] != TOKEN_EOF) {
        if (ctx.op_stack[ctx.op_top - 1] == LP) {
            printf("[语法错误 line:%d] 缺少右括号\n", token_line);
            parse_err = 1; parse_error_total++;
            break;
        }
        reduce(&ctx);
    }

    if (parse_err || ctx.operand_top == 0) {
        return NULL;
    }

    return ctx.operand_stack[ctx.operand_top - 1];
}

/* 递归下降语法分析  */


AstNode* Program(void)
{
    w = gettoken();
    AstNode *ext_list = ExtDefList();
    if (parse_err) {
        ast_free(ext_list);
        return NULL;
    }
    return ast_new_prog(ext_list);
}

 /* 错误时，跳到下一个分号或文件结束，清除错误标志继续分析 */
 /* 功能：语法错误恢复，跳过当前错误语句到下一个分号，清除错误标志继续分析*/
static void sync_recover(void)
{
    int guard = 0;  /* 保护，防止无限循环 */
    while (w != TOKEN_EOF && w != SEMI && guard < 1000) {
        w = gettoken();
        guard++;
    }
    if (w == SEMI) {
        w = gettoken();  /* 跳过分号，继续下一条语句/定义 */
    }
    parse_err = 0;  /* 清除错误标志，继续分析 */
}

AstNode* ExtDefList(void)
{
    if (w == TOKEN_EOF) return NULL;
	
	    /* 处理预处理指令和注释（直接生成AST结点） */
    if (w == PRE_INCLUDE) {
        AstNode *node = ast_new_include(token_text);
        w = gettoken();
        AstNode *next = ExtDefList();
        if (next) ast_add_sibling(node, next);
        return ast_new_ext_def_list(node);
    }
    if (w == PRE_DEFINE) {
        AstNode *node = ast_new_define(token_text);
        w = gettoken();
        AstNode *next = ExtDefList();
        if (next) ast_add_sibling(node, next);
        return ast_new_ext_def_list(node);
    }
    if (w == COMMENT) {
        AstNode *node = ast_new_comment(token_text);
        w = gettoken();
        AstNode *next = ExtDefList();
        if (next) ast_add_sibling(node, next);
        return ast_new_ext_def_list(node);
    }
    
    AstNode *def = ExtDef();
    if (!def) {
        /* 错误恢复：跳过到同步token（分号或右大括号），继续分析 */
        if (parse_err) {
            sync_recover();
            if (w != TOKEN_EOF) {
                return ExtDefList();  /* 继续下一个外部定义 */
            }
        }
        return NULL;
    }

    AstNode *next = ExtDefList();
    if (next) {
        ast_add_sibling(def, next);
    }
    return ast_new_ext_def_list(def);
}

/* 判断是否为类型关键字 */
static int is_type_keyword(int tk)
{
    return tk == KW_INT || tk == KW_FLOAT || tk == KW_CHAR || tk == KW_LONG || tk == KW_VOID;
}

 /* <外部定义> ::= <类型> <标识符> (<变量序列>; | <函数定义>) */
 /*
 * 解析一个外部定义（外部变量定义、函数定义或函数声明）
 * w必须是类型关键字(int/float/char/long/void)
 */
AstNode* ExtDef(void)
{
    if (!is_type_keyword(w)) {
        printf("[语法错误 line:%d] 外部定义需要以类型关键字开头，实际是 '%s'\n",
               token_line, token_name(w));
        parse_err = 1; parse_error_total++;
        return NULL;
    }

    AstNode *type_node = ast_new_type(w);
    w = gettoken();

    if (w != IDENT) {
        printf("[语法错误 line:%d] 期望标识符，实际是 '%s'\n", token_line, token_name(w));
        parse_err = 1; parse_error_total++;
        ast_free(type_node);
        return NULL;
    }

    char name[128];
    strncpy(name, token_text, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    w = gettoken();

    if (w == LP) {
        /* 函数定义 */
        w = gettoken();  /* 跳过LP */
        AstNode *param_list = ParamList();
        if (w != RP) {
            printf("[语法错误 line:%d] 函数定义缺少右括号\n", prev_token_line);
            parse_err = 1; parse_error_total++;
            ast_free(type_node);
            ast_free(param_list);
            return NULL;
        }
        w = gettoken();  /* 跳过RP */

        if (w == SEMI) {
            /* 函数声明（原型）：int foo(int a); */
            w = gettoken();  /* 跳过分号 */
            return ast_new_func_decl(type_node, name, param_list);
        }

        if (w != LB) {
            printf("[语法错误 line:%d] 函数定义缺少左大括号，实际是 '%s'\n",
                   token_line, token_name(w));
            parse_err = 1; parse_error_total++;
            ast_free(type_node);
            ast_free(param_list);
            return NULL;
        }

        AstNode *body = Compound();
        return ast_new_func_def(type_node, name, param_list, body);
    } else {
        /* 外部变量定义：name已保存，w是name后面的token */
        AstNode *var_list = VarList(name);
        if (w != SEMI) {
            printf("[语法错误 line:%d] 外部变量定义缺少分号，实际是 '%s'\n",
                   prev_token_line, token_name(w));
            parse_err = 1; parse_error_total++;
            ast_free(type_node);
            ast_free(var_list);
            return NULL;
        }
        w = gettoken();  /* 跳过分号 */
        return ast_new_ext_var_def(type_node, var_list);
    }
}

/*  first_name: 调用者已经预读的第一个变量名（不为NULL时先处理它）
  进入时：如果first_name不为NULL，w是该变量名后面的token；
  如果first_name为NULL，w应该是IDENT */
AstNode* VarList(const char *first_name)
{
    AstNode *first = NULL;
    AstNode *last = NULL;

    /* 处理第一个变量 */
    if (first_name) {
        AstNode *init_expr = NULL;
        int arr_size = 0;
        int is_array = 0;
        if (w == LSQUARE) {
            /* 数组声明 a[10] */
            is_array = 1;
            w = gettoken();  /* 跳过[ */
            if (w == INT_CONST || w == LONG_CONST) {
                arr_size = atoi(token_text);
                w = gettoken();
            } else {
                printf("[语法错误 line:%d] 数组大小需要整型常量\n", token_line);
                parse_err = 1; parse_error_total++;
            }
            if (w == RSQUARE) {
                w = gettoken();  /* 跳过] */
            } else {
                printf("[语法错误 line:%d] 数组声明缺少右中括号\n", prev_token_line);
                parse_err = 1; parse_error_total++;
            }
        }
        if (w == ASSIGN) {
            w = gettoken();  /* 跳过= */
            init_expr = Exp(COMMA);
        }
        AstNode *decl;
        if (is_array)
            decl = ast_new_array_decl(first_name, arr_size, init_expr);
        else
            decl = ast_new_var_decl(first_name, init_expr);
        first = decl;
        last = decl;
    }

    /* 处理逗号分隔的后续变量 */
    while (w == COMMA && !parse_err) {
        w = gettoken();  /* 跳过逗号 */
        if (w != IDENT) {
            printf("[语法错误 line:%d] 变量声明中期望标识符，实际是 '%s'\n",
                   token_line, token_name(w));
            parse_err = 1; parse_error_total++;
            break;
        }
        char name[128];
        strncpy(name, token_text, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        w = gettoken();

        AstNode *init_expr = NULL;
        int arr_size = 0;
        int is_array = 0;
        if (w == LSQUARE) {
            /* 数组声明 a[10] */
            is_array = 1;
            w = gettoken();  /* 跳过[ */
            if (w == INT_CONST || w == LONG_CONST) {
                arr_size = atoi(token_text);
                w = gettoken();
            } else {
                printf("[语法错误 line:%d] 数组大小需要整型常量\n", token_line);
                parse_err = 1; parse_error_total++;
            }
            if (w == RSQUARE) {
                w = gettoken();  /* 跳过] */
            } else {
                printf("[语法错误 line:%d] 数组声明缺少右中括号\n", prev_token_line);
                parse_err = 1; parse_error_total++;
            }
        }
        if (w == ASSIGN) {
            w = gettoken();
            init_expr = Exp(COMMA);
        }

        AstNode *decl;
        if (is_array)
            decl = ast_new_array_decl(name, arr_size, init_expr);
        else
            decl = ast_new_var_decl(name, init_expr);
        if (!first) first = decl;
        else last->next_sibling = decl;
        last = decl; 
    }

    return ast_new_var_list(first);
}

AstNode* ParamList(void)
{
    if (w == RP) {
        return ast_new_param_list(NULL);  /* 无参数 */
    }

    AstNode *first = NULL;
    AstNode *last = NULL;

    while (w != RP && w != TOKEN_EOF && !parse_err) {
        if (!is_type_keyword(w)) {
            printf("[语法错误 line:%d] 形参需要类型，实际是 '%s'\n", token_line, token_name(w));
            parse_err = 1; parse_error_total++;
            break;
        }
        int param_type = w;
        AstNode *type_node = ast_new_type(w);
        w = gettoken();

        /* 处理 void 作为无参数标记：void func(void) */
        if (param_type == KW_VOID && w == RP) {
            ast_free(type_node);
            break;
        }

        if (w != IDENT) {
            printf("[语法错误 line:%d] 形参需要标识符\n", token_line);
            parse_err = 1; parse_error_total++;
            ast_free(type_node);
            break;
        }
        char name[128];
        strncpy(name, token_text, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        w = gettoken();

        AstNode *param = ast_new_param(type_node, name);
        if (!first) first = param;
        else last->next_sibling = param;
        last = param;

        if (w == COMMA) {
            w = gettoken();
        } else {
            break;
        }
    }

    return ast_new_param_list(first);
}

AstNode* Compound(void)
{
    /* 进入时w == LB */
    w = gettoken();  /* 跳过LB */

    AstNode *first_local = NULL;
    AstNode *last_local = NULL;

    /* 解析局部变量声明序列 */
    while (is_type_keyword(w) && !parse_err) {
        AstNode *type_node = ast_new_type(w);
        w = gettoken();

        if (w != IDENT) {
            printf("[语法错误 line:%d] 局部变量声明需要标识符\n", token_line);
            parse_err = 1; parse_error_total++;
            ast_free(type_node);
            break;
        }
        char local_name[128];
        strncpy(local_name, token_text, sizeof(local_name) - 1);
        local_name[sizeof(local_name) - 1] = '\0';
        w = gettoken();

        AstNode *var_list = VarList(local_name);
        if (w != SEMI) {
            printf("[语法错误 line:%d] 局部变量声明缺少分号\n", prev_token_line);
            parse_err = 1; parse_error_total++;
            ast_free(type_node);
            ast_free(var_list);
            break;
        }
        w = gettoken();  /* 跳过分号 */

        AstNode *local_decl = ast_new_ext_var_def(type_node, var_list);
        if (!first_local) first_local = local_decl;
        else last_local->next_sibling = local_decl;
        last_local = local_decl;
        if (parse_err) break;  /* 声明解析出错，停止避免连锁错误 */
    }

    /* 解析语句序列 */
    AstNode *stmt_list = StmtList();

    /* 期望RB */
    if (w != RB) {
        printf("[语法错误 line:%d] 复合语句缺少右大括号，实际是 '%s'\n",
               prev_token_line, token_name(w));
        parse_err = 1; parse_error_total++;
    } else {
        w = gettoken();  /* 跳过RB */
    }

    return ast_new_compound(first_local, stmt_list);
}


AstNode* StmtList(void)
{
    AstNode *first = NULL;
    AstNode *last = NULL;

    while (w != RB && w != TOKEN_EOF) {
        AstNode *stmt = Statement();
        if (stmt) {
            if (parse_err) {
                /* 语句返回了但内部有错误(如if-else的else子句失败)，进行错误恢复 */
                sync_recover();
            }
            if (!first) first = stmt;
            else last->next_sibling = stmt;
            last = stmt;
        } else if (parse_err) {
            /* 错误恢复：跳过本语句，继续分析后面的语句 */
            sync_recover();
        } else {
            break;
        }
    }

    return ast_new_stmt_list(first);
}

 /* 解析一条语句（if/while/for/return/break/continue/表达式/复合语句）
   根据w的类型判断语句种类*/
AstNode* Statement(void)
{
    if (parse_err) return NULL;  /* 前面已有错误，直接返回，避免级联报错 */
    switch (w) {
        case KW_IF: {
            w = gettoken();  /* 跳过if */
            if (w != LP) {
                printf("[语法错误 line:%d] if语句缺少左括号\n", token_line);
                parse_err = 1; parse_error_total++;
                return NULL;
            }
            w = gettoken();  /* 跳过LP */
            AstNode *cond = Exp(RP);
            if (w != RP) {
                printf("[语法错误 line:%d] if条件缺少右括号\n", prev_token_line);
                parse_err = 1; parse_error_total++;
                ast_free(cond);
                return NULL;
            }
            w = gettoken();  /* 跳过RP */
			while (w == COMMENT) w = gettoken();  /* 跳过if条件和子句之间的注释 */
            AstNode *then_stmt = Statement();
            if (!then_stmt) return NULL;
            
            while (w == COMMENT) w = gettoken();  /* 跳过if子句和else之间的注释 */
            if (w == KW_ELSE) {
                w = gettoken();  /* 跳过else */
                AstNode *else_stmt = Statement();
                if (!else_stmt) return NULL;  /* else子句解析失败 */
                return ast_new_if_else(cond, then_stmt, else_stmt);
            }
            return ast_new_if(cond, then_stmt);
        }

        case KW_WHILE: {
            w = gettoken();
            if (w != LP) {
                printf("[语法错误 line:%d] while语句缺少左括号\n", token_line);
                parse_err = 1; parse_error_total++;
                return NULL;
            }
            w = gettoken();
            AstNode *cond = Exp(RP);
            if (w != RP) {
                printf("[语法错误 line:%d] while条件缺少右括号\n", prev_token_line);
                parse_err = 1; parse_error_total++;
                ast_free(cond);
                return NULL;
            }
            w = gettoken();
            while (w == COMMENT) w = gettoken();  /* 跳过while条件和循环体之间的注释 */
            AstNode *body = Statement();
            if (!body) return NULL;  /* 循环体解析失败 */
            return ast_new_while(cond, body);
        }

        case KW_FOR: {
            w = gettoken();
            if (w != LP) {
                printf("[语法错误 line:%d] for语句缺少左括号\n", token_line);
                parse_err = 1; parse_error_total++;
                return NULL;
            }
            w = gettoken();
            /* init表达式 */
            AstNode *init = (w == SEMI) ? ast_new_empty() : Exp(SEMI);
            if (w != SEMI) {
                printf("[语法错误 line:%d] for语句第一个分号缺失\n", token_line);
                parse_err = 1; parse_error_total++;
                return NULL;
            }
            w = gettoken();
            /* cond表达式 */
            AstNode *cond = (w == SEMI) ? ast_new_empty() : Exp(SEMI);
            if (w != SEMI) {
                printf("[语法错误 line:%d] for语句第二个分号缺失\n", token_line);
                parse_err = 1; parse_error_total++;
                return NULL;
            }
            w = gettoken();
            /* step表达式 */
            AstNode *step = (w == RP) ? ast_new_empty() : Exp(RP);
            if (w != RP) {
                printf("[语法错误 line:%d] for语句缺少右括号\n", prev_token_line);
                parse_err = 1; parse_error_total++;
                return NULL;
            }
            w = gettoken();
            while (w == COMMENT) w = gettoken();  /* 跳过for条件和循环体之间的注释 */
            AstNode *body = Statement();
            if (!body) return NULL;  /* 循环体解析失败 */
            return ast_new_for(init, cond, step, body);
        }

        case KW_RETURN: {
            w = gettoken();
            AstNode *expr = NULL;
            if (w != SEMI) {
                expr = Exp(SEMI);
            } else {
                expr = ast_new_empty();
            }
            if (w != SEMI) {
                printf("[语法错误 line:%d] return语句缺少分号\n", prev_token_line);
                parse_err = 1; parse_error_total++;
                ast_free(expr);
                return NULL;
            }
            w = gettoken();
            return ast_new_return(expr);
        }

        case KW_BREAK: {
            w = gettoken();
            if (w != SEMI) {
                printf("[语法错误 line:%d] break语句缺少分号\n", prev_token_line);
                parse_err = 1; parse_error_total++;
                return NULL;
            }
            w = gettoken();
            return ast_new_break();
        }

        case KW_CONTINUE: {
            w = gettoken();
            if (w != SEMI) {
                printf("[语法错误 line:%d] continue语句缺少分号\n", prev_token_line);
                parse_err = 1; parse_error_total++;
                return NULL;
            }
            w = gettoken();
            return ast_new_continue();
        }

        case LB: {
            return Compound();
        }

		case SEMI: {
		            /* 空语句 */
		            w = gettoken();
		            return ast_new_expr_stmt(ast_new_empty());
		        }
		case PRE_INCLUDE: {
		            AstNode *node = ast_new_include(token_text);
		            w = gettoken();
		            return node;
		        }
		case PRE_DEFINE: {
		            AstNode *node = ast_new_define(token_text);
		            w = gettoken();
		            return node;
		        }
		case COMMENT: {
		            AstNode *node = ast_new_comment(token_text);
		            w = gettoken();
		            return node;
		        }
        default: {
            /* 孤立的else：前面if语句的else子句未被消费，跳过else和后面的语句 */
            if (w == KW_ELSE) {
    			printf("[语法错误 line:%d] else没有对应的if语句\n", token_line);
    			parse_err = 1;
			    parse_error_total++;
			    w = gettoken();      /* 跳过else */
			    Statement();          /* 跳过else子句(忽略返回值) */
			    return ast_new_empty();  /* 返回空结点，避免影响上层分析 */
				}
            /* 表达式语句 */
            if (is_operand_start(w) || w == LP) {
                AstNode *expr = Exp(SEMI);
                if (!expr) {
                    if (parse_err) {
                        printf("[语法错误 line:%d] 表达式语句缺少分号，实际是 '%s'\n",
                               prev_token_line, token_name(w));
                        parse_error_total++;
                    }
                    return NULL;
                }
                if (w != SEMI) {
                    printf("[语法错误 line:%d] 表达式语句缺少分号，实际是 '%s'\n",
                           prev_token_line, token_name(w));
                    parse_err = 1; parse_error_total++;
                    ast_free(expr);
                    return NULL;
                }
                w = gettoken();
                return ast_new_expr_stmt(expr);
            }
            printf("[语法错误 line:%d] 未知语句开头 '%s'\n", token_line, token_name(w));
            parse_err = 1; parse_error_total++;
            return NULL;
        }
    }
}
