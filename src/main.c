#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "optimizers/optimizer.h"
#include "ir.h"
#include "code_gen.h"


int main(void) {
    /* Test */

    
    char *code =
        "local x = false\n"
        "local y = true\n"
        "local z = false\n"
        "if (x) then\n"
        "    local g = 1;\n"
        "    if (g) then\n"
        "        local a = 10;\n"
        "        if (a) then\n"
        "            local q = 100;\n"
        "        else\n"
        "            local q = 200;\n"
        "        end\n"
        "    else\n"
        "        local a = 20;\n"
        "    end\n"
        "else\n"
        "    if (g) then\n"
        "        local p = 1;\n"
        "        if (p) then\n"
        "            local g = 2;\n"
        "            if (g) then\n"
        "                local r = 3;\n"
        "            else\n"
        "                local r = 4;\n"
        "            end\n"
        "        else\n"
        "            local p = 5;\n"
        "        end\n"
        "    else\n"
        "        local p = 0;\n"
        "        if (y) then\n"
        "            local g = 6;\n"
        "            if (z) then\n"
        "                local s = 7;\n"
        "            else\n"
        "                if (g) then\n"
        "                    local t = 8;\n"
        "                else\n"
        "                    local t = 9;\n"
        "                end\n"
        "            end\n"
        "        else\n"
        "            local g = 10;\n"
        "        end\n"
        "    end\n"
        "    local g = 0;\n"
        "    if (g) then\n"
        "        local u = 11;\n"
        "    else\n"
        "        local u = 12;\n"
        "    end\n"
        "end\n"
        "if (y) then\n"
        "    local g = 13;\n"
        "    if (g) then\n"
        "        local v = 14;\n"
        "    else\n"
        "        local v = 15;\n"
        "    end\n"
        "else\n"
        "    if (g) then\n"
        "        local w = 16;\n"
        "    else\n"
        "        local w = 17;\n"
        "    end\n"
        "end\n";
    

    /*
    char *code =
        "if (x) then\n"
        "    local g = 1;\n"
        "else\n"
        "    local g = 0;\n"
        "end\n";
    */

    //char *code =
    //    "local x = 2 + 3 * 4\n";

    TokenArray* lexed = lex(code, LUA_54);
    if (!lexed) {
        return 1;
    }

    printMyTokens(lexed);

    astNode* node = parse(lexed, LUA_54);
    if (!node) {
        TokenCleanup(lexed);
        return 1;
    }

    PrintMyAST(node, 0);

    /* IR part */
    IRArray ir;
    IRAlloc(&ir);

    IRStmtGen(&ir, node);
    IRPrint(&ir);

    /* Optimization Test */
    optimize(&ir);
    printf("Optimized:\n");
    IRPrint(&ir);

    /* Code Gen test */
    printf("Code Gen:\n");
    CodeGenLua(&ir, stdout);

    /* Free before ending */
    TokenCleanup(lexed);
    AstCleanup(node);
    IRCleanup(&ir);

    return 0;
}
