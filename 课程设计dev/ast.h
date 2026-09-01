#ifndef __AST_H__
#define __AST_H__

#include "lex.h"

/* ===== AST 结点类型枚举 ===== */
typedef enum {
    AST_PROG,           /* 程序根 */
    AST_EXT_DEF_LIST,   /* 外部定义序列 */
    AST_EXT_VAR_DEF,    /* 外部变量定义 */
    AST_FUNC_DEF,       /* 函数定义 */
    AST_FUNC_DECL,      /* 函数声明(原型) */
    AST_PARAM_LIST,     /* 形参列表 */
    AST_PARAM,          /* 单个形参 */
    AST_VAR_LIST,       /* 变量声明列表 */
    AST_VAR_DECL,       /* 单个变量声明(可带初始化) */
    AST_COMPOUND,       /* 复合语句 { } */
    AST_STMT_LIST,      /* 语句序列 */
    AST_IF,             /* if 语句 */
    AST_IF_ELSE,        /* if-else 语句 */
    AST_WHILE,          /* while 语句 */
    AST_FOR,            /* for 语句 */
    AST_RETURN,         /* return 语句 */
    AST_BREAK,          /* break 语句 */
    AST_CONTINUE,       /* continue 语句 */
    AST_EXPR_STMT,      /* 表达式语句 */
    AST_EXPR_OP,        /* 二元运算符表达式结点 */
    AST_UNARY_OP,       /* 一元运算符表达式结点 */
    AST_FUNC_CALL,      /* 函数调用 */
    AST_ARRAY_DECL,     /* 数组声明 */
    AST_ARRAY_ACCESS,   /* 数组下标访问 */
    AST_ARG_LIST,       /* 实参列表 */
    AST_IDENT,          /* 标识符叶子 */
    AST_INT_CONST,      /* 整型常量叶子 */
    AST_FLOAT_CONST,    /* 浮点常量叶子 */
    AST_CHAR_CONST,     /* 字符常量叶子 */
    AST_STRING_CONST,   /* 字符串常量叶子 */
    AST_TYPE,           /* 类型结点 */
    AST_EMPTY           /* 空结点(用于for空表达式等) */
} AstNodeType;

/* ===== AST 结点：结构体 + 共用体 union（文档强制要求） ===== */
typedef struct AstNode {
    AstNodeType type;
    union {
        char   id_name[128];    /* 标识符/函数名 */
        long   int_val;          /* 整型常量值 */
        double float_val;        /* 浮点常量值 */
        char   char_val[16];     /* 字符常量文本 */
        char   str_val[256];     /* 字符串常量文本 */
        int    op_kind;          /* 运算符 token_kind */
        int    type_kind;        /* 类型 token_kind (KW_INT/KW_FLOAT/...) */
    } u;
    int array_size;              /* 数组大小(仅数组声明结点使用) */
    struct AstNode *first_child;  /* 第一个孩子 */
    struct AstNode *next_sibling; /* 下一个兄弟 */
} AstNode;

/* ===== 结点创建函数 ===== */
AstNode* ast_new_prog(AstNode *ext_list);
AstNode* ast_new_ext_def_list(AstNode *first);
AstNode* ast_new_ext_var_def(AstNode *type_node, AstNode *var_list);
AstNode* ast_new_func_def(AstNode *ret_type, const char *name,
                           AstNode *param_list, AstNode *body);
AstNode* ast_new_func_decl(AstNode *ret_type, const char *name,
                            AstNode *param_list);
AstNode* ast_new_param_list(AstNode *first);
AstNode* ast_new_param(AstNode *type_node, const char *name);
AstNode* ast_new_var_list(AstNode *first);
AstNode* ast_new_var_decl(const char *name, AstNode *init_expr);
AstNode* ast_new_compound(AstNode *local_decls, AstNode *stmt_list);
AstNode* ast_new_stmt_list(AstNode *first);
AstNode* ast_new_if(AstNode *cond, AstNode *then_stmt);
AstNode* ast_new_if_else(AstNode *cond, AstNode *then_stmt, AstNode *else_stmt);
AstNode* ast_new_while(AstNode *cond, AstNode *body);
AstNode* ast_new_for(AstNode *init, AstNode *cond, AstNode *step, AstNode *body);
AstNode* ast_new_return(AstNode *expr);
AstNode* ast_new_break(void);
AstNode* ast_new_continue(void);
AstNode* ast_new_expr_stmt(AstNode *expr);
AstNode* ast_new_expr_op(int op, AstNode *left, AstNode *right);
AstNode* ast_new_unary_op(int op, AstNode *operand);
AstNode* ast_new_func_call(const char *name, AstNode *arg_list);
AstNode* ast_new_array_decl(const char *name, int size, AstNode *init_expr);
AstNode* ast_new_array_access(const char *name, AstNode *index);
AstNode* ast_new_arg_list(AstNode *first);
AstNode* ast_new_ident(const char *name);
AstNode* ast_new_int_const(long v);
AstNode* ast_new_float_const(double v);
AstNode* ast_new_char_const(const char *text);
AstNode* ast_new_string_const(const char *text);
AstNode* ast_new_type(int tk);
AstNode* ast_new_empty(void);

/* ===== 树操作 ===== */
void ast_add_sibling(AstNode *node, AstNode *sib);  /* 添加兄弟结点 */
void ast_print(AstNode *root, int indent);            /* 先根遍历打印AST */
void ast_free(AstNode *root);                          /* 释放整棵树 */

/* ===== 格式化输出 ===== */
void ast_gen_format(AstNode *root, FILE *fp_out);     /* 遍历AST输出格式化C源码 */

#endif
