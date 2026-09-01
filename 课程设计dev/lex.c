#include "lex.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* ===== 全局变量定义 ===== */
char token_text[TOKEN_TEXT_LEN];
FILE *fp_src = NULL;
int  line_no = 1;

/* ===== 关键字表 ===== */
typedef struct {
    const char *str;
    int kind;
} KeywordTab;

static const KeywordTab kw_tab[] = {
    {"int",      KW_INT},
    {"float",    KW_FLOAT},
    {"char",     KW_CHAR},
    {"long",     KW_LONG},
    {"void",     KW_VOID},
    {"if",       KW_IF},
    {"else",     KW_ELSE},
    {"while",    KW_WHILE},
    {"for",      KW_FOR},
    {"return",   KW_RETURN},
    {"break",    KW_BREAK},
    {"continue", KW_CONTINUE},
    {NULL, 0}
};

/* 查询关键字，是则返回对应kind，否则返回IDENT */
static int lookup_keyword(const char *s)
{
    for (int i = 0; kw_tab[i].str != NULL; i++) {
        if (strcmp(s, kw_tab[i].str) == 0)
            return kw_tab[i].kind;
    }
    return IDENT;
}

/* 跳过块注释（进入时已经读掉了 / 和 *） */
static void skip_block_comment(void)
{
    int c;
    int prev = 0;
    while ((c = fgetc(fp_src)) != EOF) {
        if (c == '\n') line_no++;
        if (prev == '*' && c == '/') break;
        prev = c;
    }
}

/* 跳过行注释（进入时已经读掉了 / 和 /） */
static void skip_line_comment(void)
{
    int c;
    while ((c = fgetc(fp_src)) != EOF && c != '\n')
        ;
    if (c == '\n') line_no++;
}

/* 处理预处理指令 #include / #define，整行跳过 */
static void skip_preprocessor(void)
{
    int c;
    /* 读取 # 后面的指令名 */
    char cmd[32] = {0};
    int idx = 0;
    while ((c = fgetc(fp_src)) != EOF && isalpha(c) && idx < 31) {
        cmd[idx++] = (char)c;
    }
    ungetc(c, fp_src);

    if (strcmp(cmd, "include") == 0) {
        /* 跳过到行尾，处理 <xxx.h> 或 "xxx.h" */
        while ((c = fgetc(fp_src)) != EOF && c != '\n')
            ;
        if (c == '\n') line_no++;
        return;
    } else if (strcmp(cmd, "define") == 0) {
        /* 简单宏定义，跳过到行尾（不处理跨行\续行） */
        while ((c = fgetc(fp_src)) != EOF && c != '\n')
            ;
        if (c == '\n') line_no++;
        return;
    }
    /* 其他预处理指令，跳过整行 */
    while ((c = fgetc(fp_src)) != EOF && c != '\n')
        ;
    if (c == '\n') line_no++;
}

/* ===== 核心词法分析函数 ===== */
int gettoken(void)
{
    int c;
    memset(token_text, 0, sizeof(token_text));

again:
    /* 跳过空白字符，统计行号 */
    while ((c = fgetc(fp_src)) != EOF && isspace(c)) {
        if (c == '\n') line_no++;
    }
    if (c == EOF) return TOKEN_EOF;

    /* 处理注释开头 '/' */
    if (c == '/') {
        int next = fgetc(fp_src);
        if (next == '/') {
            skip_line_comment();
            goto again;
        } else if (next == '*') {
            skip_block_comment();
            goto again;
        } else {
            ungetc(next, fp_src);
            token_text[0] = '/';
            token_text[1] = '\0';
            return DIV;
        }
    }

    /* 处理预处理指令 '#' */
    if (c == '#') {
        skip_preprocessor();
        goto again;
    }

    /* ===== 1. 字母/下划线开头：标识符或关键字 ===== */
    if (isalpha(c) || c == '_') {
        int idx = 0;
        token_text[idx++] = (char)c;
        while ((c = fgetc(fp_src)) != EOF && (isalnum(c) || c == '_')) {
            if (idx < TOKEN_TEXT_LEN - 1)
                token_text[idx++] = (char)c;
        }
        token_text[idx] = '\0';
        ungetc(c, fp_src);  /* 多读字符退回，文档要求ungetc */
        return lookup_keyword(token_text);
    }

    /* ===== 2. 数字开头：整型或浮点常量 ===== */
    if (isdigit(c)) {
        int idx = 0;
        int is_float = 0;
        int is_long = 0;   /* 是否带L后缀(long类型) */
        token_text[idx++] = (char)c;

        /* 十六进制 0x / 0X */
        if (c == '0') {
            int next = fgetc(fp_src);
            if (next == 'x' || next == 'X') {
                token_text[idx++] = (char)next;
                while ((c = fgetc(fp_src)) != EOF && isxdigit(c)) {
                    if (idx < TOKEN_TEXT_LEN - 1)
                        token_text[idx++] = (char)c;
                }
                /* 后缀 L/U */
                while (c == 'L' || c == 'l' || c == 'U' || c == 'u') {
                    if (c == 'L' || c == 'l') is_long = 1;
                    if (idx < TOKEN_TEXT_LEN - 1)
                        token_text[idx++] = (char)c;
                    c = fgetc(fp_src);
                }
                ungetc(c, fp_src);
                token_text[idx] = '\0';
                return is_long ? LONG_CONST : INT_CONST;
            } else if (isdigit(next) && next <= '7') {
                /* 八进制 */
                token_text[idx++] = (char)next;
                while ((c = fgetc(fp_src)) != EOF && c >= '0' && c <= '7') {
                    if (idx < TOKEN_TEXT_LEN - 1)
                        token_text[idx++] = (char)c;
                }
                while (c == 'L' || c == 'l' || c == 'U' || c == 'u') {
                    if (c == 'L' || c == 'l') is_long = 1;
                    if (idx < TOKEN_TEXT_LEN - 1)
                        token_text[idx++] = (char)c;
                    c = fgetc(fp_src);
                }
                ungetc(c, fp_src);
                token_text[idx] = '\0';
                return is_long ? LONG_CONST : INT_CONST;
            } else {
                ungetc(next, fp_src);
            }
        }

        /* 十进制整数部分 */
        while ((c = fgetc(fp_src)) != EOF && isdigit(c)) {
            if (idx < TOKEN_TEXT_LEN - 1)
                token_text[idx++] = (char)c;
        }

        /* 小数点 */
        if (c == '.') {
            is_float = 1;
            token_text[idx++] = '.';
            while ((c = fgetc(fp_src)) != EOF && isdigit(c)) {
                if (idx < TOKEN_TEXT_LEN - 1)
                    token_text[idx++] = (char)c;
            }
        }

        /* 指数部分 e/E */
        if (c == 'e' || c == 'E') {
            is_float = 1;
            token_text[idx++] = (char)c;
            c = fgetc(fp_src);
            if (c == '+' || c == '-') {
                token_text[idx++] = (char)c;
                c = fgetc(fp_src);
            }
            while (c != EOF && isdigit(c)) {
                if (idx < TOKEN_TEXT_LEN - 1)
                    token_text[idx++] = (char)c;
                c = fgetc(fp_src);
            }
        }

        /* 后缀：f/F (float), l/L (long) */
        if (c == 'f' || c == 'F') {
            is_float = 1;
            token_text[idx++] = (char)c;
            c = fgetc(fp_src);
        } else if (c == 'l' || c == 'L' || c == 'u' || c == 'U') {
            if (c == 'l' || c == 'L') is_long = 1;
            token_text[idx++] = (char)c;
            c = fgetc(fp_src);
            /* 可能有 LL */
            if ((c == 'l' || c == 'L') && (token_text[idx-1] == 'l' || token_text[idx-1] == 'L')) {
                token_text[idx++] = (char)c;
                c = fgetc(fp_src);
            }
        }

        ungetc(c, fp_src);
        token_text[idx] = '\0';
        if (is_float) return FLOAT_CONST;
        if (is_long) return LONG_CONST;
        return INT_CONST;
    }

    /* ===== 3. 字符常量 'x' ===== */
    if (c == '\'') {
        int idx = 0;
        token_text[idx++] = '\'';
        c = fgetc(fp_src);
        if (c == '\\') {
            /* 转义字符 */
            token_text[idx++] = '\\';
            c = fgetc(fp_src);
            token_text[idx++] = (char)c;
        } else {
            token_text[idx++] = (char)c;
        }
        c = fgetc(fp_src);
        if (c == '\'') {
            token_text[idx++] = '\'';
            token_text[idx] = '\0';
            return CHAR_CONST;
        } else {
            ungetc(c, fp_src);
            token_text[idx] = '\0';
            printf("[词法错误 line:%d] 字符常量缺少右单引号\n", line_no);
            return ERROR_TOKEN;
        }
    }

    /* ===== 4. 字符串常量 "xxx" ===== */
    if (c == '"') {
        int idx = 0;
        token_text[idx++] = '"';
        while ((c = fgetc(fp_src)) != EOF && c != '"') {
            if (c == '\\') {
                token_text[idx++] = '\\';
                c = fgetc(fp_src);
                if (c == '\n') line_no++;
            }
            if (c == '\n') line_no++;
            if (idx < TOKEN_TEXT_LEN - 2)
                token_text[idx++] = (char)c;
        }
        if (c == '"') {
            token_text[idx++] = '"';
            token_text[idx] = '\0';
            return STRING_CONST;
        } else {
            token_text[idx] = '\0';
            printf("[词法错误 line:%d] 字符串常量缺少右双引号\n", line_no);
            return ERROR_TOKEN;
        }
    }

    /* ===== 5. 运算符和定界符 ===== */
    switch (c) {
        case '+': token_text[0] = '+'; token_text[1] = 0; return PLUS;
        case '-': token_text[0] = '-'; token_text[1] = 0; return MINUS;
        case '*': token_text[0] = '*'; token_text[1] = 0; return MUL;
        case '%': token_text[0] = '%'; token_text[1] = 0; return MOD;

        case '=': {
            int nc = fgetc(fp_src);
            if (nc == '=') { strcpy(token_text, "=="); return EQ; }
            ungetc(nc, fp_src);
            token_text[0] = '='; token_text[1] = 0; return ASSIGN;
        }
        case '!': {
            int nc = fgetc(fp_src);
            if (nc == '=') { strcpy(token_text, "!="); return NEQ; }
            ungetc(nc, fp_src);
            token_text[0] = '!'; token_text[1] = 0; return NOT;
        }
        case '>': {
            int nc = fgetc(fp_src);
            if (nc == '=') { strcpy(token_text, ">="); return GE; }
            ungetc(nc, fp_src);
            token_text[0] = '>'; token_text[1] = 0; return GT;
        }
        case '<': {
            int nc = fgetc(fp_src);
            if (nc == '=') { strcpy(token_text, "<="); return LE; }
            ungetc(nc, fp_src);
            token_text[0] = '<'; token_text[1] = 0; return LT;
        }
        case '&': {
            int nc = fgetc(fp_src);
            if (nc == '&') { strcpy(token_text, "&&"); return AND; }
            ungetc(nc, fp_src);
            printf("[词法错误 line:%d] 非法字符 & (位运算不支持)\n", line_no);
            return ERROR_TOKEN;
        }
        case '|': {
            int nc = fgetc(fp_src);
            if (nc == '|') { strcpy(token_text, "||"); return OR; }
            ungetc(nc, fp_src);
            printf("[词法错误 line:%d] 非法字符 | (位运算不支持)\n", line_no);
            return ERROR_TOKEN;
        }

        case '(': token_text[0] = '('; return LP;
        case ')': token_text[0] = ')'; return RP;
        case '{': token_text[0] = '{'; return LB;
        case '}': token_text[0] = '}'; return RB;
        case '[': token_text[0] = '['; return LSQUARE;
        case ']': token_text[0] = ']'; return RSQUARE;
        case ';': token_text[0] = ';'; return SEMI;
        case ',': token_text[0] = ','; return COMMA;

        default:
            printf("[词法错误 line:%d] 非法字符 '%c' (ASCII=%d)\n", line_no, c, c);
            return ERROR_TOKEN;
    }
}

/* 返回token种类的中文名称 */
const char* token_name(int kind)
{
    switch (kind) {
        case IDENT:        return "标识符";
        case INT_CONST:    return "整型常量";
        case LONG_CONST:   return "长整型常量";
        case FLOAT_CONST:  return "浮点常量";
        case CHAR_CONST:   return "字符常量";
        case STRING_CONST: return "字符串常量";
        case KW_INT:       return "关键字-int";
        case KW_FLOAT:     return "关键字-float";
        case KW_CHAR:      return "关键字-char";
        case KW_LONG:      return "关键字-long";
        case KW_VOID:      return "关键字-void";
        case KW_IF:        return "关键字-if";
        case KW_ELSE:      return "关键字-else";
        case KW_WHILE:     return "关键字-while";
        case KW_FOR:       return "关键字-for";
        case KW_RETURN:    return "关键字-return";
        case KW_BREAK:     return "关键字-break";
        case KW_CONTINUE:  return "关键字-continue";
        case PLUS:         return "加号+";
        case MINUS:        return "减号-";
        case MUL:          return "乘号*";
        case DIV:          return "除号/";
        case MOD:          return "取模%";
        case ASSIGN:       return "赋值=";
        case EQ:           return "等于==";
        case NEQ:          return "不等于!=";
        case GT:           return "大于>";
        case GE:           return "大于等于>=";
        case LT:           return "小于<";
        case LE:           return "小于等于<=";
        case AND:          return "逻辑与&&";
        case OR:           return "逻辑或||";
        case NOT:          return "逻辑非!";
        case LP:           return "左括号(";
        case RP:           return "右括号)";
        case LB:           return "左大括号{";
        case RB:           return "右大括号}";
        case LSQUARE:      return "左中括号[";
        case RSQUARE:      return "右中括号]";
        case SEMI:         return "分号;";
        case COMMA:        return "逗号,";
        case HASH:         return "井号#";
        case TOKEN_EOF:    return "文件结束";
        case ERROR_TOKEN:  return "错误token";
        default:            return "未知";
    }
}

/* 打印token */
void print_token(int kind)
{
    printf("%-16s | %s\n", token_name(kind), token_text);
}
