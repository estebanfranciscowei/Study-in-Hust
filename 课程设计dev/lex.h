#ifndef __LEX_H__
#define __LEX_H__

#include <stdio.h>

/* ===== 单词种类枚举 ===== */
enum token_kind {
    ERROR_TOKEN = 0,
    /* 标识符与常量 */
    IDENT,
    INT_CONST,
    FLOAT_CONST,
    CHAR_CONST,
    STRING_CONST,
    /* 关键字 */
    KW_INT, KW_FLOAT, KW_CHAR,
    KW_IF, KW_ELSE,
    KW_WHILE, KW_FOR,
    KW_RETURN, KW_BREAK, KW_CONTINUE,
    KW_VOID,
    /* 运算符 */
    PLUS,       /* +  */
    MINUS,      /* -  */
    MUL,        /* *  */
    DIV,        /* /  */
    MOD,        /* %  */
    ASSIGN,     /* =  */
    EQ,         /* == */
    NEQ,        /* != */
    GT,         /* >  */
    GE,         /* >= */
    LT,         /* <  */
    LE,         /* <= */
    AND,        /* && */
    OR,         /* || */
    NOT,        /* !  */
    /* 定界符 */
    LP,         /* (  */
    RP,         /* )  */
    LB,         /* {  */
    RB,         /* }  */
    SEMI,       /* ;  */
    COMMA,      /* ,  */
    HASH,       /* #  */
    /* 预处理伪token */
    PRE_INCLUDE,
    PRE_DEFINE,
    /* 文件结束 */
    TOKEN_EOF
};

#define TOKEN_TEXT_LEN 512

/* ===== 全局变量（文档要求） ===== */
extern char token_text[TOKEN_TEXT_LEN];  /* 当前单词自身字符串 */
extern FILE* fp_src;                      /* 源文件指针 */
extern int  line_no;                      /* 当前行号，用于报错 */

/* ===== 函数声明 ===== */
int  gettoken(void);       /* 词法分析核心：每次调用返回一个token */
void print_token(int kind);/* 打印token调试信息 */
const char* token_name(int kind); /* 返回token种类的中文名称 */

#endif
