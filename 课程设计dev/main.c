#include "lex.h"
#include "parser.h"
#include "ast.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("用法: %s <源文件.c>\n", argv[0]);
        printf("示例: %s test.c\n", argv[0]);
        return 1;
    }
    
	/*    需要能独立运行的exe文件就改用此代码 
	const char* src_file;
    if (argc < 2) {
        src_file = "test.c";   // 没传参数就默认用test.c
    } else {
        src_file = argv[1];
    }
	*/
	
    const char* src_file = argv[1];
    fp_src = fopen(src_file, "r");
    if (!fp_src) {
        printf("无法打开源文件: %s\n", src_file);
        return 1;
    }

    printf("==================================================\n");
    printf("  高级语言源程序格式处理工具\n");
    printf("  输入文件: %s\n", src_file);
    printf("==================================================\n\n");

    /* ===== 第一部分：词法分析测试 ===== */
    printf("========== 【一、词法分析输出】 ==========\n");
    rewind(fp_src);
    line_no = 1;
    int tk;
    int token_count = 0;
    int lex_error = 0;
    while ((tk = gettoken()) != TOKEN_EOF) {
        print_token(tk);
        token_count++;
        if (tk == ERROR_TOKEN) { lex_error = 1; break; }
    }
    printf("--------------------------------------------------\n");
    printf("共识别 %d 个单词", token_count);
    if (lex_error) printf(" (含词法错误)");
    printf("\n\n");

    if (lex_error) {
        printf("词法分析存在错误，终止语法分析。\n");
        fclose(fp_src);
        return 1;
    }

    /* ===== 第二部分：语法分析，构建AST ===== */
    printf("========== 【二、语法分析 & 抽象语法树】 ==========\n");
    rewind(fp_src);
    line_no = 1;
    parse_err = 0;

    AstNode* tree = Program();

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

    /* ===== 第三部分：格式化输出 ===== */
    printf("========== 【三、格式化输出】 ==========\n");
    const char* out_file = "out.c";
    FILE* fout = fopen(out_file, "w");
    if (!fout) {
        printf("无法创建输出文件: %s\n", out_file);
        ast_free(tree);
        fclose(fp_src);
        return 1;
    }
    ast_gen_format(tree, fout);
    fclose(fout);
    printf("格式化源码已输出到文件: %s\n\n", out_file);

    /* 同时在控制台显示格式化结果 */
    printf("----- 格式化后的源码 -----\n");
    FILE* fshow = fopen(out_file, "r");
    if (fshow) {
        char buf[1024];
        while (fgets(buf, sizeof(buf), fshow)) {
            printf("%s", buf);
        }
        fclose(fshow);
    }
    printf("\n");

    /* ===== 清理 ===== */
    ast_free(tree);
    fclose(fp_src);

    printf("==================================================\n");
    printf("  处理完成！\n");
    printf("==================================================\n");

	system("pause");

    return 0;
}
