#ifndef __PARSER_H__
#define __PARSER_H__

#include "ast.h"

/* ===== 全局变量（文档要求） ===== */
extern int w;           /* 当前token种类 */
extern int parse_err;   /* 语法错误标记 */

/* ===== 递归下降子程序（每个BNF单元对应一个函数） ===== */
AstNode* Program(void);         /* <程序> */
AstNode* ExtDefList(void);      /* <外部定义序列> */
AstNode* ExtDef(void);           /* <外部定义> */
AstNode* VarList(const char *first_name);  /* <变量声明序列>，first_name为已预读的第一个变量名 */
AstNode* ParamList(void);        /* <形参列表> */
AstNode* Compound(void);         /* <复合语句> */
AstNode* StmtList(void);         /* <语句序列> */
AstNode* Statement(void);        /* <语句> */
AstNode* Exp(int endsym);        /* <表达式>，endsym为结束符号 */

#endif
