#if !defined(_LANGUAGE_IRGEN_H_)
#define _LANGUAGE_IRGEN_H_
#include "ast.h"
#include "sema.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

/*
# object $someGlobalVar, type u32 = 10;
# function $printf(#0 type u8*, #1 type Variadic) {
#     #2 = reserve type i32
#     #3 = mul type i32 2, type i32 3
#     #4 = load $someGlobalVar, type i32
#     #5 = add type i32 #3, type i32 #4
#     store type ptr #2, type i32 #5
#     #6 = reserve type i32
#     #7 = load #2, type i32
#     #8 = mul type i32 #7, type i32 2
#     store type ptr #3, type i32 #8
#     #9 = load #7, type i32
#     ret #9, type i32
# }
*/

namespace language {
enum struct IrTypeType {
    I32,
    I64,
    String,
    Variable,
    Variadic,
    Pointer,
    Label,
    Custom,
};
struct IrType {
    IrTypeType  type;
    std::string name;
    void        print();
};
enum struct IrOperandType {
    ConstI32,
    ConstI64,
    Type,
    SSA,
    Name,
    Label,
};
struct IrOperand {
    IrOperandType type;
    IrType*       irType;
    int32_t       constI32;
    int64_t       constI64;
    size_t        ssaResult;
    std::string   name;
    void          print();
};
enum struct IrInstructionType {
    Reserve,
    Store,
    Load,

    Trunc,
    Sext,
    Zext,

    Const,

    Add,
    Mul,

    Return,
    Br,
};
struct IrInstruction {
    std::optional<size_t>   result;
    IrInstructionType       type;
    std::vector<IrOperand*> operands;
    void                    print(size_t indent);
};
struct IrBlock {
    std::string                 name;
    std::vector<IrInstruction*> insts;
    void                        print(size_t indent);
};
struct IrFunction {
    std::string                             name;
    std::vector<std::pair<IrType*, size_t>> arguments;
    std::unordered_map<std::string, size_t> nameToSSANumber;
    std::vector<IrInstruction*>             entryInsts;
    std::vector<IrBlock*>                   blocks;
    void                                    print(size_t indent);
};
struct IrObject {
    std::string name;
    IrOperand*  value;
    IrType*     type;
    void        print(size_t indent);
};
struct IrModule {
    std::vector<IrFunction*> functions;
    std::vector<IrObject*>   objects;
    void                     print();
};
class IrGen {
  public:
    IrGen(Ast* ast);
    ~IrGen();
    void      generate();
    IrModule* getModule();

  private:
    IrObject*                            emitTopVariableDecl(VariableDeclarationNode* node);
    IrFunction*                          emitTopFunctionDecl(FunctionDeclarationNode* node);
    std::variant<IrFunction*, IrObject*> emitTopDeclaration(DeclarationNode* node);
    std::variant<IrFunction*, IrObject*> emitNode(AstNode* node);
    std::pair<std::vector<std::pair<IrType*, size_t>>, std::unordered_map<std::string, size_t>>
                                constructFuncArgs(std::vector<DeclarationNode*> nodes);
    IrType*                     generateType(TypeSpec* type);
    IrOperand*                  generateOperand(ExpressionNode* expr);
    std::vector<IrInstruction*> genInstsFromExpr(ExpressionNode* node);
    std::vector<IrBlock*>       generateCompoundBlocks(CompoundStatementNode* node);
    std::vector<IrBlock*>       generateBlocks(StatementNode* node);
    Ast*                        inAst;
    IrModule*                   outModule;
    IrFunction*                 currentFunc;
};
}; // namespace language

#endif // _LANGUAGE_IRGEN_H_
