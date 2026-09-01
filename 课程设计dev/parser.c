#include "parser.h"
#include "lex.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ===== 全局变量定义 ===== */
int w;
int parse_err = 0;

/* ================================================================
 * ===== 表达式分析：运算符栈 + 操作数栈 + 优先关系矩阵 =====
 * ================================================================ */

#define STACK_SIZE 256

 /* 表达式分析上下文（栈改为局部，避免递归调用时互相覆盖） */
typedef struct {
    AstNode* operand_stack[STACK_SIZE];
    int      op_stack[STACK_SIZE];
    int      operand_top;
    int      op_top;
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
static void reduce(ExprContext* ctx)
{
    if (ctx->op_top <= 1) return;  /* 栈底标记不弹出 */
    int op = ctx->op_stack[--ctx->op_top];

    if (op == NOT) {
        /* 单目运算符 */
        if (ctx->operand_top < 1) { parse_err = 1; return; }
        AstNode* operand = ctx->operand_stack[--ctx->operand_top];
        ctx->operand_stack[ctx->operand_top++] = ast_new_unary_op(op, operand);
    }
    else {
        /* 双目运算符 */
        if (ctx->operand_top < 2) { parse_err = 1; return; }
        AstNode* right = ctx->operand_stack[--ctx->operand_top];
        AstNode* left = ctx->operand_stack[--ctx->operand_top];
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
    return (tk == IDENT || tk == INT_CONST || tk == FLOAT_CONST ||
        tk == CHAR_CONST || tk == STRING_CONST || tk == MINUS || tk == NOT);
}

/* 解析函数调用参数列表（进入时w为LP后第一个token） */
static AstNode* parse_arg_list(void)
{
    AstNode* first = NULL;
    AstNode* last = NULL;

    if (w == RP) {
        return ast_new_arg_list(NULL);  /* 无参数 */
    }

    while (w != RP && w != TOKEN_EOF && !parse_err) {
        AstNode* arg = Exp(COMMA);  /* 解析到逗号 */
        if (!arg) break;
        if (!first) first = arg;
        else last->next_sibling = arg;
        last = arg;

        if (w == COMMA) {
            w = gettoken();  /* 跳过逗号 */
        }
        else {
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
            AstNode* arg_list = parse_arg_list();
            if (w == RP) {
                w = gettoken();  /* 跳过RP */
            }
            else {
                printf("[语法错误 line:%d] 函数调用缺少右括号\n", line_no);
                parse_err = 1;
            }
            return ast_new_func_call(name, arg_list);
        }
        /* 普通标识符，w已指向下一个token */
        return ast_new_ident(name);
    }

    if (w == INT_CONST) {
        long v = atol(token_text);
        w = gettoken();
        return ast_new_int_const(v);
    }

    if (w == FLOAT_CONST) {
        double v = atof(token_text);
        w = gettoken();
        return ast_new_float_const(v);
    }

    if (w == CHAR_CONST) {
        AstNode* n = ast_new_char_const(token_text);
        w = gettoken();
        return n;
    }

    if (w == STRING_CONST) {
        AstNode* n = ast_new_string_const(token_text);
        w = gettoken();
        return n;
    }

    if (w == MINUS || w == NOT) {
        /* 单目运算符 */
        int op = w;
        w = gettoken();
        AstNode* operand = parse_operand();
        return ast_new_unary_op(op, operand);
    }

    printf("[语法错误 line:%d] 意外的token '%s'，期望操作数\n", line_no, token_text);
    parse_err = 1;
    return NULL;
}

/*
 * 表达式分析：运算符优先分析法
 * endsym: 表达式结束符号（如SEMI、RP、COMMA）
 * 遇到 endsym / RP / TOKEN_EOF / RB / SEMI 时停止
 */
AstNode* Exp(int endsym)
{
    /* 使用局部栈，避免递归调用（括号表达式、函数参数）时互相覆盖 */
    ExprContext ctx;
    ctx.operand_top = 0;
    ctx.op_top = 0;
    ctx.op_stack[ctx.op_top++] = TOKEN_EOF;  /* 栈底标记 */

    while (!parse_err) {
        /* 停止条件：注意RP不在此列，RP由主循环处理括号归约
         * 只有当endsym本身就是RP时（如if条件），w==endsym会触发停止 */
        if (w == endsym || w == TOKEN_EOF || w == RB || w == SEMI) {
            break;
        }

        if (w == LP) {
            /* 左括号：直接入运算符栈 */
            if (ctx.op_top >= STACK_SIZE) {
                printf("[语法错误] 表达式过于复杂\n");
                parse_err = 1;
                break;
            }
            ctx.op_stack[ctx.op_top++] = LP;
            w = gettoken();
        }
        else if (w == RP) {
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
                    printf("[语法错误 line:%d] 缺少左括号\n", line_no);
                    parse_err = 1;
                    break;
                }
                reduce(&ctx);
                if (parse_err) break;
            }
            if (parse_err) break;
            ctx.op_top--;  /* 弹出LP */
            w = gettoken();
        }
        else if (is_operand_start(w)) {
            /* MINUS/NOT 在操作数栈非空时是双目运算符，不是单目 */
            if ((w == MINUS || w == NOT) && ctx.operand_top > 0) {
                while (precede(ctx.op_stack[ctx.op_top - 1], w) == '>') {
                    reduce(&ctx);
                    if (parse_err) break;
                }
                if (parse_err) break;
                if (ctx.op_top >= STACK_SIZE) {
                    printf("[语法错误] 表达式过于复杂\n");
                    parse_err = 1;
                    break;
                }
                ctx.op_stack[ctx.op_top++] = w;
                w = gettoken();
            }
            else {
                AstNode* operand = parse_operand();
                if (!operand) break;
                if (ctx.operand_top >= STACK_SIZE) {
                    printf("[语法错误] 表达式过于复杂\n");
                    parse_err = 1;
                    break;
                }
                ctx.operand_stack[ctx.operand_top++] = operand;
            }
        }
        else if (is_operator(w)) {
            while (precede(ctx.op_stack[ctx.op_top - 1], w) == '>') {
                reduce(&ctx);
                if (parse_err) break;
            }
            if (parse_err) break;
            if (ctx.op_top >= STACK_SIZE) {
                printf("[语法错误] 表达式过于复杂\n");
                parse_err = 1;
                break;
            }
            ctx.op_stack[ctx.op_top++] = w;
            w = gettoken();
        }
        else {
            printf("[语法错误 line:%d] 表达式中出现意外token '%s'\n", line_no, token_text);
            parse_err = 1;
            break;
        }
    }

    /* 归约剩余运算符 */
    while (!parse_err && ctx.op_stack[ctx.op_top - 1] != TOKEN_EOF) {
        if (ctx.op_stack[ctx.op_top - 1] == LP) {
            printf("[语法错误 line:%d] 缺少右括号\n", line_no);
            parse_err = 1;
            break;
        }
        reduce(&ctx);
    }

    if (parse_err || ctx.operand_top == 0) {
        return NULL;
    }

    return ctx.operand_stack[ctx.operand_top - 1];
}

/* ================================================================
 * ===== 递归下降语法分析 =====
 * ================================================================ */

 /* <程序> ::= <外部定义序列> */
AstNode* Program(void)
{
    w = gettoken();
    AstNode* ext_list = ExtDefList();
    if (parse_err) {
        ast_free(ext_list);
        return NULL;
    }
    return ast_new_prog(ext_list);
}

/* <外部定义序列> ::= <外部定义> <外部定义序列> | ε */
AstNode* ExtDefList(void)
{
    if (w == TOKEN_EOF) return NULL;

    AstNode* def = ExtDef();
    if (!def) return NULL;

    AstNode* next = ExtDefList();
    if (next) {
        ast_add_sibling(def, next);
    }
    return ast_new_ext_def_list(def);
}

/* 判断是否为类型关键字 */
static int is_type_keyword(int tk)
{
    return tk == KW_INT || tk == KW_FLOAT || tk == KW_CHAR || tk == KW_VOID;
}

/* <外部定义> ::= <类型> <标识符> (<变量序列>; | <函数定义>) */
AstNode* ExtDef(void)
{
    if (!is_type_keyword(w)) {
        printf("[语法错误 line:%d] 外部定义需要以类型关键字开头，实际是 '%s'\n",
            line_no, token_name(w));
        parse_err = 1;
        return NULL;
    }

    AstNode* type_node = ast_new_type(w);
    w = gettoken();

    if (w != IDENT) {
        printf("[语法错误 line:%d] 期望标识符，实际是 '%s'\n", line_no, token_name(w));
        parse_err = 1;
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
        AstNode* param_list = ParamList();
        if (w != RP) {
            printf("[语法错误 line:%d] 函数定义缺少右括号\n", line_no);
            parse_err = 1;
            ast_free(type_node);
            ast_free(param_list);
            return NULL;
        }
        w = gettoken();  /* 跳过RP */

        if (w != LB) {
            printf("[语法错误 line:%d] 函数定义缺少左大括号，实际是 '%s'\n",
                line_no, token_name(w));
            parse_err = 1;
            ast_free(type_node);
            ast_free(param_list);
            return NULL;
        }

        AstNode* body = Compound();
        return ast_new_func_def(type_node, name, param_list, body);
    }
    else {
        /* 外部变量定义：name已保存，w是name后面的token */
        AstNode* var_list = VarList(name);
        if (w != SEMI) {
            printf("[语法错误 line:%d] 外部变量定义缺少分号，实际是 '%s'\n",
                line_no, token_name(w));
            parse_err = 1;
            ast_free(type_node);
            ast_free(var_list);
            return NULL;
        }
        w = gettoken();  /* 跳过分号 */
        return ast_new_ext_var_def(type_node, var_list);
    }
}

/* <变量声明序列> ::= <变量声明>(,<变量声明>)*
 * first_name: 调用者已经预读的第一个变量名（不为NULL时先处理它）
 * 进入时：如果first_name不为NULL，w是该变量名后面的token；
 *         如果first_name为NULL，w应该是IDENT */
AstNode* VarList(const char* first_name)
{
    AstNode* first = NULL;
    AstNode* last = NULL;

    /* 处理第一个变量 */
    if (first_name) {
        AstNode* init_expr = NULL;
        if (w == ASSIGN) {
            w = gettoken();  /* 跳过= */
            init_expr = Exp(COMMA);
        }
        AstNode* decl = ast_new_var_decl(first_name, init_expr);
        first = decl;
        last = decl;
    }

    /* 处理逗号分隔的后续变量 */
    while (w == COMMA && !parse_err) {
        w = gettoken();  /* 跳过逗号 */
        if (w != IDENT) {
            printf("[语法错误 line:%d] 变量声明中期望标识符，实际是 '%s'\n",
                line_no, token_name(w));
            parse_err = 1;
            break;
        }
        char name[128];
        strncpy(name, token_text, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        w = gettoken();

        AstNode* init_expr = NULL;
        if (w == ASSIGN) {
            w = gettoken();
            init_expr = Exp(COMMA);
        }

        AstNode* decl = ast_new_var_decl(name, init_expr);
        if (!first) first = decl;
        else last->next_sibling = decl;
        last = decl;
    }

    return ast_new_var_list(first);
}

/* <形参列表> ::= <形参>(,<形参>)* | ε */
AstNode* ParamList(void)
{
    if (w == RP) {
        return ast_new_param_list(NULL);  /* 无参数 */
    }

    AstNode* first = NULL;
    AstNode* last = NULL;

    while (w != RP && w != TOKEN_EOF && !parse_err) {
        if (!is_type_keyword(w)) {
            printf("[语法错误 line:%d] 形参需要类型，实际是 '%s'\n", line_no, token_name(w));
            parse_err = 1;
            break;
        }
        int param_type = w;
        AstNode* type_node = ast_new_type(w);
        w = gettoken();

        /* 处理 void 作为无参数标记：void func(void) */
        if (param_type == KW_VOID && w == RP) {
            ast_free(type_node);
            break;
        }

        if (w != IDENT) {
            printf("[语法错误 line:%d] 形参需要标识符\n", line_no);
            parse_err = 1;
            ast_free(type_node);
            break;
        }
        char name[128];
        strncpy(name, token_text, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        w = gettoken();

        AstNode* param = ast_new_param(type_node, name);
        if (!first) first = param;
        else last->next_sibling = param;
        last = param;

        if (w == COMMA) {
            w = gettoken();
        }
        else {
            break;
        }
    }

    return ast_new_param_list(first);
}

/* <复合语句> ::= { <局部声明序列> <语句序列> } */
AstNode* Compound(void)
{
    /* 进入时w == LB */
    w = gettoken();  /* 跳过LB */

    AstNode* first_local = NULL;
    AstNode* last_local = NULL;

    /* 解析局部变量声明序列 */
    while (is_type_keyword(w) && !parse_err) {
        AstNode* type_node = ast_new_type(w);
        w = gettoken();

        if (w != IDENT) {
            printf("[语法错误 line:%d] 局部变量声明需要标识符\n", line_no);
            parse_err = 1;
            ast_free(type_node);
            break;
        }
        char local_name[128];
        strncpy(local_name, token_text, sizeof(local_name) - 1);
        local_name[sizeof(local_name) - 1] = '\0';
        w = gettoken();

        AstNode* var_list = VarList(local_name);
        if (w != SEMI) {
            printf("[语法错误 line:%d] 局部变量声明缺少分号\n", line_no);
            parse_err = 1;
            ast_free(type_node);
            ast_free(var_list);
            break;
        }
        w = gettoken();  /* 跳过分号 */

        AstNode* local_decl = ast_new_ext_var_def(type_node, var_list);
        if (!first_local) first_local = local_decl;
        else last_local->next_sibling = local_decl;
        last_local = local_decl;
    }

    /* 解析语句序列 */
    AstNode* stmt_list = StmtList();

    /* 期望RB */
    if (w != RB) {
        printf("[语法错误 line:%d] 复合语句缺少右大括号，实际是 '%s'\n",
            line_no, token_name(w));
        parse_err = 1;
    }
    else {
        w = gettoken();  /* 跳过RB */
    }

    return ast_new_compound(first_local, stmt_list);
}

/* <语句序列> ::= <语句>* */
AstNode* StmtList(void)
{
    AstNode* first = NULL;
    AstNode* last = NULL;

    while (w != RB && w != TOKEN_EOF && !parse_err) {
        AstNode* stmt = Statement();
        if (!stmt) break;
        if (!first) first = stmt;
        else last->next_sibling = stmt;
        last = stmt;
    }

    return ast_new_stmt_list(first);
}

/* <语句> ::= 各种语句 */
AstNode* Statement(void)
{
    switch (w) {
    case KW_IF: {
        w = gettoken();  /* 跳过if */
        if (w != LP) {
            printf("[语法错误 line:%d] if语句缺少左括号\n", line_no);
            parse_err = 1;
            return NULL;
        }
        w = gettoken();  /* 跳过LP */
        AstNode* cond = Exp(RP);
        if (w != RP) {
            printf("[语法错误 line:%d] if条件缺少右括号\n", line_no);
            parse_err = 1;
            ast_free(cond);
            return NULL;
        }
        w = gettoken();  /* 跳过RP */

        AstNode* then_stmt = Statement();
        if (!then_stmt) return NULL;

        if (w == KW_ELSE) {
            w = gettoken();  /* 跳过else */
            AstNode* else_stmt = Statement();
            return ast_new_if_else(cond, then_stmt, else_stmt);
        }
        return ast_new_if(cond, then_stmt);
    }

    case KW_WHILE: {
        w = gettoken();
        if (w != LP) {
            printf("[语法错误 line:%d] while语句缺少左括号\n", line_no);
            parse_err = 1;
            return NULL;
        }
        w = gettoken();
        AstNode* cond = Exp(RP);
        if (w != RP) {
            printf("[语法错误 line:%d] while条件缺少右括号\n", line_no);
            parse_err = 1;
            ast_free(cond);
            return NULL;
        }
        w = gettoken();
        AstNode* body = Statement();
        return ast_new_while(cond, body);
    }

    case KW_FOR: {
        w = gettoken();
        if (w != LP) {
            printf("[语法错误 line:%d] for语句缺少左括号\n", line_no);
            parse_err = 1;
            return NULL;
        }
        w = gettoken();
        /* init表达式 */
        AstNode* init = (w == SEMI) ? ast_new_empty() : Exp(SEMI);
        if (w != SEMI) {
            printf("[语法错误 line:%d] for语句第一个分号缺失\n", line_no);
            parse_err = 1;
            return NULL;
        }
        w = gettoken();
        /* cond表达式 */
        AstNode* cond = (w == SEMI) ? ast_new_empty() : Exp(SEMI);
        if (w != SEMI) {
            printf("[语法错误 line:%d] for语句第二个分号缺失\n", line_no);
            parse_err = 1;
            return NULL;
        }
        w = gettoken();
        /* step表达式 */
        AstNode* step = (w == RP) ? ast_new_empty() : Exp(RP);
        if (w != RP) {
            printf("[语法错误 line:%d] for语句缺少右括号\n", line_no);
            parse_err = 1;
            return NULL;
        }
        w = gettoken();
        AstNode* body = Statement();
        return ast_new_for(init, cond, step, body);
    }

    case KW_RETURN: {
        w = gettoken();
        AstNode* expr = NULL;
        if (w != SEMI) {
            expr = Exp(SEMI);
        }
        else {
            expr = ast_new_empty();
        }
        if (w != SEMI) {
            printf("[语法错误 line:%d] return语句缺少分号\n", line_no);
            parse_err = 1;
            ast_free(expr);
            return NULL;
        }
        w = gettoken();
        return ast_new_return(expr);
    }

    case KW_BREAK: {
        w = gettoken();
        if (w != SEMI) {
            printf("[语法错误 line:%d] break语句缺少分号\n", line_no);
            parse_err = 1;
            return NULL;
        }
        w = gettoken();
        return ast_new_break();
    }

    case KW_CONTINUE: {
        w = gettoken();
        if (w != SEMI) {
            printf("[语法错误 line:%d] continue语句缺少分号\n", line_no);
            parse_err = 1;
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

    default: {
        /* 表达式语句 */
        if (is_operand_start(w) || w == LP) {
            AstNode* expr = Exp(SEMI);
            if (!expr) return NULL;
            if (w != SEMI) {
                printf("[语法错误 line:%d] 表达式语句缺少分号，实际是 '%s'\n",
                    line_no, token_name(w));
                parse_err = 1;
                ast_free(expr);
                return NULL;
            }
            w = gettoken();
            return ast_new_expr_stmt(expr);
        }
        printf("[语法错误 line:%d] 未知语句开头 '%s'\n", line_no, token_name(w));
        parse_err = 1;
        return NULL;
    }
    }
}
