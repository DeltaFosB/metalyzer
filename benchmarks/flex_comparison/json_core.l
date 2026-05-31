%{
#include <iostream>
#include <string>
#include <sstream>

enum class JsonToken {
    TOK_EOF = -1,
    TOK_ERROR = -2,
    OBJ_START = 1,
    OBJ_END = 2,
    ARR_START = 3,
    ARR_END = 4,
    COLON = 5,
    COMMA = 6,
    KW_TRUE = 7,
    KW_FALSE = 8,
    KW_NULL = 9,
    STRING_LIT = 10
};

namespace {
    int current_line = 1;
    int current_col = 1;
    int token_start_col = 1;
}

#define YY_USER_ACTION { \
    token_start_col = current_col; \
    current_col += yyleng; \
}
%}

%option c++
%option batch
%option noyywrap

%%

"{"         { return (int)JsonToken::OBJ_START; }
"}"         { return (int)JsonToken::OBJ_END; }
"["         { return (int)JsonToken::ARR_START; }
"]"         { return (int)JsonToken::ARR_END; }
":"         { return (int)JsonToken::COLON; }
","         { return (int)JsonToken::COMMA; }
"true"      { return (int)JsonToken::KW_TRUE; }
"false"     { return (int)JsonToken::KW_FALSE; }
"null"      { return (int)JsonToken::KW_NULL; }

\"([^\"\\\n]|\\[\"\\/bfnrt])*\" { return (int)JsonToken::STRING_LIT; }

[ \t\r]     {
                if (yytext[0] == '\t') {
                    current_col += (4 - ((current_col - 1) % 4)) - 1;
                }
            }
\n          { current_line++; current_col = 1; }

.           { return (int)JsonToken::TOK_ERROR; }

%%
