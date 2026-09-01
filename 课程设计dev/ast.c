#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 内部工具：分配结点 ===== */
static AstNode* alloc_node(AstNodeType t)
{
    AstNode *p = (AstNode*)malloc(sizeof(AstNode));
    if (!p) { fprintf(stderr, "内存分配失败\n"); exit(1); }
    p->type = t;
    memset(&p->u, 0, sizeof(p->u));
    p->first_child = NULL;
    p->next_sibling = NULL;
    return p;
}

/* ===== 结点创建函数 ===== */
AstNode* ast_new_prog(AstNode *ext_list)
{
    AstNode *n = alloc_node(AST_PROG);
    n->first_child = ext_list;
    return n;
}

AstNode* ast_new_ext_def_list(AstNode *first)
{
    AstNode *n = alloc_node(AST_EXT_DEF_LIST);
    n->first_child = first;
    return n;
}

AstNode* ast_new_ext_var_def(AstNode *type_node, AstNode *var_list)
{
    AstNode *n = alloc_node(AST_EXT_VAR_DEF);
    n->first_child = type_node;
    if (type_node) type_node->next_sibling = var_list;
    return n;
}

AstNode* ast_new_func_def(AstNode *ret_type, const char *name,
                           AstNode *param_list, AstNode *body)
{
    AstNode *n = alloc_node(AST_FUNC_DEF);
    strncpy(n->u.id_name, name, sizeof(n->u.id_name) - 1);
    n->first_child = ret_type;
    if (ret_type) ret_type->next_sibling = param_list;
    if (param_list) param_list->next_sibling = body;
    return n;
}

AstNode* ast_new_func_decl(AstNode *ret_type, const char *name,
                           AstNode *param_list)
{
    AstNode *n = alloc_node(AST_FUNC_DECL);
    strncpy(n->u.id_name, name, sizeof(n->u.id_name) - 1);
    n->first_child = ret_type;
    if (ret_type) ret_type->next_sibling = param_list;
    return n;
}

AstNode* ast_new_param_list(AstNode *first)
{
    AstNode *n = alloc_node(AST_PARAM_LIST);
    n->first_child = first;
    return n;
}

AstNode* ast_new_param(AstNode *type_node, const char *name)
{
    AstNode *n = alloc_node(AST_PARAM);
    strncpy(n->u.id_name, name, sizeof(n->u.id_name) - 1);
    n->first_child = type_node;
    return n;
}

AstNode* ast_new_var_list(AstNode *first)
{
    AstNode *n = alloc_node(AST_VAR_LIST);
    n->first_child = first;
    return n;
}

AstNode* ast_new_var_decl(const char *name, AstNode *init_expr)
{
    AstNode *n = alloc_node(AST_VAR_DECL);
    strncpy(n->u.id_name, name, sizeof(n->u.id_name) - 1);
    n->first_child = init_expr;
    return n;
}

AstNode* ast_new_compound(AstNode *local_decls, AstNode *stmt_list)
{
    AstNode *n = alloc_node(AST_COMPOUND);
    if (local_decls) {
        n->first_child = local_decls;
        /* 找到最后一个局部声明，链接语句序列 */
        AstNode *p = local_decls;
        while (p->next_sibling) p = p->next_sibling;
        p->next_sibling = stmt_list;
    } else {
        n->first_child = stmt_list;
    }
    return n;
}

AstNode* ast_new_stmt_list(AstNode *first)
{
    AstNode *n = alloc_node(AST_STMT_LIST);
    n->first_child = first;
    return n;
}

AstNode* ast_new_if(AstNode *cond, AstNode *then_stmt)
{
    AstNode *n = alloc_node(AST_IF);
    n->first_child = cond;
    cond->next_sibling = then_stmt;
    return n;
}

AstNode* ast_new_if_else(AstNode *cond, AstNode *then_stmt, AstNode *else_stmt)
{
    AstNode *n = alloc_node(AST_IF_ELSE);
    n->first_child = cond;
    cond->next_sibling = then_stmt;
    then_stmt->next_sibling = else_stmt;
    return n;
}

AstNode* ast_new_while(AstNode *cond, AstNode *body)
{
    AstNode *n = alloc_node(AST_WHILE);
    n->first_child = cond;
    cond->next_sibling = body;
    return n;
}

AstNode* ast_new_for(AstNode *init, AstNode *cond, AstNode *step, AstNode *body)
{
    AstNode *n = alloc_node(AST_FOR);
    n->first_child = init;
    if (init) init->next_sibling = cond;
    if (cond) cond->next_sibling = step;
    if (step) step->next_sibling = body;
    return n;
}

AstNode* ast_new_return(AstNode *expr)
{
    AstNode *n = alloc_node(AST_RETURN);
    n->first_child = expr;
    return n;
}

AstNode* ast_new_break(void)
{
    return alloc_node(AST_BREAK);
}

AstNode* ast_new_continue(void)
{
    return alloc_node(AST_CONTINUE);
}

AstNode* ast_new_expr_stmt(AstNode *expr)
{
    AstNode *n = alloc_node(AST_EXPR_STMT);
    n->first_child = expr;
    return n;
}

AstNode* ast_new_expr_op(int op, AstNode *left, AstNode *right)
{
    AstNode *n = alloc_node(AST_EXPR_OP);
    n->u.op_kind = op;
    n->first_child = left;
    if (left) left->next_sibling = right;
    return n;
}

AstNode* ast_new_unary_op(int op, AstNode *operand)
{
    AstNode *n = alloc_node(AST_UNARY_OP);
    n->u.op_kind = op;
    n->first_child = operand;
    return n;
}

AstNode* ast_new_func_call(const char *name, AstNode *arg_list)
{
    AstNode *n = alloc_node(AST_FUNC_CALL);
    strncpy(n->u.id_name, name, sizeof(n->u.id_name) - 1);
    n->first_child = arg_list;
    return n;
}

AstNode* ast_new_array_decl(const char *name, int size, AstNode *init_expr)
{
    AstNode *n = alloc_node(AST_ARRAY_DECL);
    strncpy(n->u.id_name, name, sizeof(n->u.id_name) - 1);
    n->array_size = size;
    n->first_child = init_expr;
    return n;
}

AstNode* ast_new_array_access(const char *name, AstNode *index)
{
    AstNode *n = alloc_node(AST_ARRAY_ACCESS);
    strncpy(n->u.id_name, name, sizeof(n->u.id_name) - 1);
    n->first_child = index;
    return n;
}

AstNode* ast_new_arg_list(AstNode *first)
{
    AstNode *n = alloc_node(AST_ARG_LIST);
    n->first_child = first;
    return n;
}

AstNode* ast_new_ident(const char *name)
{
    AstNode *n = alloc_node(AST_IDENT);
    strncpy(n->u.id_name, name, sizeof(n->u.id_name) - 1);
    return n;
}

AstNode* ast_new_int_const(long v)
{
    AstNode *n = alloc_node(AST_INT_CONST);
    n->u.int_val = v;
    return n;
}

AstNode* ast_new_float_const(double v)
{
    AstNode *n = alloc_node(AST_FLOAT_CONST);
    n->u.float_val = v;
    return n;
}

AstNode* ast_new_char_const(const char *text)
{
    AstNode *n = alloc_node(AST_CHAR_CONST);
    strncpy(n->u.char_val, text, sizeof(n->u.char_val) - 1);
    return n;
}

AstNode* ast_new_string_const(const char *text)
{
    AstNode *n = alloc_node(AST_STRING_CONST);
    strncpy(n->u.str_val, text, sizeof(n->u.str_val) - 1);
    return n;
}

AstNode* ast_new_type(int tk)
{
    AstNode *n = alloc_node(AST_TYPE);
    n->u.type_kind = tk;
    return n;
}

AstNode* ast_new_empty(void)
{
    return alloc_node(AST_EMPTY);
}

/* ===== 添加兄弟结点 ===== */
void ast_add_sibling(AstNode *node, AstNode *sib)
{
    if (!node || !sib) return;
    while (node->next_sibling) node = node->next_sibling;
    node->next_sibling = sib;
}

/* ===== 打印缩进 ===== */
static void print_ind(int n)
{
    for (int i = 0; i < n; i++) printf("  ");
}

/* ===== 先根遍历打印AST（任务书示例格式） ===== */
static void print_type_name(AstNode *type_node)
{
    if (!type_node) return;
    switch (type_node->u.type_kind) {
        case KW_INT:   printf("int"); break;
        case KW_FLOAT: printf("float"); break;
        case KW_CHAR:  printf("char"); break;
        case KW_LONG:  printf("long"); break;
        case KW_VOID:  printf("void"); break;
        default:        printf("unknown"); break;
    }
}

static void print_expr(AstNode *expr, int indent)
{
    if (!expr) return;
    print_ind(indent);
    switch (expr->type) {
        case AST_EXPR_OP: {
            switch (expr->u.op_kind) {
                case ASSIGN: printf("ASSIGNOP\n"); break;
                case PLUS:   printf("+\n"); break;
                case MINUS:  printf("-\n"); break;
                case MUL:    printf("*\n"); break;
                case DIV:    printf("/\n"); break;
                case MOD:    printf("%%\n"); break;
                case EQ:     printf("==\n"); break;
                case NEQ:    printf("!=\n"); break;
                case GT:     printf(">\n"); break;
                case GE:     printf(">=\n"); break;
                case LT:     printf("<\n"); break;
                case LE:     printf("<=\n"); break;
                case AND:    printf("&&\n"); break;
                case OR:     printf("||\n"); break;
                default:      printf("OP\n"); break;
            }
            print_expr(expr->first_child, indent + 1);
            print_expr(expr->first_child ? expr->first_child->next_sibling : NULL, indent + 1);
            break;
        }
        case AST_IDENT:
            printf("ID：%s\n", expr->u.id_name);
            break;
        case AST_INT_CONST:
            printf("INT：%ld\n", expr->u.int_val);
            break;
        case AST_FLOAT_CONST:
            printf("FLOAT：%g\n", expr->u.float_val);
            break;
        case AST_CHAR_CONST:
            printf("CHAR：%s\n", expr->u.char_val);
            break;
        case AST_STRING_CONST:
            printf("STRING：%s\n", expr->u.str_val);
            break;
        case AST_FUNC_CALL:
            printf("函数调用：%s\n", expr->u.id_name);
            if (expr->first_child && expr->first_child->first_child) {
                print_ind(indent + 1);
                printf("实参：\n");
                AstNode *arg = expr->first_child->first_child;
                while (arg) {
                    print_expr(arg, indent + 2);
                    arg = arg->next_sibling;
                }
            }
            break;
        case AST_ARRAY_ACCESS:
            printf("数组访问：%s\n", expr->u.id_name);
            print_ind(indent + 1);
            printf("下标：\n");
            print_expr(expr->first_child, indent + 2);
            break;
        default:
            printf("表达式\n");
            break;
    }
}

void ast_print(AstNode *root, int indent)
{
    if (!root) return;

    /* 列表结点透明：不打印自身，直接打印内容 */
    if (root->type == AST_PROG ||
        root->type == AST_EXT_DEF_LIST ||
        root->type == AST_VAR_LIST ||
        root->type == AST_PARAM_LIST ||
        root->type == AST_STMT_LIST ||
        root->type == AST_ARG_LIST) {
        AstNode *child = root->first_child;
        while (child) {
            ast_print(child, indent);
            child = child->next_sibling;
        }
        return;
    }

    print_ind(indent);
    switch (root->type) {
        case AST_EXT_VAR_DEF: {
            printf("外部变量定义：\n");
            AstNode *type = root->first_child;
            AstNode *var_list = type ? type->next_sibling : NULL;
            print_ind(indent + 1);
            printf("类型：");
            print_type_name(type);
            printf("\n");
            print_ind(indent + 1);
            printf("变量名：\n");
            if (var_list) {
                AstNode *v = var_list->first_child;
                while (v) {
                    print_ind(indent + 2);
                    if (v->type == AST_ARRAY_DECL) {
                        printf("ID：%s[%d]\n", v->u.id_name, v->array_size);
                    } else {
                        printf("ID：%s\n", v->u.id_name);
                    }
                    if (v->first_child) {
                        print_expr(v->first_child, indent + 3);
                    }
                    v = v->next_sibling;
                }
            }
            break;
        }
        case AST_FUNC_DEF: {
            printf("函数定义：\n");
            AstNode *type = root->first_child;
            AstNode *param_list = type ? type->next_sibling : NULL;
            AstNode *body = param_list ? param_list->next_sibling : NULL;
            print_ind(indent + 1);
            printf("类型：");
            print_type_name(type);
            printf("\n");
            print_ind(indent + 1);
            printf("函数名：%s\n", root->u.id_name);
            print_ind(indent + 1);
            printf("函数形参：\n");
            if (param_list && param_list->first_child) {
                AstNode *p = param_list->first_child;
                while (p) {
                    print_ind(indent + 2);
                    printf("类型：");
                    print_type_name(p->first_child);
                    printf("，参数名：%s\n", p->u.id_name);
                    p = p->next_sibling;
                }
            } else {
                print_ind(indent + 2);
                printf("void\n");
            }
            if (body) ast_print(body, indent + 1);
            break;
        }
        case AST_FUNC_DECL: {
            printf("函数声明：\n");
            AstNode *type = root->first_child;
            AstNode *param_list = type ? type->next_sibling : NULL;
            print_ind(indent + 1);
            printf("类型：");
            print_type_name(type);
            printf("\n");
            print_ind(indent + 1);
            printf("函数名：%s\n", root->u.id_name);
            print_ind(indent + 1);
            printf("函数形参：\n");
            if (param_list && param_list->first_child) {
                AstNode *p = param_list->first_child;
                while (p) {
                    print_ind(indent + 2);
                    printf("类型：");
                    print_type_name(p->first_child);
                    printf("，参数名：%s\n", p->u.id_name);
                    p = p->next_sibling;
                }
            }
            break;
        }
        case AST_COMPOUND: {
            printf("复合语句：\n");
            /* 找到语句列表（局部声明通过next_sibling链接，最后一个的next是stmt_list） */
            AstNode *p = root->first_child;
            AstNode *stmt_list = NULL;
            while (p) {
                if (p->type == AST_STMT_LIST) {
                    stmt_list = p;
                    break;
                }
                p = p->next_sibling;
            }
            /* 打印局部变量声明 */
            if (root->first_child && root->first_child->type != AST_STMT_LIST) {
                print_ind(indent + 1);
                printf("复合语句的变量定义：\n");
                AstNode *decl = root->first_child;
                while (decl && decl->type != AST_STMT_LIST) {
                    print_ind(indent + 2);
                    printf("LOCAL_VAR_NAME：\n");
                    AstNode *type = decl->first_child;
                    AstNode *var_list = type ? type->next_sibling : NULL;
                    print_ind(indent + 3);
                    printf("类型：");
                    print_type_name(type);
                    printf("\n");
                    if (var_list) {
                        AstNode *v = var_list->first_child;
                        while (v) {
                            print_ind(indent + 3);
                            if (v->type == AST_ARRAY_DECL) {
                                printf("ID：%s[%d]\n", v->u.id_name, v->array_size);
                            } else {
                                printf("ID：%s\n", v->u.id_name);
                            }
                            if (v->first_child) {
                                print_expr(v->first_child, indent + 4);
                            }
                            v = v->next_sibling;
                        }
                    }
                    decl = decl->next_sibling;
                }
            }
            print_ind(indent + 1);
            printf("复合语句的语句部分：\n");
            if (stmt_list) {
                AstNode *stmt = stmt_list->first_child;
                while (stmt) {
                    ast_print(stmt, indent + 2);
                    stmt = stmt->next_sibling;
                }
            }
            break;
        }
        case AST_IF_ELSE: {
            printf("条件语句(IF_THEN_ELSE)：\n");
            AstNode *cond = root->first_child;
            AstNode *then_stmt = cond ? cond->next_sibling : NULL;
            AstNode *else_stmt = then_stmt ? then_stmt->next_sibling : NULL;
            print_ind(indent + 1);
            printf("条件：\n");
            print_expr(cond, indent + 2);
            print_ind(indent + 1);
            printf("IF子句：\n");
            if (then_stmt) ast_print(then_stmt, indent + 2);
            print_ind(indent + 1);
            printf("ELSE子句：\n");
            if (else_stmt) ast_print(else_stmt, indent + 2);
            break;
        }
        case AST_IF: {
            printf("条件语句(IF_THEN)：\n");
            AstNode *cond = root->first_child;
            AstNode *then_stmt = cond ? cond->next_sibling : NULL;
            print_ind(indent + 1);
            printf("条件：\n");
            print_expr(cond, indent + 2);
            print_ind(indent + 1);
            printf("IF子句：\n");
            if (then_stmt) ast_print(then_stmt, indent + 2);
            break;
        }
        case AST_WHILE: {
            printf("循环语句(WHILE)：\n");
            AstNode *cond = root->first_child;
            AstNode *body = cond ? cond->next_sibling : NULL;
            print_ind(indent + 1);
            printf("条件：\n");
            print_expr(cond, indent + 2);
            print_ind(indent + 1);
            printf("循环体：\n");
            if (body) ast_print(body, indent + 2);
            break;
        }
        case AST_FOR: {
            printf("循环语句(FOR)：\n");
            AstNode *init = root->first_child;
            AstNode *cond = init ? init->next_sibling : NULL;
            AstNode *step = cond ? cond->next_sibling : NULL;
            AstNode *body = step ? step->next_sibling : NULL;
            print_ind(indent + 1);
            printf("初始化：\n");
            if (init && init->type != AST_EMPTY) print_expr(init, indent + 2);
            print_ind(indent + 1);
            printf("条件：\n");
            if (cond && cond->type != AST_EMPTY) print_expr(cond, indent + 2);
            print_ind(indent + 1);
            printf("增量：\n");
            if (step && step->type != AST_EMPTY) print_expr(step, indent + 2);
            print_ind(indent + 1);
            printf("循环体：\n");
            if (body) ast_print(body, indent + 2);
            break;
        }
        case AST_RETURN: {
            printf("返回语句：\n");
            if (root->first_child) {
                print_expr(root->first_child, indent + 1);
            }
            break;
        }
        case AST_BREAK:
            printf("BREAK语句\n");
            break;
        case AST_CONTINUE:
            printf("CONTINUE语句\n");
            break;
        case AST_EXPR_STMT: {
            printf("表达式语句：\n");
            if (root->first_child) {
                print_expr(root->first_child, indent + 1);
            }
            break;
        }
        default:
            printf("未知结点 type=%d\n", root->type);
            break;
    }
}

/* ===== 释放整棵树 ===== */
void ast_free(AstNode *root)
{
    if (!root) return;
    ast_free(root->first_child);
    ast_free(root->next_sibling);
    free(root);
}

/* ================================================================
 * ===== 格式化输出：遍历AST输出格式化C源码 =====
 * ================================================================ */

static int g_indent = 0;  /* 当前缩进级别 */

static void fp_indent(FILE *fp, int level)
{
    for (int i = 0; i < level; i++) fprintf(fp, "    ");
}

/* 输出类型名 */
static void gen_type(AstNode *type_node, FILE *fp)
{
    if (!type_node) return;
    switch (type_node->u.type_kind) {
        case KW_INT:   fprintf(fp, "int");    break;
        case KW_FLOAT: fprintf(fp, "float");  break;
        case KW_CHAR:  fprintf(fp, "char");   break;
        case KW_VOID:  fprintf(fp, "void");   break;
        default:       fprintf(fp, "int");
    }
}

/* 输出表达式（递归） */
static void gen_expr(AstNode *expr, FILE *fp)
{
    if (!expr) return;
    switch (expr->type) {
        case AST_IDENT:
            fprintf(fp, "%s", expr->u.id_name);
            break;
        case AST_INT_CONST:
            fprintf(fp, "%ld", expr->u.int_val);
            break;
        case AST_FLOAT_CONST:
            fprintf(fp, "%g", expr->u.float_val);
            break;
        case AST_CHAR_CONST:
            fprintf(fp, "%s", expr->u.char_val);
            break;
        case AST_STRING_CONST:
            fprintf(fp, "%s", expr->u.str_val);
            break;
        case AST_EXPR_OP: {
            AstNode *left = expr->first_child;
            AstNode *right = left ? left->next_sibling : NULL;
            /* 左子表达式加括号（如果优先级更低） */
            if (left && left->type == AST_EXPR_OP) {
                fprintf(fp, "(");
                gen_expr(left, fp);
                fprintf(fp, ")");
            } else {
                gen_expr(left, fp);
            }
            /* 运算符 */
            const char *op_str = "";
            switch (expr->u.op_kind) {
                case PLUS:   op_str = " + ";   break;
                case MINUS:  op_str = " - ";   break;
                case MUL:    op_str = " * ";   break;
                case DIV:    op_str = " / ";   break;
                case MOD:    op_str = " %% ";  break;
                case ASSIGN: op_str = " = ";   break;
                case EQ:     op_str = " == ";  break;
                case NEQ:    op_str = " != ";  break;
                case GT:     op_str = " > ";   break;
                case GE:     op_str = " >= ";  break;
                case LT:     op_str = " < ";   break;
                case LE:     op_str = " <= ";  break;
                case AND:    op_str = " && ";  break;
                case OR:     op_str = " || ";  break;
                default:     op_str = " ? ";
            }
            fprintf(fp, "%s", op_str);
            /* 右子表达式 */
            if (right && right->type == AST_EXPR_OP && expr->u.op_kind != ASSIGN) {
                fprintf(fp, "(");
                gen_expr(right, fp);
                fprintf(fp, ")");
            } else {
                gen_expr(right, fp);
            }
            break;
        }
        case AST_UNARY_OP: {
            const char *op_str = "";
            switch (expr->u.op_kind) {
                case NOT:   op_str = "!";  break;
                case MINUS: op_str = "-";  break;
                case PLUS:  op_str = "+";  break;
                default:    op_str = "?";
            }
            fprintf(fp, "%s", op_str);
            gen_expr(expr->first_child, fp);
            break;
        }
        case AST_FUNC_CALL: {
            fprintf(fp, "%s(", expr->u.id_name);
            AstNode *arg_list = expr->first_child;
            if (arg_list && arg_list->first_child) {
                AstNode *arg = arg_list->first_child;
                gen_expr(arg, fp);
                arg = arg->next_sibling;
                while (arg) {
                    fprintf(fp, ", ");
                    gen_expr(arg, fp);
                    arg = arg->next_sibling;
                }
            }
            fprintf(fp, ")");
            break;
        }
        case AST_ARRAY_ACCESS: {
            fprintf(fp, "%s[", expr->u.id_name);
            if (expr->first_child) gen_expr(expr->first_child, fp);
            fprintf(fp, "]");
            break;
        }
        case AST_EMPTY:
            break;
        default:
            break;
    }
}

/* 输出变量声明列表 */
static void gen_var_list(AstNode *var_list, FILE *fp)
{
    if (!var_list || !var_list->first_child) return;
    AstNode *v = var_list->first_child;
    while (v) {
        if (v->type == AST_ARRAY_DECL) {
            fprintf(fp, "%s[%d]", v->u.id_name, v->array_size);
        } else {
            fprintf(fp, "%s", v->u.id_name);
        }
        if (v->first_child) {
            fprintf(fp, " = ");
            gen_expr(v->first_child, fp);
        }
        v = v->next_sibling;
        if (v) fprintf(fp, ", ");
    }
}

/* 输出形参列表 */
static void gen_param_list(AstNode *param_list, FILE *fp)
{
    if (!param_list || !param_list->first_child) {
        fprintf(fp, "void");
        return;
    }
    AstNode *p = param_list->first_child;
    while (p) {
        gen_type(p->first_child, fp);
        fprintf(fp, " %s", p->u.id_name);
        p = p->next_sibling;
        if (p) fprintf(fp, ", ");
    }
}

/* 输出语句（递归） */
static void gen_stmt(AstNode *stmt, FILE *fp, int indent);

/* 输出复合语句 */
static void gen_compound(AstNode *comp, FILE *fp, int indent)
{
    fprintf(fp, "{\n");

    /* 遍历孩子：局部声明(AST_EXT_VAR_DEF) + 语句序列(AST_STMT_LIST) */
    AstNode *child = comp->first_child;
    while (child) {
        if (child->type == AST_EXT_VAR_DEF) {
            /* 局部变量声明 */
            fp_indent(fp, indent + 1);
            AstNode *type_node = child->first_child;
            AstNode *var_list = type_node ? type_node->next_sibling : NULL;
            gen_type(type_node, fp);
            fprintf(fp, " ");
            gen_var_list(var_list, fp);
            fprintf(fp, ";\n");
        } else if (child->type == AST_STMT_LIST) {
            /* 语句序列 */
            AstNode *s = child->first_child;
            while (s) {
                gen_stmt(s, fp, indent + 1);
                s = s->next_sibling;
            }
        }
        child = child->next_sibling;
    }

    fp_indent(fp, indent);
    fprintf(fp, "}\n");
}

/* 输出单条语句 */
static void gen_stmt(AstNode *stmt, FILE *fp, int indent)
{
    if (!stmt) return;
    fp_indent(fp, indent);
    switch (stmt->type) {
        case AST_EXPR_STMT:
            gen_expr(stmt->first_child, fp);
            fprintf(fp, ";\n");
            break;
        case AST_COMPOUND:
            gen_compound(stmt, fp, indent);
            break;
        case AST_IF: {
            AstNode *cond = stmt->first_child;
            AstNode *then_s = cond->next_sibling;
            fprintf(fp, "if (");
            gen_expr(cond, fp);
            fprintf(fp, ")\n");
            if (then_s && then_s->type != AST_COMPOUND) {
                gen_stmt(then_s, fp, indent + 1);
            } else if (then_s) {
                fp_indent(fp, indent);
                gen_compound(then_s, fp, indent);
            }
            break;
        }
        case AST_IF_ELSE: {
            AstNode *cond = stmt->first_child;
            AstNode *then_s = cond->next_sibling;
            AstNode *else_s = then_s->next_sibling;
            fprintf(fp, "if (");
            gen_expr(cond, fp);
            fprintf(fp, ")\n");
            if (then_s && then_s->type != AST_COMPOUND) {
                gen_stmt(then_s, fp, indent + 1);
            } else if (then_s) {
                fp_indent(fp, indent);
                gen_compound(then_s, fp, indent);
            }
            fp_indent(fp, indent);
            fprintf(fp, "else\n");
            if (else_s && else_s->type != AST_COMPOUND) {
                gen_stmt(else_s, fp, indent + 1);
            } else if (else_s) {
                fp_indent(fp, indent);
                gen_compound(else_s, fp, indent);
            }
            break;
        }
        case AST_WHILE: {
            AstNode *cond = stmt->first_child;
            AstNode *body = cond->next_sibling;
            fprintf(fp, "while (");
            gen_expr(cond, fp);
            fprintf(fp, ")\n");
            if (body && body->type != AST_COMPOUND) {
                gen_stmt(body, fp, indent + 1);
            } else if (body) {
                fp_indent(fp, indent);
                gen_compound(body, fp, indent);
            }
            break;
        }
        case AST_FOR: {
            AstNode *init = stmt->first_child;
            AstNode *cond = init ? init->next_sibling : NULL;
            AstNode *step = cond ? cond->next_sibling : NULL;
            AstNode *body = step ? step->next_sibling : NULL;
            fprintf(fp, "for (");
            if (init && init->type != AST_EMPTY) gen_expr(init, fp);
            fprintf(fp, "; ");
            if (cond && cond->type != AST_EMPTY) gen_expr(cond, fp);
            fprintf(fp, "; ");
            if (step && step->type != AST_EMPTY) gen_expr(step, fp);
            fprintf(fp, ")\n");
            if (body && body->type != AST_COMPOUND) {
                gen_stmt(body, fp, indent + 1);
            } else if (body) {
                fp_indent(fp, indent);
                gen_compound(body, fp, indent);
            }
            break;
        }
        case AST_RETURN:
            fprintf(fp, "return");
            if (stmt->first_child && stmt->first_child->type != AST_EMPTY) {
                fprintf(fp, " ");
                gen_expr(stmt->first_child, fp);
            }
            fprintf(fp, ";\n");
            break;
        case AST_BREAK:
            fprintf(fp, "break;\n");
            break;
        case AST_CONTINUE:
            fprintf(fp, "continue;\n");
            break;
        default:
            break;
    }
}

/* ===== 格式化输出主入口 ===== */
void ast_gen_format(AstNode *root, FILE *fp_out)
{
    if (!root || !fp_out) return;
    g_indent = 0;

    /* 遍历外部定义序列（嵌套结构：EXT_DEF_LIST的first_child是定义，next_sibling是下一个EXT_DEF_LIST） */
    AstNode *ext_list = root->first_child;
    if (!ext_list) return;

    AstNode *ext = ext_list->first_child;
    while (ext) {
        /* 跳过嵌套的EXT_DEF_LIST结点，进入其first_child */
        if (ext->type == AST_EXT_DEF_LIST) {
            ext = ext->first_child;
            continue;
        }
        switch (ext->type) {
            case AST_EXT_VAR_DEF: {
                AstNode *type_node = ext->first_child;
                AstNode *var_list = type_node->next_sibling;
                gen_type(type_node, fp_out);
                fprintf(fp_out, " ");
                gen_var_list(var_list, fp_out);
                fprintf(fp_out, ";\n\n");
                break;
            }
            case AST_FUNC_DECL: {
                AstNode *ret_type = ext->first_child;
                AstNode *param_list = ret_type->next_sibling;
                gen_type(ret_type, fp_out);
                fprintf(fp_out, " %s(", ext->u.id_name);
                gen_param_list(param_list, fp_out);
                fprintf(fp_out, ");\n\n");
                break;
            }
            case AST_FUNC_DEF: {
                AstNode *ret_type = ext->first_child;
                AstNode *param_list = ret_type->next_sibling;
                AstNode *body = param_list->next_sibling;
                gen_type(ret_type, fp_out);
                fprintf(fp_out, " %s(", ext->u.id_name);
                gen_param_list(param_list, fp_out);
                fprintf(fp_out, ")\n");
                if (body && body->type == AST_COMPOUND) {
                    gen_compound(body, fp_out, 0);
                }
                fprintf(fp_out, "\n");
                break;
            }
            default:
                break;
        }
        ext = ext->next_sibling;
    }
}
