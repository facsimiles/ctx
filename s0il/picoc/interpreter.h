/* picoc main header file - this has all the main data structures and 
 * function prototypes. If you're just calling picoc you should look at the
 * external interface instead, in picoc.h */
 
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "platform.h"


/* handy definitions */
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef min
#define min(x,y) (((x)<(y))?(x):(y))
#endif

#define MEM_ALIGN(x) (((x) + sizeof(ALIGN_TYPE) - 1) & ~(sizeof(ALIGN_TYPE)-1))

#define GETS_BUF_MAX 256

/* for debugging */
#define PRINT_SOURCE_POS ({ PrintSourceTextErrorLine(Parser->pc->CStdOut, Parser->FileName, Parser->SourceText, Parser->Line, Parser->CharacterPos); PlatformPrintf(Parser->pc->CStdOut, "\n"); })
#define PRINT_TYPE(typ) PlatformPrintf(Parser->pc->CStdOut, "%t\n", typ);

/* small processors use a simplified FILE * for stdio, otherwise use the system FILE * */
#ifdef BUILTIN_MINI_STDLIB
typedef struct OutputStream IOFILE;
#else
typedef FILE IOFILE;
#endif

/* coercion of numeric types to other numeric types */
#ifndef NO_FP
#define IS_FP(v) ((v)->Typ->Base == TypeFP)
#define FP_VAL(v) ((v)->Val->FP)
#else
#define IS_FP(v) 0
#define FP_VAL(v) 0
#endif

#define IS_POINTER_COERCIBLE(v, ap) ((ap) ? ((v)->Typ->Base == TypePointer) : 0)
#define POINTER_COERCE(v) ((int)(v)->Val->Pointer)

#define IS_INTEGER_NUMERIC_TYPE(t) ((t)->Base >= TypeInt && (t)->Base <= TypeUnsignedLong)
#define IS_INTEGER_NUMERIC(v) IS_INTEGER_NUMERIC_TYPE((v)->Typ)
#define IS_NUMERIC_COERCIBLE(v) (IS_INTEGER_NUMERIC(v) || IS_FP(v))
#define IS_NUMERIC_COERCIBLE_PLUS_POINTERS(v,ap) (IS_NUMERIC_COERCIBLE(v) || IS_POINTER_COERCIBLE(v,ap))


struct PcTable;
struct Picoc_Struct;

typedef struct Picoc_Struct Picoc;

/* lexical tokens */
enum LexToken
{
    /* 0x00 */ TokenNone, 
    /* 0x01 */ TokenComma,
    /* 0x02 */ TokenAssign, TokenAddAssign, TokenSubtractAssign, TokenMultiplyAssign, TokenDivideAssign, TokenModulusAssign,
    /* 0x08 */ TokenShiftLeftAssign, TokenShiftRightAssign, TokenArithmeticAndAssign, TokenArithmeticOrAssign, TokenArithmeticExorAssign,
    /* 0x0d */ TokenQuestionMark, TokenColon, 
    /* 0x0f */ TokenLogicalOr, 
    /* 0x10 */ TokenLogicalAnd, 
    /* 0x11 */ TokenArithmeticOr, 
    /* 0x12 */ TokenArithmeticExor, 
    /* 0x13 */ TokenAmpersand, 
    /* 0x14 */ TokenEqual, TokenNotEqual, 
    /* 0x16 */ TokenLessThan, TokenGreaterThan, TokenLessEqual, TokenGreaterEqual,
    /* 0x1a */ TokenShiftLeft, TokenShiftRight, 
    /* 0x1c */ TokenPlus, TokenMinus, 
    /* 0x1e */ TokenAsterisk, TokenSlash, TokenModulus,
    /* 0x21 */ TokenIncrement, TokenDecrement, TokenUnaryNot, TokenUnaryExor, TokenSizeof, TokenCast,
    /* 0x27 */ TokenLeftSquareBracket, TokenRightSquareBracket, TokenDot, TokenArrow, 
    /* 0x2b */ TokenOpenBracket, TokenCloseBracket,
    /* 0x2d */ TokenIdentifier, TokenIntegerConstant, TokenFPConstant, TokenStringConstant, TokenCharacterConstant,
    /* 0x32 */ TokenSemicolon, TokenEllipsis,
    /* 0x34 */ TokenLeftBrace, TokenRightBrace,
    /* 0x36 */ TokenIntType, TokenCharType, TokenFloatType, TokenDoubleType, TokenVoidType, TokenEnumType,
    /* 0x3c */ TokenLongType, TokenSignedType, TokenShortType, TokenStaticType, TokenAutoType, TokenRegisterType, TokenExternType, TokenStructType, TokenUnionType, TokenUnsignedType, TokenTypedef,
    /* 0x46 */ TokenContinue, TokenDo, TokenElse, TokenFor, TokenGoto, TokenIf, TokenWhile, TokenBreak, TokenSwitch, TokenCase, TokenDefault, TokenReturn,
    /* 0x52 */ TokenHashDefine, TokenHashInclude, TokenHashIf, TokenHashIfdef, TokenHashIfndef, TokenHashElse, TokenHashEndif,
    /* 0x59 */ TokenNew, TokenDelete,
    /* 0x5b */ TokenOpenMacroBracket,
    /* 0x5c */ TokenEOF, TokenEndOfLine, TokenEndOfFunction
};

/* used in dynamic memory allocation */
struct AllocNode
{
    unsigned int Size;
    struct AllocNode *NextFree;
};

/* whether we're running or skipping code */
enum RunMode
{
    RunModeRun,                 /* we're running code as we parse it */
    RunModeSkip,                /* skipping code, not running */
    RunModeReturn,              /* returning from a function */
    RunModeCaseSearch,          /* searching for a case label */
    RunModeBreak,               /* breaking out of a switch/while/do */
    RunModeContinue,            /* as above but repeat the loop */
    RunModeGoto                 /* searching for a goto label */
};

/* parser state - has all this detail so we can parse nested files */
struct ParseState
{
    Picoc *pc;                  /* the picoc instance this parser is a part of */
    const unsigned char *Pos;   /* the character position in the source text */
    char *FileName;             /* what file we're executing (registered string) */
    short int Line;             /* line number we're executing */
    short int CharacterPos;     /* character/column in the line we're executing */
    enum RunMode Mode;          /* whether to skip or run code */
    int SearchLabel;            /* what case label we're searching for */
    const char *SearchGotoLabel;/* what goto label we're searching for */
    const char *SourceText;     /* the entire source text */
    short int HashIfLevel;      /* how many "if"s we're nested down */
    short int HashIfEvaluateToLevel;    /* if we're not evaluating an if branch, what the last evaluated level was */
    char DebugMode;             /* debugging mode */
    int ScopeID;                /* for keeping track of local variables (free them after they go out of scope) */
};

/* values */
enum BaseType
{
    TypeVoid,                   /* no type */
    TypeInt,                    /* integer */
    TypeShort,                  /* short integer */
    TypeChar,                   /* a single character (signed) */
    TypeLong,                   /* long integer */
    TypeUnsignedInt,            /* unsigned integer */
    TypeUnsignedShort,          /* unsigned short integer */
    TypeUnsignedChar,           /* unsigned 8-bit number */ /* must be before unsigned long */
    TypeUnsignedLong,           /* unsigned long integer */
#ifndef NO_FP
    TypeFP,                     /* floating point */
#endif
    TypeFunction,               /* a function */
    TypeMacro,                  /* a macro */
    TypePointer,                /* a pointer */
    TypeArray,                  /* an array of a sub-type */
    TypeStruct,                 /* aggregate type */
    TypeUnion,                  /* merged type */
    TypeEnum,                   /* enumerated integer type */
    TypeGotoLabel,              /* a label we can "goto" */
    Type_Type                   /* a type for storing types */
};

/* data type */
struct PcValueType
{
    enum BaseType Base;             /* what kind of type this is */
    int ArraySize;                  /* the size of an array type */
    int Sizeof;                     /* the storage required */
    int AlignBytes;                 /* the alignment boundary of this type */
    const char *Identifier;         /* the name of a struct or union */
    struct PcValueType *FromType;     /* the type we're derived from (or NULL) */
    struct PcValueType *DerivedTypeList;  /* first in a list of types derived from this one */
    struct PcValueType *Next;         /* next item in the derived type list */
    struct PcTable *Members;          /* members of a struct or union */
    int OnHeap;                     /* true if allocated on the heap */
    int StaticQualifier;            /* true if it's a static */
};

/* function definition */
struct FuncDef
{
    struct PcValueType *ReturnType;   /* the return value type */
    int NumParams;                  /* the number of parameters */
    int VarArgs;                    /* has a variable number of arguments after the explicitly specified ones */
    struct PcValueType **ParamType;   /* array of parameter types */
    char **ParamName;               /* array of parameter names */
    void (*Intrinsic)();            /* intrinsic call address or NULL */
    struct ParseState Body;         /* lexical tokens of the function body if not intrinsic */
};

/* macro definition */
struct MacroDef
{
    int NumParams;                  /* the number of parameters */
    char **ParamName;               /* array of parameter names */
    struct ParseState Body;         /* lexical tokens of the function body if not intrinsic */
};

/* values */
union AnyValue
{
    char Character;
    short ShortInteger;
    int Integer;
    long LongInteger;
    unsigned short UnsignedShortInteger;
    unsigned int UnsignedInteger;
    unsigned long UnsignedLongInteger;
    unsigned char UnsignedCharacter;
    char *Identifier;
    char ArrayMem[2];               /* placeholder for where the data starts, doesn't point to it */
    struct PcValueType *Typ;
    struct FuncDef FuncDef;
    struct MacroDef MacroDef;
#ifndef NO_FP
    double FP;
#endif
    void *Pointer;                  /* unsafe native pointers */
};

struct PcValue
{
    struct PcValueType *Typ;          /* the type of this value */
    union AnyValue *Val;            /* pointer to the AnyValue which holds the actual content */
    struct PcValue *LValueFrom;       /* if an LValue, this is a PcValue our LValue is contained within (or NULL) */
    char ValOnHeap;                 /* this PcValue is on the heap */
    char ValOnStack;                /* the AnyValue is on the stack along with this PcValue */
    char AnyValOnHeap;              /* the AnyValue is separately allocated from the PcValue on the heap */
    char IsLValue;                  /* is modifiable and is allocated somewhere we can usefully modify it */
    int ScopeID;                    /* to know when it goes out of scope */
    char OutOfScope;
};

/* hash table data structure */
struct PcTableEntry
{
    struct PcTableEntry *Next;        /* next item in this hash chain */
    const char *DeclFileName;       /* where the variable was declared */
    unsigned short DeclLine;
    unsigned short DeclColumn;

    union PcTableEntryPayload
    {
        struct PcValueEntry
        {
            char *Key;              /* points to the shared string table */
            struct PcValue *Val;      /* the value we're storing */
        } v;                        /* used for tables of values */
        
        char Key[1];                /* dummy size - used for the shared string table */
        
        struct BreakpointEntry      /* defines a breakpoint */
        {
            const char *FileName;
            short int Line;
            short int CharacterPos;
        } b;
        
    } p;
};
    
struct PcTable
{
    short Size;
    short OnHeap;
    struct PcTableEntry **HashTable;
};

/* stack frame for function calls */
struct StackFrame
{
    struct ParseState ReturnParser;         /* how we got here */
    const char *FuncName;                   /* the name of the function we're in */
    struct PcValue *ReturnValue;              /* copy the return value here */
    struct PcValue **Parameter;               /* array of parameter values */
    int NumParams;                          /* the number of parameters */
    struct PcTable LocalTable;                /* the local variables and parameters */
    struct PcTableEntry *LocalHashTable[LOCAL_TABLE_SIZE];
    struct StackFrame *PreviousStackFrame;  /* the next lower stack frame */
};

/* lexer state */
enum LexMode
{
    LexModeNormal,
    LexModeHashInclude,
    LexModeHashDefine,
    LexModeHashDefineSpace,
    LexModeHashDefineSpaceIdent
};

struct PcLexState
{
    const char *Pos;
    const char *End;
    const char *FileName;
    int Line;
    int CharacterPos;
    const char *SourceText;
    enum LexMode Mode;
    int EmitExtraNewlines;
};

/* library function definition */
struct LibraryFunction
{
    void (*Func)(struct ParseState *Parser, struct PcValue *, struct PcValue **, int);
    const char *Prototype;
};

/* output stream-type specific state information */
union OutputStreamInfo
{
    struct StringOutputStream
    {
        struct ParseState *Parser;
        char *WritePos;
    } Str;
};

/* stream-specific method for writing characters to the console */
typedef void CharWriter(unsigned char, union OutputStreamInfo *);

/* used when writing output to a string - eg. sprintf() */
struct OutputStream
{
    CharWriter *Putch;
    union OutputStreamInfo i;
};

/* possible results of parsing a statement */
enum ParseResult { ParseResultEOF, ParseResultError, ParseResultOk };

/* a chunk of heap-allocated tokens we'll cleanup when we're done */
struct CleanupTokenNode
{
    void *Tokens;
    const char *SourceText;
    struct CleanupTokenNode *Next;
};

/* linked list of lexical tokens used in interactive mode */
struct TokenLine
{
    struct TokenLine *Next;
    unsigned char *Tokens;
    int NumBytes;
};


/* a list of libraries we can include */
struct IncludeLibrary
{
    char *IncludeName;
    void (*SetupFunction)(Picoc *pc);
    const struct LibraryFunction *FuncList;
    const char *SetupCSource;
    struct IncludeLibrary *NextLib;
};

#define FREELIST_BUCKETS 8                          /* freelists for 4, 8, 12 ... 32 byte allocs */
#define SPLIT_MEM_THRESHOLD 16                      /* don't split memory which is close in size */
#define BREAKPOINT_TABLE_SIZE 21


/* the entire state of the picoc system */
struct Picoc_Struct
{
    /* parser global data */
    struct PcTable GlobalTable;
    struct CleanupTokenNode *CleanupTokenList;
    struct PcTableEntry *GlobalHashTable[GLOBAL_TABLE_SIZE];
    
    /* lexer global data */
    struct TokenLine *InteractiveHead;
    struct TokenLine *InteractiveTail;
    struct TokenLine *InteractiveCurrentLine;
    int LexUseStatementPrompt;
    union AnyValue LexAnyValue;
    struct PcValue LexValue;
    struct PcTable ReservedWordTable;
    struct PcTableEntry *ReservedWordHashTable[RESERVED_WORD_TABLE_SIZE];

    /* the table of string literal values */
    struct PcTable StringLiteralTable;
    struct PcTableEntry *StringLiteralHashTable[STRING_LITERAL_TABLE_SIZE];
    
    /* the stack */
    struct StackFrame *TopStackFrame;

    /* the value passed to exit() */
    int PicocExitValue;

    /* a list of libraries we can include */
    struct IncludeLibrary *IncludeLibList;

    /* heap memory */
#ifdef USE_MALLOC_STACK
    unsigned char *HeapMemory;          /* stack memory since our heap is malloc()ed */
    void *HeapBottom;                   /* the bottom of the (downward-growing) heap */
    void *StackFrame;                   /* the current stack frame */
    void *HeapStackTop;                 /* the top of the stack */
#else
# ifdef SURVEYOR_HOST
    unsigned char *HeapMemory;          /* all memory - stack and heap */
    void *HeapBottom;                   /* the bottom of the (downward-growing) heap */
    void *StackFrame;                   /* the current stack frame */
    void *HeapStackTop;                 /* the top of the stack */
    void *HeapMemStart;
# else
    unsigned char HeapMemory[HEAP_SIZE];  /* all memory - stack and heap */
    void *HeapBottom;                   /* the bottom of the (downward-growing) heap */
    void *StackFrame;                   /* the current stack frame */
    void *HeapStackTop;                 /* the top of the stack */
# endif
#endif

    struct AllocNode *FreeListBucket[FREELIST_BUCKETS];      /* we keep a pool of freelist buckets to reduce fragmentation */
    struct AllocNode *FreeListBig;                           /* free memory which doesn't fit in a bucket */

    /* types */    
    struct PcValueType UberType;
    struct PcValueType IntType;
    struct PcValueType ShortType;
    struct PcValueType CharType;
    struct PcValueType LongType;
    struct PcValueType UnsignedIntType;
    struct PcValueType UnsignedShortType;
    struct PcValueType UnsignedLongType;
    struct PcValueType UnsignedCharType;
    #ifndef NO_FP
    struct PcValueType FPType;
    #endif
    struct PcValueType VoidType;
    struct PcValueType TypeType;
    struct PcValueType FunctionType;
    struct PcValueType MacroType;
    struct PcValueType EnumType;
    struct PcValueType GotoLabelType;
    struct PcValueType *CharPtrType;
    struct PcValueType *CharPtrPtrType;
    struct PcValueType *CharArrayType;
    struct PcValueType *VoidPtrType;

    /* debugger */
    struct PcTable BreakpointTable;
    struct PcTableEntry *BreakpointHashTable[BREAKPOINT_TABLE_SIZE];
    int BreakpointCount;
    int DebugManualBreak;
    
    /* C library */
    int BigEndian;
    int LittleEndian;

    IOFILE *CStdOut;
#ifdef BUILTIN_MINI_STDLIB
    IOFILE CStdOutBase;
#endif

    /* the picoc version string */
    const char *VersionString;
    
    /* exit longjump buffer */
#if defined(UNIX_HOST) || defined(WIN32)
    jmp_buf PicocExitBuf;
#endif
#ifdef SURVEYOR_HOST
    int PicocExitBuf[41];
#endif
    
    /* string table */
    struct PcTable StringTable;
    struct PcTableEntry *StringHashTable[STRING_TABLE_SIZE];
    char *StrEmpty;
};

/* table.c */
void PcTableInit(Picoc *pc);
char *TableStrRegister(Picoc *pc, const char *Str);
char *TableStrRegister2(Picoc *pc, const char *Str, int Len);
void PcTableInitTable(struct PcTable *Tbl, struct PcTableEntry **HashTable, int Size, int OnHeap);
int TableSet(Picoc *pc, struct PcTable *Tbl, char *Key, struct PcValue *Val, const char *DeclFileName, int DeclLine, int DeclColumn);
int TableGet(struct PcTable *Tbl, const char *Key, struct PcValue **Val, const char **DeclFileName, int *DeclLine, int *DeclColumn);
struct PcValue *TableDelete(Picoc *pc, struct PcTable *Tbl, const char *Key);
char *TableSetIdentifier(Picoc *pc, struct PcTable *Tbl, const char *Ident, int IdentLen);
void TableStrFree(Picoc *pc);

/* lex.c */
void LexInit(Picoc *pc);
void LexCleanup(Picoc *pc);
void *LexAnalyse(Picoc *pc, const char *FileName, const char *Source, int SourceLen, int *TokenLen);
void LexInitParser(struct ParseState *Parser, Picoc *pc, const char *SourceText, void *TokenSource, char *FileName, int RunIt, int SetDebugMode);
enum LexToken LexGetToken(struct ParseState *Parser, struct PcValue **PcValue, int IncPos);
enum LexToken LexRawPeekToken(struct ParseState *Parser);
void LexToEndOfLine(struct ParseState *Parser);
void *LexCopyTokens(struct ParseState *StartParser, struct ParseState *EndParser);
void LexInteractiveClear(Picoc *pc, struct ParseState *Parser);
void LexInteractiveCompleted(Picoc *pc, struct ParseState *Parser);
void LexInteractiveStatementPrompt(Picoc *pc);

/* parse.c */
/* the following are defined in picoc.h:
 * void PicocParse(const char *FileName, const char *Source, int SourceLen, int RunIt, int CleanupNow, int CleanupSource);
 * void PicocParseInteractive(); */
void PicocParseInteractiveNoStartPrompt(Picoc *pc, int EnableDebugger);
enum ParseResult ParseStatement(struct ParseState *Parser, int CheckTrailingSemicolon);
struct PcValue *ParseFunctionDefinition(struct ParseState *Parser, struct PcValueType *ReturnType, char *Identifier);
void ParseCleanup(Picoc *pc);
void ParserCopyPos(struct ParseState *To, struct ParseState *From);
void ParserCopy(struct ParseState *To, struct ParseState *From);

/* expression.c */
int ExpressionParse(struct ParseState *Parser, struct PcValue **Result);
long ExpressionParseInt(struct ParseState *Parser);
void ExpressionAssign(struct ParseState *Parser, struct PcValue *DestValue, struct PcValue *SourceValue, int Force, const char *FuncName, int ParamNo, int AllowPointerCoercion);
long ExpressionCoerceInteger(struct PcValue *Val);
unsigned long ExpressionCoerceUnsignedInteger(struct PcValue *Val);
#ifndef NO_FP
double ExpressionCoerceFP(struct PcValue *Val);
#endif

/* type.c */
void TypeInit(Picoc *pc);
void TypeCleanup(Picoc *pc);
int TypeSize(struct PcValueType *Typ, int ArraySize, int Compact);
int TypeSizeValue(struct PcValue *Val, int Compact);
int TypeStackSizeValue(struct PcValue *Val);
int TypeLastAccessibleOffset(Picoc *pc, struct PcValue *Val);
int TypeParseFront(struct ParseState *Parser, struct PcValueType **Typ, int *IsStatic);
void TypeParseIdentPart(struct ParseState *Parser, struct PcValueType *BasicTyp, struct PcValueType **Typ, char **Identifier);
void TypeParse(struct ParseState *Parser, struct PcValueType **Typ, char **Identifier, int *IsStatic);
struct PcValueType *TypeGetMatching(Picoc *pc, struct ParseState *Parser, struct PcValueType *ParentType, enum BaseType Base, int ArraySize, const char *Identifier, int AllowDuplicates);
struct PcValueType *TypeCreateOpaqueStruct(Picoc *pc, struct ParseState *Parser, const char *StructName, int Size);
int TypeIsForwardDeclared(struct ParseState *Parser, struct PcValueType *Typ);

/* heap.c */
void HeapInit(Picoc *pc, int StackSize);
void HeapCleanup(Picoc *pc);
void *HeapAllocStack(Picoc *pc, int Size);
int HeapPopStack(Picoc *pc, void *Addr, int Size);
void HeapUnpopStack(Picoc *pc, int Size);
void HeapPushStackFrame(Picoc *pc);
int HeapPopStackFrame(Picoc *pc);
void *HeapAllocMem(Picoc *pc, int Size);
void HeapFreeMem(Picoc *pc, void *Mem);

/* variable.c */
void VariableInit(Picoc *pc);
void VariableCleanup(Picoc *pc);
void VariableFree(Picoc *pc, struct PcValue *Val);
void VariableTableCleanup(Picoc *pc, struct PcTable *HashTable);
void *VariableAlloc(Picoc *pc, struct ParseState *Parser, int Size, int OnHeap);
void VariableStackPop(struct ParseState *Parser, struct PcValue *Var);
struct PcValue *VariableAllocValueAndData(Picoc *pc, struct ParseState *Parser, int DataSize, int IsLValue, struct PcValue *LValueFrom, int OnHeap);
struct PcValue *VariableAllocValueAndCopy(Picoc *pc, struct ParseState *Parser, struct PcValue *FromValue, int OnHeap);
struct PcValue *VariableAllocValueFromType(Picoc *pc, struct ParseState *Parser, struct PcValueType *Typ, int IsLValue, struct PcValue *LValueFrom, int OnHeap);
struct PcValue *VariableAllocValueFromExistingData(struct ParseState *Parser, struct PcValueType *Typ, union AnyValue *FromValue, int IsLValue, struct PcValue *LValueFrom);
struct PcValue *VariableAllocValueShared(struct ParseState *Parser, struct PcValue *FromValue);
struct PcValue *VariableDefine(Picoc *pc, struct ParseState *Parser, char *Ident, struct PcValue *InitValue, struct PcValueType *Typ, int MakeWritable);
struct PcValue *VariableDefineButIgnoreIdentical(struct ParseState *Parser, char *Ident, struct PcValueType *Typ, int IsStatic, int *FirstVisit);
int VariableDefined(Picoc *pc, const char *Ident);
int VariableDefinedAndOutOfScope(Picoc *pc, const char *Ident);
void VariableRealloc(struct ParseState *Parser, struct PcValue *FromValue, int NewSize);
void VariableGet(Picoc *pc, struct ParseState *Parser, const char *Ident, struct PcValue **LVal);
void VariableDefinePlatformVar(Picoc *pc, struct ParseState *Parser, char *Ident, struct PcValueType *Typ, union AnyValue *FromValue, int IsWritable);
void VariableStackFrameAdd(struct ParseState *Parser, const char *FuncName, int NumParams);
void VariableStackFramePop(struct ParseState *Parser);
struct PcValue *VariableStringLiteralGet(Picoc *pc, char *Ident);
void VariableStringLiteralDefine(Picoc *pc, char *Ident, struct PcValue *Val);
void *VariableDereferencePointer(struct ParseState *Parser, struct PcValue *PointerValue, struct PcValue **DerefVal, int *DerefOffset, struct PcValueType **DerefType, int *DerefIsLValue);
int VariableScopeBegin(struct ParseState * Parser, int* PrevScopeID);
void VariableScopeEnd(struct ParseState * Parser, int ScopeID, int PrevScopeID);

/* clibrary.c */
void BasicIOInit(Picoc *pc);
void LibraryInit(Picoc *pc);
void LibraryAdd(Picoc *pc, struct PcTable *GlobalTable, const char *LibraryName, const struct LibraryFunction *FuncList);
void CLibraryInit(Picoc *pc);
void PrintCh(char OutCh, IOFILE *Stream);
void PrintSimpleInt(long Num, IOFILE *Stream);
void PrintInt(long Num, int FieldWidth, int ZeroPad, int LeftJustify, IOFILE *Stream);
void PrintStr(const char *Str, IOFILE *Stream);
void PrintFP(double Num, IOFILE *Stream);
void PcPrintType(struct PcValueType *Typ, IOFILE *Stream);
void LibPrintf(struct ParseState *Parser, struct PcValue *ReturnValue, struct PcValue **Param, int NumArgs);

/* platform.c */
/* the following are defined in picoc.h:
 * void PicocCallMain(int argc, char **argv);
 * int PicocPlatformSetExitPoint();
 * void PicocInitialise(int StackSize);
 * void PicocCleanup();
 * void PicocPlatformScanFile(const char *FileName);
 * extern int PicocExitValue; */
void ProgramFail(struct ParseState *Parser, const char *Message, ...);
void ProgramFailNoParser(Picoc *pc, const char *Message, ...);
void AssignFail(struct ParseState *Parser, const char *Format, struct PcValueType *Type1, struct PcValueType *Type2, int Num1, int Num2, const char *FuncName, int ParamNo);
void LexFail(Picoc *pc, struct PcLexState *Lexer, const char *Message, ...);
void PlatformInit(Picoc *pc);
void PlatformCleanup(Picoc *pc);
char *PlatformGetLine(char *Buf, int MaxLen, const char *Prompt);
int PlatformGetCharacter();
void PlatformPutc(unsigned char OutCh, union OutputStreamInfo *);
void PlatformPrintf(IOFILE *Stream, const char *Format, ...);
void PlatformVPrintf(IOFILE *Stream, const char *Format, va_list Args);
void PlatformExit(Picoc *pc, int ExitVal);
char *PlatformMakeTempName(Picoc *pc, char *TempNameBuffer);
void PlatformLibraryInit(Picoc *pc);

/* include.c */
void IncludeInit(Picoc *pc);
void IncludeCleanup(Picoc *pc);
void IncludeRegister(Picoc *pc, const char *IncludeName, void (*SetupFunction)(Picoc *pc), const struct LibraryFunction *FuncList, const char *SetupCSource);
void IncludeFile(Picoc *pc, char *Filename);
/* the following is defined in picoc.h:
 * void PicocIncludeAllSystemHeaders(); */
 
/* debug.c */
void DebugInit();
void DebugCleanup();
void DebugCheckStatement(struct ParseState *Parser);


/* stdio.c */
extern const char StdioDefs[];
extern const struct LibraryFunction StdioFunctions[];
void StdioSetupFunc(Picoc *pc);

/* math.c */
extern const struct LibraryFunction MathFunctions[];
void MathSetupFunc(Picoc *pc);

/* string.c */
extern const struct LibraryFunction StringFunctions[];
void StringSetupFunc(Picoc *pc);

/* stdlib.c */
extern const struct LibraryFunction StdlibFunctions[];
void StdlibSetupFunc(Picoc *pc);

/* time.c */
extern const char StdTimeDefs[];
extern const struct LibraryFunction StdTimeFunctions[];
void StdTimeSetupFunc(Picoc *pc);

/* errno.c */
void StdErrnoSetupFunc(Picoc *pc);

/* ctype.c */
extern const struct LibraryFunction StdCtypeFunctions[];

/* stdbool.c */
extern const char StdboolDefs[];
void StdboolSetupFunc(Picoc *pc);

/* unistd.c */
extern const char UnistdDefs[];
extern const struct LibraryFunction UnistdFunctions[];
void UnistdSetupFunc(Picoc *pc);

/* dirent.c */
extern const char DirentDefs[];
extern const struct LibraryFunction DirentFunctions[];
void DirentSetupFunc(Picoc *pc);

#endif /* INTERPRETER_H */
