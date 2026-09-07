#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("用法: %s <源文件.c>\n", argv[0]);
        printf("示例: %s test.c\n", argv[0]);
        return 1;
    }

    const char *src_file = argv[1];
    fp_src = fopen(src_file, "r");
    if (!fp_src) {
        printf("无法打开源文件: %s\n", src_file);
        return 1;
    }

    printf("==================================================\n");
    printf("  高级语言源程序格式处理工具\n");
    printf("  输入文件: %s\n", src_file);
    printf("==================================================\n\n");

    /*  词法分析测试  */
    printf("========== 【一、词法分析输出】 ==========\n");
    rewind(fp_src);
    line_no = 1;
    lex_error_count = 0;
    int tk;
    int token_count = 0;
    while ((tk = gettoken()) != TOKEN_EOF) {
        print_token(tk);
        token_count++;
    }
    printf("--------------------------------------------------\n");
    printf("共识别 %d 个单词", token_count);
    if (lex_error_count > 0) printf(" (含 %d 个词法错误)", lex_error_count);
    printf("\n\n");

    /* 语法分析，构建AST  */
    printf("========== 【二、语法分析 & 抽象语法树】 ==========\n");
    rewind(fp_src);
    line_no = 1;
    parse_err = 0;
    parse_error_total = 0;

    AstNode *tree = Program();

    if (parse_error_total > 0) {
        printf("\n语法分析完成，共发现 %d 个语法错误。\n", parse_error_total);
        ast_free(tree);
        fclose(fp_src);
        return 1;
    }

    if (parse_err || !tree) {
        printf("\n语法分析失败，存在语法错误。\n");
        ast_free(tree);
        fclose(fp_src);
        return 1;
    }

    printf("语法分析成功！\n\n");
    printf("----- 抽象语法树(AST)打印 -----\n");
    ast_print(tree, 0);
    printf("\n");

    /*  格式化输出  */
    printf("========== 【三、格式化输出】 ==========\n");
    const char *out_file = "out.c";
    FILE *fout = fopen(out_file, "w");
    if (!fout) {
        printf("无法创建输出文件: %s\n", out_file);
        ast_free(tree);
        fclose(fp_src);
        return 1;
    }
    ast_gen_format(tree, fout);
    fclose(fout);
    printf("格式化源码已输出到文件: %s\n\n", out_file);

    /* 在控制台显示格式化结果 */
    printf("----- 格式化后的源码 -----\n");
    FILE *fshow = fopen(out_file, "r");
    if (fshow) {
        char buf[1024];
        while (fgets(buf, sizeof(buf), fshow)) {
            printf("%s", buf);
        }
        fclose(fshow);
    }
    printf("\n");

    /*  清理  */
    ast_free(tree);
    fclose(fp_src);

    printf("==================================================\n");
    printf("  处理完成！\n");
    printf("==================================================\n");

    return 0;
}
