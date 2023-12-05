#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "DSL.h"
#include "MathExpressionsMain.h"
#include "MathExpressionEquationRead.h"
#include "Common/StringFuncs.h"
#include "Vector/Vector.h"
#include "Common/Colors.h"


// G       ::= ADD_SUB '\0'
// ADD_SUB ::= MUL_DIV {['+', '-'] MUL_DIV}*
// MUL_DIV ::= POW {['*', '/'] POW}*
// POW     ::= TRIG {['^'] TRIG}*
// TRIG    ::= ['sin', 'cos', 'tan', 'cot', 'arctan', 'arccot', 'arcsin', 'arccos'] '(' ADD_SUB ')' | EXPR
// EXPR    ::= '(' ADD_SUB ')' | ARG
// ARG     ::= NUM | VAR
// VAR     ::= ['a'-'z''A'-'Z''_']+['a'-'z' & 'A'-'Z' & '_' & '0'-'9']*
// NUM     ::= ['0'-'9']+

typedef VectorType TokensArrType;

static ExpressionErrors ParseOnTokens(const char* str, TokensArrType* tokens);

struct DescentStorage
{
    TokensArrType tokens;
    size_t tokenPos;

    ExpressionVariablesArrayType varsArr;
};

static void DescentStorageCtor(DescentStorage* storage);

//DOESN'T DTOR VARS_ARR, because it's better just to copy it to my expression
static void DescentStorageDtor(DescentStorage* storage);

static ExpressionTokenType* GetG            (DescentStorage* storage);
static ExpressionTokenType* GetAddSub       (DescentStorage* storage);
static ExpressionTokenType* GetMulDiv       (DescentStorage* storage);
static ExpressionTokenType* GetPow          (DescentStorage* storage);
static ExpressionTokenType* GetExpression   (DescentStorage* storage);
static ExpressionTokenType* GetArgument     (DescentStorage* storage);
static ExpressionTokenType* GetNum          (DescentStorage* storage);
static ExpressionTokenType* GetTrig         (DescentStorage* storage);
static ExpressionTokenType* GetVariable     (DescentStorage* storage);

#define T_CRT_VAL(val)  TokenValueCreate(val);
#define  T_OP_TYPE_CNST TokenValueType::OPERATION
#define T_NUM_TYPE_CNST TokenValueType::VALUE
#define T_VAR_TYPE_CNST TokenValueType::VARIABLE

#define POS(storage) storage->tokenPos

static inline const char* T_WORD(const DescentStorage* storage)
{
    return storage->tokens.data[storage->tokenPos].value.word;
}

static inline const char* T_WORD(const DescentStorage* storage, const size_t pos)
{
    return storage->tokens.data[pos].value.word;
}

static inline double T_NUM(const DescentStorage* storage)
{
    return storage->tokens.data[storage->tokenPos].value.val;
}

static inline double T_NUM(const DescentStorage* storage, const size_t pos)
{
    return storage->tokens.data[pos].value.val;
}

static inline bool T_IS_OP(const DescentStorage* storage)
{
    return storage->tokens.data[storage->tokenPos].valueType == T_OP_TYPE_CNST;
}

static inline bool T_IS_OP(const DescentStorage* storage, const size_t pos)
{
    return storage->tokens.data[pos].valueType == T_OP_TYPE_CNST;
}

static inline bool T_IS_NUM(const DescentStorage* storage)
{
    return storage->tokens.data[storage->tokenPos].valueType == T_NUM_TYPE_CNST;
}

static inline bool T_IS_NUM(const DescentStorage* storage, const size_t pos)
{
    return storage->tokens.data[pos].valueType == T_NUM_TYPE_CNST;
}

static inline bool T_IS_VAR(const DescentStorage* storage)
{
    return storage->tokens.data[storage->tokenPos].valueType == T_VAR_TYPE_CNST;
}

static inline bool T_IS_VAR(const DescentStorage* storage, const size_t pos)
{
    return storage->tokens.data[pos].valueType == T_VAR_TYPE_CNST;
}

static inline bool T_CMP_WORD(const DescentStorage* storage, const char* str)
{
    return (strcmp(T_WORD(storage), str) == 0);
}

static inline bool T_CMP_WORD(const DescentStorage* storage, const size_t pos, const char* str)
{
    return (strcmp(T_WORD(storage, pos), str) == 0);
}

static size_t ParseDigit(const char* str, const size_t posStart, const size_t line, 
                                                                TokensArrType* tokens)
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
                                                              TokensArrType* tokens)
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
                                                                TokensArrType* tokens)
{
    assert(str);
    assert(tokens);
    
    static const size_t  charWordLength  =  2;
    static char charWord[charWordLength] = "";

    charWord[0] = str[posStart];
    charWord[1] = '\0';

    VectorPush(tokens, TokenCreate(TokenValueCreate(charWord), TokenValueType::OPERATION,
                                                                            line, posStart));

    return posStart + 1;
}

ExpressionType ExpressionRead(const char* str)
{
    assert(str);

    ExpressionType expression = {};

    DescentStorage storage = {};
    DescentStorageCtor(&storage);

    ParseOnTokens(str, &storage.tokens);
    expression.root = GetG(&storage);

    expression.variables = storage.varsArr;
    //DescentStorageDtor(&storage);
    ExpressionGraphicDump(&expression, true);
    return expression;
}

static ExpressionErrors ParseOnTokens(const char* str, TokensArrType* tokens)
{
    size_t pos  = 0;
    size_t line = 0;

    while (str[pos] != '\0')
    {
        switch (str[pos])
        {
            // как это можно сделать - я читаю данное мне выражение, вижу -, смотрю в массив токенов 
            // до меня, который я уже построил. Предыдущее должно быть не операцией если это не унарник
            // иначе это унарник, тогда можно считать это как число тип 
            //(удобнее унарник перевести в умножение на (-1) в случае переменной, мне кажется).
            //TODO: +, - могут быть унарными операторами, можно просто пихать прям сразу в число
            case '+':
            case '-':
            case '*':
            case '/':
            case '^':
            {
                pos = ParseChar(str, pos, line, tokens);
                break;
            }

            case '(':
            {
                VectorPush(tokens, TokenCreate(TokenValueCreate("("), 
                                                    TokenValueType::OPERATION, line, pos));
                ++pos;
                break;
            }
            case ')':
            {
                VectorPush(tokens, TokenCreate(TokenValueCreate(")"), 
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
                pos = ParseDigit(str, pos, line, tokens);
                break;
            }

            default:
            {
                if (isalpha(str[pos]) || str[pos] == '_')
                {
                    pos = ParseWord(str, pos, line, tokens);
                    break;
                }

                //TODO: здесь syn_assert очев, такого не бывает же
                assert(0 == 1);
                break;
            }
        }
    }

    /*for (size_t i = 0; i < tokens.size; ++i)
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
    }*/
    
    VectorPush(tokens, TokenCreate(TokenValueCreate("\0"), TokenValueType::OPERATION, 
                                                                            line, pos));

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

//-------------------Recursive descent-----------------

#define SyntaxAssert(statement) SynAssert(statement, __FILE__, __func__, __LINE__)
static inline void SynAssert(bool statement, const char* fileName, const char* funcName, const int line) 
                                //const char* string, const size_t line, const size_t pos)
{
    if (statement)
        return;

    printf("File - %s, func - %s, line - %d\n", fileName, funcName, line);

    assert(false);
    //assert(string);

    //printf(RED_TEXT("Syntax error in line %zu, pos %zu, string - %s"), line, pos, string);
}

static ExpressionTokenType* GetG(DescentStorage* storage)
{
    storage->tokenPos = 0;
    ExpressionTokenType* token = GetAddSub(storage);

    SyntaxAssert(T_IS_OP(storage) && T_CMP_WORD(storage, "\0"));

    //printf("Operation - %s, next op - %s, %d\n", T_WORD(storage), T_WORD(storage, storage->tokenPos + 1));
    return  token;
}

static ExpressionTokenType* GetAddSub(DescentStorage* storage)
{
    assert(storage);

    ExpressionTokenType* mainToken = GetMulDiv(storage);

    while (T_IS_OP(storage) && (T_CMP_WORD(storage, "+") || T_CMP_WORD(storage, "-")))
    {
        size_t operationPos = POS(storage);
        POS(storage)++;
        ExpressionTokenType* tmpToken = GetMulDiv(storage);

        if (T_CMP_WORD(storage, operationPos, "+"))
            mainToken = _ADD(mainToken, tmpToken);
        else if (T_CMP_WORD(storage, operationPos, "-"))
            mainToken = _SUB(mainToken, tmpToken);
        else
            SyntaxAssert(false);
    }
    
    return mainToken;
}

static ExpressionTokenType* GetMulDiv(DescentStorage* storage)
{
    assert(storage);

    ExpressionTokenType* mainToken = GetPow(storage);

    while (T_IS_OP(storage) && (T_CMP_WORD(storage, "*") || T_CMP_WORD(storage, "/")))
    {
        size_t operationPos = POS(storage);
        POS(storage)++;
        ExpressionTokenType* tmpToken = GetPow(storage);

        if (T_CMP_WORD(storage, operationPos, "*"))
            mainToken = _MUL(mainToken, tmpToken);
        else if (T_CMP_WORD(storage, operationPos, "/"))
            mainToken = _DIV(mainToken, tmpToken);
        else
            SyntaxAssert(false);
    }

    return mainToken;
}

static ExpressionTokenType* GetPow(DescentStorage* storage)
{
    assert(storage);

    ExpressionTokenType* mainToken = GetTrig(storage);
    ExpressionTokenType* powToken  = nullptr;
    while (T_IS_OP(storage) && T_CMP_WORD(storage, "^"))
    {
        size_t operationPos = POS(storage);
        POS(storage)++;
        ExpressionTokenType* tmpToken = GetTrig(storage);

        if (T_CMP_WORD(storage, operationPos, "^"))
        {
            if (powToken == nullptr) powToken = tmpToken;
            else                     powToken = _POW(powToken, tmpToken);
        }
        else
            SyntaxAssert(false);
    }

    if (powToken)
        mainToken = _POW(mainToken, powToken);

    return mainToken;    
}

static ExpressionTokenType* GetTrig(DescentStorage* storage)
{
    assert(storage);

    if (T_IS_OP(storage)     && ((T_CMP_WORD(storage, "sin"))       || 
                                 (T_CMP_WORD(storage, "cos"))       ||
                                 (T_CMP_WORD(storage, "tan"))       ||
                                 (T_CMP_WORD(storage, "cot"))       ||
                                 (T_CMP_WORD(storage, "arcsin"))    ||
                                 (T_CMP_WORD(storage, "arccos"))    ||
                                 (T_CMP_WORD(storage, "arctan"))    ||
                                 (T_CMP_WORD(storage, "arccot"))))
    {
        size_t operationPos = POS(storage);
        POS(storage)++;
        SyntaxAssert(T_IS_OP(storage) && T_CMP_WORD(storage,"("));
        POS(storage)++;

        ExpressionTokenType* mainToken = GetAddSub(storage);

        SyntaxAssert(T_IS_OP(storage) && T_CMP_WORD(storage, ")"));
        POS(storage)++;

        assert(T_IS_OP(storage, operationPos));

        if (T_CMP_WORD(storage, operationPos, "sin"))
            return _SIN(mainToken);
        else if (T_CMP_WORD(storage, operationPos, "cos"))
            return _COS(mainToken);
        else if (T_CMP_WORD(storage, operationPos, "tan"))
            return _TAN(mainToken);
        else if (T_CMP_WORD(storage, operationPos, "cot"))
            return _COT(mainToken);
        else if (T_CMP_WORD(storage, operationPos, "arcsin"))
            return _ARCSIN(mainToken);
        else if (T_CMP_WORD(storage, operationPos, "arccos"))
            return _ARCCOS(mainToken);
        else if (T_CMP_WORD(storage, operationPos, "arctan"))
            return _ARCTAN(mainToken);
        else if (T_CMP_WORD(storage, operationPos, "arccot"))
            return _ARCCOT(mainToken);
        else
            SyntaxAssert(false);
    }

    ExpressionTokenType* mainToken = GetExpression(storage);

    return mainToken;
}

static ExpressionTokenType* GetExpression(DescentStorage* storage)
{
    assert(storage);

    ExpressionTokenType* mainToken = nullptr;
    if (T_IS_OP(storage) && T_CMP_WORD(storage, "("))
    {
        POS(storage)++;

        mainToken = GetAddSub(storage);
        SyntaxAssert(T_IS_OP(storage) && T_CMP_WORD(storage, ")"));
        POS(storage)++;

        return mainToken;
    }

    mainToken = GetArgument(storage);

    return mainToken;
}

static ExpressionTokenType* GetArgument(DescentStorage* storage)
{
    assert(storage);

    ExpressionTokenType* mainToken = nullptr;

    if (T_IS_NUM(storage))
        return GetNum(storage);

    return GetVariable(storage);
}

static ExpressionTokenType* GetNum(DescentStorage* storage)
{
    assert(storage);

    SyntaxAssert(T_IS_NUM(storage));

    ExpressionTokenType* mainToken = CRT_NUM(T_NUM(storage));

    POS(storage)++;

    return mainToken;
}

static ExpressionTokenType* GetVariable(DescentStorage* storage)
{
    assert(storage);

    SyntaxAssert(T_IS_VAR(storage));

    ExpressionTokenType* mainToken = CRT_VAR(&storage->varsArr, T_WORD(storage));

    POS(storage)++;

    return mainToken;
}

static void DescentStorageCtor(DescentStorage* storage)
{
    VectorCtor(&storage->tokens);
    storage->tokenPos  = 0;
    ExpressionVariableArrayCtor(&storage->varsArr);
}

static void DescentStorageDtor(DescentStorage* storage)
{
    VectorDtor(&storage->tokens);
    storage->tokenPos = 0;
}
