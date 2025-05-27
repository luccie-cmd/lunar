#include <irgen.h>

namespace language {
static TypeSpec* getBiggestType(TypeSpec* type1, TypeSpec* type2) {
    if (type1->isInteger() && type2->isInteger()) {
        if (type1->isUnsigned() == type2->isUnsigned()) {
            if (type1->getBitSize() >= type2->getBitSize()) {
                return type1;
            }
            return type2;
        }
        TypeSpec* signedType   = type1->isUnsigned() ? type2 : type1;
        TypeSpec* unsignedType = type1->isUnsigned() ? type1 : type2;
        if (unsignedType->getBitSize() >= signedType->getBitSize()) {
            return unsignedType;
        } else {
            if (signedType->getBitSize() > unsignedType->getBitSize()) {
                return signedType;
            }
            return unsignedType;
        }
    }
    std::printf("ICE: Implicit cast\n");
    std::exit(1);
}
static IrOperand* createSSAOperand(size_t ssaNumber, IrType* type) {
    IrOperand* op = new IrOperand;
    op->type      = IrOperandType::SSA;
    op->irType    = type;
    op->ssaResult = ssaNumber;
    return op;
}
static IrObject* findObjectWithName(std::vector<IrObject*> objects, std::string name) {
    for (IrObject* obj : objects) {
        if (obj->name == name) {
            return obj;
        }
    }
    std::printf("ICE: No object with name `%s`\n", name.c_str());
    std::exit(1);
}
static TypeSpec* convertExpressionToType(std::vector<IrObject*> objects, IrFunction* currentFunc,
                                         ExpressionNode* node) {
    switch (node->getExprType()) {
    case ExpressionNodeType::NumericLiteral: {
        int64_t val = std::stol(reinterpret_cast<NumericLiteralExpressionNode*>(node)->getValue());
        return new TypeSpec(0, val < 0 ? (val + UINT32_MAX >= 0 ? "i32" : "i64")
                                       : (val > UINT32_MAX ? "u64" : "u32"));
    } break;
    case ExpressionNodeType::Unary: {
        return new TypeSpec(0, "i32");
    } break;
    case ExpressionNodeType::Binary: {
        BinaryExpressionNode* binNode = reinterpret_cast<BinaryExpressionNode*>(node);
        TypeSpec* lhs = convertExpressionToType(objects, currentFunc, binNode->getLhs());
        TypeSpec* rhs = convertExpressionToType(objects, currentFunc, binNode->getRhs());
        return getBiggestType(lhs, rhs);
    } break;
    case ExpressionNodeType::LtoRValue: {
        return convertExpressionToType(objects, currentFunc,
                                       reinterpret_cast<LtoRValueCastExpression*>(node)->getExpr());
    } break;
    case ExpressionNodeType::Cast: {
        return reinterpret_cast<CastExpressionNode*>(node)->getType();
    } break;
    case ExpressionNodeType::IdentifierLiteral: {
        if (currentFunc->nameToSSANumber.contains(
                reinterpret_cast<IdentifierLiteralExpressionNode*>(node)->getValue())) {
            IrInstruction* inst;
            for (IrInstruction* blockInst : currentFunc->entryInsts) {
                if (blockInst->result.has_value() &&
                    blockInst->result.value() ==
                        currentFunc->nameToSSANumber.at(
                            reinterpret_cast<IdentifierLiteralExpressionNode*>(node)->getValue())) {
                    inst = blockInst;
                    break;
                }
            }
            if (!inst || inst->type != IrInstructionType::Reserve || inst->operands.size() < 1) {
                std::printf(
                    "ICE: Invalid point to name SSA `%s`\n",
                    reinterpret_cast<IdentifierLiteralExpressionNode*>(node)->getValue().c_str());
                std::exit(1);
            }
            return new TypeSpec(inst->operands.at(0)->irType->name == "ptr" ? 1 : 0,
                                inst->operands.at(0)->irType->name);
        } else {
            return new TypeSpec(
                findObjectWithName(
                    objects, reinterpret_cast<IdentifierLiteralExpressionNode*>(node)->getValue())
                            ->type->name == "ptr"
                    ? 1
                    : 0,
                findObjectWithName(
                    objects, reinterpret_cast<IdentifierLiteralExpressionNode*>(node)->getValue())
                    ->type->name);
        }
    } break;
    default: {
        std::printf("ICE: Unhandled expression node type for conversion %llu\n",
                    node->getExprType());
        std::exit(1);
    } break;
    }
}
static IrOperand* createConstI32Operand(int32_t value) {
    IrOperand* op = new IrOperand;
    op->type      = IrOperandType::ConstI32;
    op->irType    = new IrType(IrTypeType::I32, "i32");
    op->constI32  = value;
    return op;
}
static IrOperand* createConstI64Operand(int64_t value) {
    IrOperand* op = new IrOperand;
    op->type      = IrOperandType::ConstI64;
    op->irType    = new IrType(IrTypeType::I64, "i64");
    op->constI64  = value;
    return op;
}
static IrOperand* createConstU32Operand(uint32_t value) {
    IrOperand* op = new IrOperand;
    op->type      = IrOperandType::ConstU32;
    op->irType    = new IrType(IrTypeType::U32, "u32");
    op->constU32  = value;
    return op;
}
static IrOperand* createConstU64Operand(uint64_t value) {
    IrOperand* op = new IrOperand;
    op->type      = IrOperandType::ConstU64;
    op->irType    = new IrType(IrTypeType::U64, "u64");
    op->constU64  = value;
    return op;
}
static IrOperand* createTypeOperand(IrType* type) {
    IrOperand* op = new IrOperand;
    op->type      = IrOperandType::Type;
    op->irType    = type;
    return op;
}
static IrOperand* createNameOperand(std::string name, IrType* type) {
    IrOperand* op = new IrOperand;
    op->type      = IrOperandType::Name;
    op->irType    = type;
    op->name      = name;
    return op;
}
static IrOperand* createLabelOperand(std::string label) {
    IrOperand* op = new IrOperand;
    op->type      = IrOperandType::Label;
    op->name      = label;
    op->irType    = new IrType(IrTypeType::Label, "label");
    return op;
}
static size_t blockNumbers = 0;
IrGen::IrGen(Ast* ast) {
    this->inAst = ast;
}
IrObject* IrGen::emitTopVariableDecl(VariableDeclarationNode* node) {
    IrObject* obj = new IrObject;
    obj->name     = node->getName();
    obj->type     = this->generateType(node->getType());
    obj->value    = this->generateOperand(node->getValue().value());
    return obj;
}
static size_t ssaResults = 0;
IrFunction*   IrGen::emitTopFunctionDecl(FunctionDeclarationNode* node) {
    IrFunction* func  = new IrFunction;
    this->currentFunc = func;
    func->name        = node->getName();
    std::pair<std::vector<std::pair<IrType*, size_t>>, std::unordered_map<std::string, size_t>>
        tempArgs                  = this->constructFuncArgs(node->getParams());
    func->arguments               = tempArgs.first;
    func->nameToSSANumber         = tempArgs.second;
    ssaResults                    = func->arguments.size();
    blockNumbers                  = 0;
    func->blocks                  = this->generateBlocks(node->getBody());
    IrInstruction* terminatorInst = new IrInstruction;
    terminatorInst->type          = IrInstructionType::Br;
    terminatorInst->operands      = {createLabelOperand(".BB0")};
    func->entryInsts.push_back(terminatorInst);
    return func;
}
std::variant<IrFunction*, IrObject*> IrGen::emitTopDeclaration(DeclarationNode* node) {
    switch (node->getDeclType()) {
    case DeclarationNodeType::Variable: {
        return this->emitTopVariableDecl(reinterpret_cast<VariableDeclarationNode*>(node));
    } break;
    case DeclarationNodeType::Function: {
        return this->emitTopFunctionDecl(reinterpret_cast<FunctionDeclarationNode*>(node));
    } break;
    default: {
        std::printf("TODO: Top level declaration emit of node type %llu\n", node->getDeclType());
        std::exit(1);
    } break;
    }
}
std::variant<IrFunction*, IrObject*> IrGen::emitNode(AstNode* node) {
    switch (node->getAstType()) {
    case AstNodeType::Declaration: {
        return this->emitTopDeclaration(reinterpret_cast<DeclarationNode*>(node));
    } break;
    default: {
        std::printf("TODO: Top level emit of node type %llu\n", node->getAstType());
        std::exit(1);
    } break;
    }
}
void IrGen::generate() {
    this->outModule = new IrModule;
    for (AstNode* node : this->inAst->getNodes()) {
        std::variant<IrFunction*, IrObject*> irFunctionOperand = this->emitNode(node);
        if (irFunctionOperand.index() == 0) {
            this->outModule->functions.push_back(std::get<IrFunction*>(irFunctionOperand));
        } else if (irFunctionOperand.index() == 1) {
            this->outModule->objects.push_back(std::get<IrObject*>(irFunctionOperand));
        }
    }
}
IrType* IrGen::generateType(TypeSpec* type) {
    if (type->getPointerCount() > 0) {
        return new IrType(IrTypeType::Pointer, "ptr");
    }
    if (type->getName() == "String") {
        return new IrType(IrTypeType::String, "string");
    }
    if (type->getName() == "Variadic") {
        return new IrType(IrTypeType::Variadic, "variadic");
    }
    if (type->getBitSize() == 32 && type->isInteger()) {
        return new IrType(type->isUnsigned() ? IrTypeType::U32 : IrTypeType::I32, type->getName());
    }
    if (type->getBitSize() == 64 && type->isInteger()) {
        return new IrType(type->isUnsigned() ? IrTypeType::U64 : IrTypeType::I64, type->getName());
    }
    std::printf("TODO: Generate type for typespec name `%s`\n", type->getName().c_str());
    std::exit(1);
}
IrOperand* IrGen::generateOperand(ExpressionNode* expr) {
    switch (expr->getExprType()) {
    case ExpressionNodeType::NumericLiteral: {
        NumericLiteralExpressionNode* numExpr =
            reinterpret_cast<NumericLiteralExpressionNode*>(expr);
        int64_t val = std::stol(numExpr->getValue());
        return val < 0 ? (val + UINT32_MAX >= 0 ? createConstI32Operand(val)
                                                : createConstI64Operand(val))
                       : (val > UINT32_MAX ? createConstU64Operand(std::stoul(numExpr->getValue()))
                                           : createConstU32Operand(val));
    } break;
    case ExpressionNodeType::Cast: {
        IrOperand* actualOp =
            this->generateOperand(reinterpret_cast<CastExpressionNode*>(expr)->getValue());
        actualOp->irType =
            this->generateType(reinterpret_cast<CastExpressionNode*>(expr)->getType());
        return actualOp;
    } break;
    case ExpressionNodeType::IdentifierLiteral: {
        std::string name = reinterpret_cast<IdentifierLiteralExpressionNode*>(expr)->getValue();
        if (this->currentFunc->nameToSSANumber.contains(name)) {
            return createSSAOperand(this->currentFunc->nameToSSANumber.at(name),
                                    new IrType(IrTypeType::Pointer, "ptr"));
        } else {
            return createNameOperand(findObjectWithName(this->outModule->objects, name)->name,
                                     new IrType(IrTypeType::Pointer, "ptr"));
        }
    } break;
    default: {
        std::printf("TODO: Generate operand for expression type %llu\n", expr->getExprType());
        std::exit(1);
    } break;
    }
}
std::pair<std::vector<std::pair<IrType*, size_t>>, std::unordered_map<std::string, size_t>>
IrGen::constructFuncArgs(std::vector<DeclarationNode*> nodes) {
    std::pair<std::vector<std::pair<IrType*, size_t>>, std::unordered_map<std::string, size_t>>
        args;
    for (DeclarationNode* node : nodes) {
        ParameterDeclarationNode* paramDeclNode = reinterpret_cast<ParameterDeclarationNode*>(node);
        paramDeclNode->getType();
        size_t number = args.first.size();
        args.first.push_back({this->generateType(paramDeclNode->getType()), number});
        args.second.insert({paramDeclNode->getName(), number});
    }
    return args;
}
size_t newSSAResult() {
    return ssaResults++;
}
std::vector<IrInstruction*> IrGen::genInstsFromExpr(ExpressionNode* node) {
    switch (node->getExprType()) {
    case ExpressionNodeType::NumericLiteral: {
        NumericLiteralExpressionNode* numExpr =
            reinterpret_cast<NumericLiteralExpressionNode*>(node);
        IrInstruction* inst = new IrInstruction;
        inst->type          = IrInstructionType::Const;
        inst->result        = newSSAResult();
        inst->operands      = {this->generateOperand(numExpr)};
        return {inst};
    } break;
    case ExpressionNodeType::Cast: {
        CastExpressionNode*         castExpr = reinterpret_cast<CastExpressionNode*>(node);
        std::vector<IrInstruction*> retInsts = this->genInstsFromExpr(castExpr->getValue());
        if (castExpr->getType()->getBitSize() != convertExpressionToType(this->outModule->objects,
                                                                         this->currentFunc,
                                                                         castExpr->getValue())
                                                     ->getBitSize()) {
            IrInstruction* castInst = new IrInstruction;
            castInst->result        = newSSAResult();
            if (castExpr->getType()->getBitSize() <
                convertExpressionToType(this->outModule->objects, this->currentFunc,
                                        castExpr->getValue())
                    ->getBitSize()) {
                castInst->type = IrInstructionType::Trunc;
            } else {
                castInst->type = convertExpressionToType(this->outModule->objects,
                                                         this->currentFunc, castExpr->getValue())
                                         ->isUnsigned()
                                     ? IrInstructionType::Zext
                                     : IrInstructionType::Sext;
            }
            castInst->operands = {
                createSSAOperand(ssaResults - 2, this->generateType(convertExpressionToType(
                                                     this->outModule->objects, this->currentFunc,
                                                     castExpr->getValue()))),
                createTypeOperand(this->generateType(castExpr->getType()))};
            retInsts.push_back(castInst);
        }
        return retInsts;
    } break;
    case ExpressionNodeType::Binary: {
        BinaryExpressionNode*       binExpr  = reinterpret_cast<BinaryExpressionNode*>(node);
        std::vector<IrInstruction*> lhsInsts = this->genInstsFromExpr(binExpr->getLhs());
        size_t                      lastLhs  = ssaResults - 1;
        std::vector<IrInstruction*> rhsInsts = this->genInstsFromExpr(binExpr->getRhs());
        size_t                      lastRhs  = ssaResults - 1;
        std::vector<IrInstruction*> retInsts;
        for (IrInstruction* inst : lhsInsts) {
            retInsts.push_back(inst);
        }
        for (IrInstruction* inst : rhsInsts) {
            retInsts.push_back(inst);
        }
        if (binExpr->getOperator() == "*") {
            retInsts.push_back(new IrInstruction(
                newSSAResult(), IrInstructionType::Mul,
                {createSSAOperand(
                     lastLhs, this->generateType(convertExpressionToType(
                                  this->outModule->objects, this->currentFunc, binExpr->getLhs()))),
                 createSSAOperand(lastRhs, this->generateType(convertExpressionToType(
                                               this->outModule->objects, this->currentFunc,
                                               binExpr->getRhs())))}));
        } else if (binExpr->getOperator() == "+") {
            retInsts.push_back(new IrInstruction(
                newSSAResult(), IrInstructionType::Add,
                {createSSAOperand(
                     lastLhs, this->generateType(convertExpressionToType(
                                  this->outModule->objects, this->currentFunc, binExpr->getLhs()))),
                 createSSAOperand(lastRhs, this->generateType(convertExpressionToType(
                                               this->outModule->objects, this->currentFunc,
                                               binExpr->getRhs())))}));
        } else {
            std::printf("TODO: Generate binary operator `%s`\n", binExpr->getOperator().c_str());
            std::exit(1);
        }
        return retInsts;
    } break;
    case ExpressionNodeType::LtoRValue: {
        IrInstruction* inst               = new IrInstruction;
        inst->type                        = IrInstructionType::Load;
        inst->result                      = newSSAResult();
        LtoRValueCastExpression* LtoRExpr = reinterpret_cast<LtoRValueCastExpression*>(node);
        inst->operands                    = {this->generateOperand(LtoRExpr->getExpr()),
                                             createTypeOperand(this->generateType(convertExpressionToType(
                              this->outModule->objects, this->currentFunc, LtoRExpr->getExpr())))};
        return {inst};
    } break;
    default: {
        std::printf("TODO: Generate expr %llu\n", node->getExprType());
        std::exit(1);
    } break;
    }
    return {};
}
bool isPrimaryExpressionType(ExpressionNodeType type) {
    if (type == ExpressionNodeType::NumericLiteral) {
        return true;
    }
    return false;
}
static bool isTerminatorInst(IrInstructionType type) {
    return type == IrInstructionType::Return || type == IrInstructionType::Br;
}
std::vector<IrBlock*> IrGen::generateCompoundBlocks(CompoundStatementNode* node) {
    std::vector<IrBlock*> blocks;
    IrBlock*              currentBlock = new IrBlock;
    currentBlock->name                 = ".BB" + std::to_string(blockNumbers++);
    auto insertBlock = [&blocks](IrBlock* block, std::optional<std::string> nextName) {
        if ((block->insts.empty() || !isTerminatorInst(block->insts.back()->type)) &&
            nextName.has_value()) {
            IrInstruction* terminatorInst = new IrInstruction;
            terminatorInst->type          = IrInstructionType::Br;
            terminatorInst->operands      = {createLabelOperand(nextName.value())};
            block->insts.push_back(terminatorInst);
        }
        if (block->insts.empty() || !isTerminatorInst(block->insts.back()->type)) {
            std::printf(
                "ICE: Failed to insert terminator instruction in block `%s` to block `%s`\n",
                block->name.c_str(),
                nextName.has_value() ? nextName.value().c_str() : "std::nullopt");
            std::exit(1);
        }
        blocks.push_back(block);
    };
    for (StatementNode* stmtNode : node->getNodes()) {
        if (!currentBlock ||
            (!currentBlock->insts.empty() && isTerminatorInst(currentBlock->insts.back()->type))) {
            currentBlock       = new IrBlock;
            currentBlock->name = ".BB" + std::to_string(blockNumbers++);
        }
        switch (stmtNode->getStmtType()) {
        case StatementNodeType::Declaration: {
            DeclarationNode* declNode =
                reinterpret_cast<DeclarationStatementNode*>(stmtNode)->getDeclNode();
            switch (declNode->getDeclType()) {
            case DeclarationNodeType::Variable: {
                VariableDeclarationNode* varDecl =
                    reinterpret_cast<VariableDeclarationNode*>(declNode);
                this->currentFunc->entryInsts.push_back(new IrInstruction(
                    newSSAResult(), IrInstructionType::Reserve,
                    {new IrOperand(IrOperandType::Type, this->generateType(varDecl->getType()))}));
                this->currentFunc->nameToSSANumber.insert({varDecl->getName(), ssaResults - 1});
                for (IrInstruction* inst : this->genInstsFromExpr(varDecl->getValue().value())) {
                    currentBlock->insts.push_back(inst);
                }
                if (isPrimaryExpressionType(varDecl->getValue().value()->getExprType())) {
                    currentBlock->insts.push_back(new IrInstruction(
                        newSSAResult(), IrInstructionType::Store,
                        {createSSAOperand(ssaResults - 2, new IrType(IrTypeType::Pointer, "ptr")),
                         createConstI32Operand(
                             std::stol(reinterpret_cast<NumericLiteralExpressionNode*>(
                                           varDecl->getValue().value())
                                           ->getValue()))}));
                } else {
                    currentBlock->insts.push_back(new IrInstruction(
                        newSSAResult(), IrInstructionType::Store,
                        {createSSAOperand(this->currentFunc->nameToSSANumber.at(varDecl->getName()),
                                          new IrType(IrTypeType::Pointer, "ptr")),
                         createSSAOperand(ssaResults - 2,
                                          this->generateType(convertExpressionToType(
                                              this->outModule->objects, this->currentFunc,
                                              varDecl->getValue().value())))}));
                }
            } break;
            default: {
                std::printf("TODO: Generate decl %llu\n", declNode->getDeclType());
                std::exit(1);
            } break;
            }
        } break;
        case StatementNodeType::Return: {
            std::vector<IrInstruction*> insts =
                this->genInstsFromExpr(reinterpret_cast<ReturnStatementNode*>(stmtNode)->getExpr());
            insts.push_back(new IrInstruction(
                std::nullopt, IrInstructionType::Return,
                {createSSAOperand(
                    ssaResults - 1,
                    this->generateType(convertExpressionToType(
                        this->outModule->objects, this->currentFunc,
                        reinterpret_cast<ReturnStatementNode*>(stmtNode)->getExpr())))}));
            currentBlock->insts.insert(currentBlock->insts.end(), insts.begin(), insts.end());
            insertBlock(currentBlock, std::nullopt);
            currentBlock = nullptr;
        } break;
        case StatementNodeType::Compound: {
            std::string nextName = ".BB" + std::to_string(blockNumbers);
            insertBlock(currentBlock, nextName);

            auto compoundBlocks =
                generateCompoundBlocks(reinterpret_cast<CompoundStatementNode*>(stmtNode));

            blocks.insert(blocks.end(), compoundBlocks.begin(), compoundBlocks.end());
            IrBlock* last = compoundBlocks.back();
            if (!last->insts.empty() && isTerminatorInst(last->insts.back()->type)) {
                currentBlock = nullptr;
            } else {
                currentBlock       = new IrBlock;
                currentBlock->name = ".BB" + std::to_string(blockNumbers++);
            }
        } break;
        default: {
            std::printf("TODO: Generate stmt %llu\n", stmtNode->getStmtType());
            std::exit(1);
        } break;
        }
    }
    if (currentBlock && !currentBlock->insts.empty() && blocks.empty()) {
        insertBlock(currentBlock, ".BB" + std::to_string(blockNumbers));
    }
    return blocks;
}
std::vector<IrBlock*> IrGen::generateBlocks(StatementNode* node) {
    switch (node->getStmtType()) {
    case StatementNodeType::Compound: {
        return this->generateCompoundBlocks(reinterpret_cast<CompoundStatementNode*>(node));
    } break;
    default: {
        std::printf("TODO: generateBlocks %llu\n", node->getStmtType());
        std::exit(1);
    } break;
    }
}
IrModule* IrGen::getModule() {
    this->generate();
    return this->outModule;
}
}; // namespace language