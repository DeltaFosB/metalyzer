%{
#include <iostream>
#include <string>
#include <sstream>

enum class CalcToken {
    TOK_EOF = -1,
    TOK_ERROR = -2,
    NUM_INT = 1,
    PLUS = 2,
    MINUS = 3,
    MUL = 4,
    DIV = 5,
    ASSIGN = 6,
    ADD_ASSIGN = 7,
    LPAREN = 8,
    RPAREN = 9,
    TOK_ID = 10
};
%}

%option c++
%option batch
%option noyywrap

%%

[0-9]+    { return (int)CalcToken::NUM_INT; }
[a-z]+    { return (int)CalcToken::TOK_ID; }
"+="      { return (int)CalcToken::ADD_ASSIGN; }
"="       { return (int)CalcToken::ASSIGN; }
"+"       { return (int)CalcToken::PLUS; }
"-"       { return (int)CalcToken::MINUS; }
"*"       { return (int)CalcToken::MUL; }
"/"       { return (int)CalcToken::DIV; }
"("       { return (int)CalcToken::LPAREN; }
")"       { return (int)CalcToken::RPAREN; }

[ \t\n\r]+ { }

.         { return (int)CalcToken::TOK_ERROR; }

%%
