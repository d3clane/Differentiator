#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "DSL.h"
#include "MathExpressionsMain.h"
#include "MathExpressionEquationRead.h"
#include "Common/StringFuncs.h"
#include "Vector/Vector.h"

// G       ::= ADD_SUB '\0'
// ADD_SUB ::= MUL_DIV {['+', '-'] MUL_DIV}*
// MUL_DIV ::= POW {['*', '/'] POW}*
// POW     ::= TRIG {['^'] TRIG}*
// TRIG    ::= ['sin', 'cos', 'tan', 'cot', 'arctan', 'arccot', 'arcsin', 'arccos'] '(' ADD_SUB ')' | EXPR
// EXPR    ::= '(' ADD_SUB ')' | ARG
// ARG     ::= NUM | VAR
// VAR     ::= ['a'-'z''A'-'Z''_']+['a'-'z' & 'A'-'Z' & '_' & '0'-'9']*
// NUM     ::= ['0'-'9']+

static size_t ParseDigit(const char* str, const size_t posStart, const size_t line, 
                                                                VectorType* tokens)
{
    assert(str);
    assert(tokens);

    size_t pos = posStart;

    int val = 0;
    while (isdigit(str[pos]))
    {
        val = val * 10 + str[pos] - '0';
        ++pos;
    }

    VectorPush(tokens, TokenCreate(TokenValueCreate(val), TokenValueType::VALUE, line, pos));

    return pos;
}

static size_t ParseWord(const char* str, const size_t posStart, const size_t line, 
                                                              VectorType* tokens)
{
    assert(str);
    assert(tokens);
    
    size_t pos = posStart;

    static const size_t maxWordSize   =  64;
    static char word[maxWordSize + 1] =  "";
    
    size_t wordPos = 0;

    while (isalpha(str[pos]) || isdigit(str[pos]) || str[pos] == '_')
    {
        assert(wordPos < maxWordSize);

        word[wordPos] = str[pos];
        ++pos;
        ++wordPos;
    }

    word[wordPos] = '\0';

    #define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, SHORT_NAME, ...)                   \
        if (strcmp(word, SHORT_NAME) == 0)                                              \
        {                                                                               \
            VectorPush(tokens,                                                          \
                TokenCreate(TokenValueCreate(word), TokenValueType::OPERATION,          \
                                                                line, posStart));       \
            return pos;                                                                 \
        }
    
    //GENERATING if(strcmp(word, "+") == 0) ... if...
    #include "Operations.h"

    //Now - word is not a special symbol

    VectorPush(tokens, TokenCreate(TokenValueCreate(word), 
                                        TokenValueType::VARIABLE, line, posStart));

    return pos;
}

static size_t ParseChar(const char* str, const size_t posStart, const size_t line, 
                                                                VectorType* tokens)
{
    assert(str);
    assert(tokens);
    
    static const size_t  charWordLength  =  2;
    static char charWord[charWordLength] = "";

    charWord[0] = str[posStart];
    charWord[1] = '\0';
    
    //printf("H1 sz - %zu\n", tokens->size);
    VectorPush(tokens, TokenCreate(TokenValueCreate(charWord), TokenValueType::OPERATION,
                                                                            line, posStart));
    //printf("H2 sz - %zu\n", tokens->size);                                                                            

    return posStart + 1;
}

ExpressionErrors ParseOnTokens(const char* str)
{
    size_t pos  = 0;
    size_t line = 0;

    VectorType tokens = {};
    VectorCtor(&tokens);

    //TODO: recopy \0 to tokens
    while (str[pos] != '\0')
    {
        switch (str[pos])
        {
            // как это можно сделать - я читаю данное мне выражение, вижу -, смотрю в массив токенов 
            // до меня, который я уже построил. Предыдущее должно быть числом если это не унарник
            // иначе это унарник
            //TODO: +, - могут быть унарными операторами, можно просто пихать прям сразу в число
            case '+':
            case '-':
            case '*':
            case '/':
            case '^':
            {
                pos = ParseChar(str, pos, line,&tokens);
                break;
            }

            case '(':
            {
                VectorPush(&tokens, TokenCreate(TokenValueCreate("("), 
                                                    TokenValueType::OPERATION, line, pos));
                ++pos;
                break;
            }
            case ')':
            {
                VectorPush(&tokens, TokenCreate(TokenValueCreate(")"), 
                                                    TokenValueType::OPERATION, line, pos));
                ++pos;
                break;
            }

            case '\t':
            case ' ':
            {
                const char* strPtr = SkipSymbolsWhileStatement(str + pos, isspace);
                pos = strPtr - str;
                break;
            }
            case '\n':
            {
                const char* strPtr = SkipSymbolsWhileStatement(str + pos, isspace);
                pos = strPtr - str;
                line++;
                break;
            }

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                pos = ParseDigit(str, pos, line, &tokens);
                break;
            }

            default:
            {
                if (isalpha(str[pos]) || str[pos] == '_')
                {
                    pos = ParseWord(str, pos, line, &tokens);
                    break;
                }

                //TODO: здесь syn_assert очев, такого не бывает же
                assert(0 == 1);
                break;
            }
        }
    }

    for (size_t i = 0; i < tokens.size; ++i)
    {
        switch (tokens.data[i].valueType)
        {
            case TokenValueType::OPERATION:
                printf("Operation - %s\n", tokens.data[i].value.word);
                break;
            case TokenValueType::VARIABLE:
                printf("Variable - %s\n", tokens.data[i].value.word);
                break;
            case TokenValueType::VALUE:
                printf("Value - %lf\n", tokens.data[i].value.val);
                break;
            default:
                abort();
                break;
        }
    }
    
    VectorPush(&tokens, TokenCreate(TokenValueCreate("\0"), TokenValueType::OPERATION, 
                                                                            line, pos));
    VectorDtor(&tokens);

    return ExpressionErrors::NO_ERR;
}

TokenType TokenCreate(TokenValue value, TokenValueType valueType,   const size_t line, 
                                                                    const size_t pos)
{
    TokenType token = {};

    token.value     = value;
    token.valueType = valueType;
    token.line      =      line;
    token.pos       =       pos;

    return token;
}

TokenType TokenCopy(const TokenType* token)
{
    return TokenCreate(token->value, token->valueType, token->line, token->pos);
}

TokenValue TokenValueCreate(double value)
{
    TokenValue val =
    {
        .val = value,
    };

    return val;
}

TokenValue TokenValueCreate(const char* word)
{
    TokenValue val =
    {
        .word = strdup(word),
    };

    return val;
}
