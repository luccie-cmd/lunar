#include <irgen.h>

namespace language {
void IrModule::print() {
    std::printf("Module:\n");
    for (IrObject* obj : this->objects) {
        obj->print(0);
    }
    for (IrFunction* func : this->functions) {
        func->print(0);
    }
}
void printIndent(size_t indent) {
    for (size_t i = 0; i < indent; ++i) {
        std::putchar(' ');
    }
}
static std::string convertIrTypeToString(IrTypeType type) {
    switch (type) {
    case IrTypeType::I32: {
        return "i32";
    } break;
    case IrTypeType::I64: {
        return "i64";
    } break;
    case IrTypeType::Void: {
        return "void";
    } break;
    case IrTypeType::String: {
        return "string";
    } break;
    case IrTypeType::Variable: {
        return "variable";
    } break;
    case IrTypeType::Variadic: {
        return "variadic";
    } break;
    case IrTypeType::Pointer: {
        return "pointer";
    } break;
    case IrTypeType::Label: {
        return "label";
    } break;
    case IrTypeType::Custom: {
        return "custom";
    } break;
    default: {
        std::printf("TODO: Convert IrTypeType %llu to string\n", type);
        std::exit(1);
    } break;
    }
}
void IrType::print() {
    std::printf("type %s", convertIrTypeToString(this->type).c_str());
}
void IrOperand::print() {
    this->irType->print();
    switch (this->type) {
    case IrOperandType::ConstI32: {
        std::printf(" %d", this->constI32);
    } break;
    case IrOperandType::ConstI64: {
        std::printf(" %ld", this->constI64);
    } break;
    case IrOperandType::Type: {
        std::printf(" ");
    } break;
    case IrOperandType::SSA: {
        std::printf(" #%lu", this->ssaResult);
    } break;
    case IrOperandType::Name: {
        std::printf(" $%s", this->name.c_str());
    } break;
    case IrOperandType::Label: {
        std::printf(" #%s", this->name.c_str());
    } break;
    default: {
        std::printf("TODO: Print IR operand type %llu\n", this->type);
        std::exit(1);
    } break;
    }
}
std::string irInstructionTypeToString(IrInstructionType type) {
    switch (type) {
    case IrInstructionType::Reserve: {
        return "reserve";
    } break;
    case IrInstructionType::Store: {
        return "store";
    } break;
    case IrInstructionType::Load: {
        return "load";
    } break;
    case IrInstructionType::Sext: {
        return "sext";
    } break;
    case IrInstructionType::Zext: {
        return "zext";
    } break;
    case IrInstructionType::Trunc: {
        return "trunc";
    } break;
    case IrInstructionType::Const: {
        return "const";
    } break;
    case IrInstructionType::Mul: {
        return "mul";
    } break;
    case IrInstructionType::Add: {
        return "add";
    } break;
    case IrInstructionType::Return: {
        return "return";
    } break;
    case IrInstructionType::Br: {
        return "br";
    } break;
    default: {
        std::printf("TODO: irInstructionTypeToString %llu\n", type);
        std::exit(1);
    } break;
    }
}
void IrInstruction::print(size_t indent) {
    printIndent(indent);
    if (this->result.has_value()) {
        std::printf("#%lu = ", this->result.value());
    }
    std::printf("%s ", irInstructionTypeToString(this->type).c_str());
    for (IrOperand* operand : this->operands) {
        operand->print();
        if (operand != this->operands.back()) {
            std::printf(",");
        }
        std::printf(" ");
    }
    std::printf("\n");
}
void IrBlock::print(size_t indent) {
    printIndent(indent);
    std::printf("%s:\n", this->name.c_str());
    for (IrInstruction* inst : this->insts) {
        inst->print(indent + 2);
    }
}
void IrFunction::print(size_t indent) {
    printIndent(indent);
    std::printf("function ");
    this->returnType->print();
    std::printf(" $%s(", this->name.c_str());
    for (std::pair<IrType*, size_t> arg : this->arguments) {
        std::printf("#%lu ", arg.second);
        arg.first->print();
        if (arg != this->arguments.back()) {
            std::printf(", ");
        }
    }
    std::printf(") {\n");
    for (IrInstruction* inst : this->entryInsts) {
        inst->print(4);
    }
    for (IrBlock* block : this->blocks) {
        block->print(2);
    }
    std::printf("}\n");
}
void IrObject::print(size_t indent) {
    printIndent(indent);
    std::printf("object $%s, ", this->name.c_str());
    this->type->print();
    std::printf(" = ");
    this->value->print();
    std::printf("\n");
}
}; // namespace language