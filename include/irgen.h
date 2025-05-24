#if !defined(_LANGUAGE_IRGEN_H_)
#define _LANGUAGE_IRGEN_H_
#include "ast.h"

#include <cstdint>
#include <string>
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
#     store type ptr #7, type i32 #8
#     #9 = load #7, type i32
#     ret #9, type i32
# }
*/

namespace language {
enum struct IrTypeType {
    I32,
    U32,
    String,
    Variable,
    Custom,
};
struct IrType {
    IrTypeType  type;
    std::string name;
};
enum struct IrOperandType {
    ConstU32,
};
struct IrOperand {
    IrOperandType type;
    IrType*       irType;
    union {
        uint32_t constU32;
    };
};
enum struct IrInstructionType {};
struct IrInstruction {
    IrInstructionType       type;
    std::vector<IrOperand*> operands;
};
struct IrBlock {
    std::string                 name;
    std::vector<IrInstruction*> insts;
};
struct IrFunction {
    std::string                             name;
    std::vector<std::pair<IrType*, size_t>> arguments;
};
struct IrObject {
    std::string name;
    IrOperand*  value;
    IrType*     type;
};
struct IrModule {
    std::vector<IrFunction*> functions;
    std::vector<IrObject*>   objects;
    void print();
};
class IrGen {
  public:
    IrGen(Ast* ast);
    ~IrGen();
    void      generate();
    IrModule* getModule();

  private:
    Ast*      inAst;
    IrModule* outModule;
};
}; // namespace language

#endif // _LANGUAGE_IRGEN_H_
