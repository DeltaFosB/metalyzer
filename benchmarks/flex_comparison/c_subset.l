%{
#include <iostream>
#include <string>
#include <sstream>

enum class CToken {
    TOK_EOF = -1,
    TOK_ERROR = -2,
    
    KW_AUTO = 1, KW_BREAK, KW_CASE, KW_CHAR, KW_CONST, KW_CONTINUE, KW_DEFAULT,
    KW_DO, KW_DOUBLE, KW_ELSE, KW_ENUM, KW_EXTERN, KW_FLOAT, KW_FOR, KW_GOTO,
    KW_IF, KW_INT, KW_LONG, KW_REGISTER, KW_RETURN, KW_SHORT, KW_SIGNED,
    KW_SIZEOF, KW_STATIC, KW_STRUCT, KW_SWITCH, KW_TYPEDEF, KW_UNION,
    KW_UNSIGNED, KW_VOID, KW_VOLATILE, KW_WHILE,

    IDENTIFIER = 100,
    LIT_INT,
    LIT_FLOAT,
    LIT_STRING,

    OP_ASSIGN, OP_PLUS, OP_MINUS, OP_MUL, OP_DIV, OP_MOD,
    OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LEQ, OP_GEQ,
    OP_AND, OP_OR, OP_NOT,
    OP_INC, OP_DEC,
    OP_ARROW,
    
    LBRACE, RBRACE, LBRACKET, RBRACKET, LPAREN, RPAREN,
    SEMICOLON, COMMA, COLON
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

"auto"        { return (int)CToken::KW_AUTO; }
"break"       { return (int)CToken::KW_BREAK; }
"case"        { return (int)CToken::KW_CASE; }
"char"        { return (int)CToken::KW_CHAR; }
"const"       { return (int)CToken::KW_CONST; }
"continue"    { return (int)CToken::KW_CONTINUE; }
"default"     { return (int)CToken::KW_DEFAULT; }
"do"          { return (int)CToken::KW_DO; }
"double"      { return (int)CToken::KW_DOUBLE; }
"else"        { return (int)CToken::KW_ELSE; }
"enum"        { return (int)CToken::KW_ENUM; }
"extern"      { return (int)CToken::KW_EXTERN; }
"float"       { return (int)CToken::KW_FLOAT; }
"for"         { return (int)CToken::KW_FOR; }
"goto"        { return (int)CToken::KW_GOTO; }
"if"          { return (int)CToken::KW_IF; }
"int"         { return (int)CToken::KW_INT; }
"long"        { return (int)CToken::KW_LONG; }
"register"    { return (int)CToken::KW_REGISTER; }
"return"      { return (int)CToken::KW_RETURN; }
"short"       { return (int)CToken::KW_SHORT; }
"signed"      { return (int)CToken::KW_SIGNED; }
"sizeof"      { return (int)CToken::KW_SIZEOF; }
"static"      { return (int)CToken::KW_STATIC; }
"struct"      { return (int)CToken::KW_STRUCT; }
"switch"      { return (int)CToken::KW_SWITCH; }
"typedef"     { return (int)CToken::KW_TYPEDEF; }
"union"       { return (int)CToken::KW_UNION; }
"unsigned"    { return (int)CToken::KW_UNSIGNED; }
"void"        { return (int)CToken::KW_VOID; }
"volatile"    { return (int)CToken::KW_VOLATILE; }
"while"       { return (int)CToken::KW_WHILE; }

[a-zA-Z_][a-zA-Z0-9_]* { return (int)CToken::IDENTIFIER; }

[0-9]+\.[0-9]+                  { return (int)CToken::LIT_FLOAT; }
[0-9]+                          { return (int)CToken::LIT_INT; }
\"([^\"\\\n]|\\[\"\\ntr])*\"     { return (int)CToken::LIT_STRING; }

"++"        { return (int)CToken::OP_INC; }
"--"        { return (int)CToken::OP_DEC; }
"->"        { return (int)CToken::OP_ARROW; }
"=="        { return (int)CToken::OP_EQ; }
"!="        { return (int)CToken::OP_NEQ; }
"<="        { return (int)CToken::OP_LEQ; }
">="        { return (int)CToken::OP_GEQ; }
"&&"        { return (int)CToken::OP_AND; }
"||"        { return (int)CToken::OP_OR; }

"="         { return (int)CToken::OP_ASSIGN; }
"+"         { return (int)CToken::OP_PLUS; }
"-"         { return (int)CToken::OP_MINUS; }
"*"         { return (int)CToken::OP_MUL; }
"/"         { return (int)CToken::OP_DIV; }
"%"         { return (int)CToken::OP_MOD; }
"!"         { return (int)CToken::OP_NOT; }
"<"         { return (int)CToken::OP_LT; }
">"         { return (int)CToken::OP_GT; }

"{"         { return (int)CToken::LBRACE; }
"}"         { return (int)CToken::RBRACE; }
"["         { return (int)CToken::LBRACKET; }
"]"         { return (int)CToken::RBRACKET; }
"("         { return (int)CToken::LPAREN; }
")"         { return (int)CToken::RPAREN; }
";"         { return (int)CToken::SEMICOLON; }
","         { return (int)CToken::COMMA; }
":"         { return (int)CToken::COLON; }

[ \t\r]     {
                if (yytext[0] == '\t') {
                    current_col += (4 - ((current_col - 1) % 4)) - 1;
                }
            }
\n          { current_line++; current_col = 1; }

.           { return (int)CToken::TOK_ERROR; }

%%
