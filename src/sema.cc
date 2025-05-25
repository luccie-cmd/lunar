#include <sema.h>

namespace language {
Sema::Sema(Ast* ast) {
    this->oldAst             = ast;
    SymbolTable* globalTable = new SymbolTable;
    globalTable->name        = "@GlobalScope";
    globalTable->parent      = nullptr;
    globalTable->symbols.clear();
    globalTable->allowedTypes = {"String", "Variadic", "i32", "i64", "u32", "u64"};
    this->tables.push(globalTable);
}
SymbolTable* Sema::getCurrentTable() {
    return this->tables.top();
}
static ExpressionNode* getDefaultForType(TypeSpec* type) {
    if (type->isInteger()) {
        return new NumericLiteralExpressionNode("0");
    }
    std::printf("TODO: getDefaultForType for `%s`\n", type->getName().c_str());
    std::exit(1);
}
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
    type1->print(0);
    type2->print(0);
    std::printf("TODO: Implicit cast\n");
    std::exit(1);
}
static TypeSpec* convertExpressionToType(SymbolTable* table, ExpressionNode* node) {
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
        TypeSpec*             lhs     = convertExpressionToType(table, binNode->getLhs());
        TypeSpec*             rhs     = convertExpressionToType(table, binNode->getRhs());
        return getBiggestType(lhs, rhs);
    } break;
    case ExpressionNodeType::IdentifierLiteral: {
        return table->lookup(reinterpret_cast<IdentifierLiteralExpressionNode*>(node)->getValue())
            ->type;
    } break;
    case ExpressionNodeType::LtoRValue: {
        return convertExpressionToType(table,
                                       reinterpret_cast<LtoRValueCastExpression*>(node)->getExpr());
    } break;
    case ExpressionNodeType::Cast: {
        return reinterpret_cast<CastExpressionNode*>(node)->getType();
    } break;
    default: {
        std::printf("TODO: Unhandled expression node type for conversion %llu\n",
                    node->getExprType());
        std::exit(1);
    } break;
    }
}
static bool exprCanBeFolded(ExpressionNode* node) {
    switch (node->getExprType()) {
    case ExpressionNodeType::Cast: {
        return exprCanBeFolded(reinterpret_cast<CastExpressionNode*>(node)->getValue());
    } break;
    case ExpressionNodeType::LtoRValue: {
        return exprCanBeFolded(reinterpret_cast<LtoRValueCastExpression*>(node)->getExpr());
    } break;
    case ExpressionNodeType::Binary: {
        return exprCanBeFolded(reinterpret_cast<BinaryExpressionNode*>(node)->getLhs()) &&
               exprCanBeFolded(reinterpret_cast<BinaryExpressionNode*>(node)->getRhs());
    } break;
    case ExpressionNodeType::IdentifierLiteral: {
        return false;
    } break;
    case ExpressionNodeType::NumericLiteral: {
        return true;
    } break;
    default: {
        std::printf("Check if %llu expr type can be folded\n", node->getExprType());
        std::exit(1);
    } break;
    }
}
DeclarationNode* Sema::checkFunctionDecl(FunctionDeclarationNode* node) {
    if (this->getCurrentTable()->lookup(node->getName())) {
        std::printf("Attempted to redeclare function `%s`\n", node->getName().c_str());
        std::exit(1);
    }
    Symbol* sym = new Symbol;
    sym->type   = this->checkTypeSpec(node->getReturnType());
    sym->name   = node->getName();
    sym->kind   = DeclarationNodeType::Function;
    sym->attrs  = node->getAttribs();
    this->getCurrentTable()->insert(sym);
    SymbolTable* funcTable = new SymbolTable;
    funcTable->name        = node->getName();
    funcTable->parent      = this->getCurrentTable();
    funcTable->symbols.clear();
    for (DeclarationNode* param : node->getParams()) {
        param            = this->checkDeclarationNode(param);
        Symbol* paramSym = new Symbol;
        paramSym->name   = reinterpret_cast<ParameterDeclarationNode*>(param)->getName();
        paramSym->type   = reinterpret_cast<ParameterDeclarationNode*>(param)->getType();
        paramSym->attrs  = {};
        paramSym->kind   = DeclarationNodeType::Parameter;
        funcTable->insert(paramSym);
    }
    this->tables.push(funcTable);
    StatementNode* newBody = this->checkStatement(node->getBody());
    if (newBody->getStmtType() != StatementNodeType::Compound) {
        newBody = new CompoundStatementNode({newBody});
    }
    StatementNode* topBody = newBody;
    while (newBody->getStmtType() == StatementNodeType::Compound) {
        newBody = reinterpret_cast<CompoundStatementNode*>(newBody)->getNodes().back();
    }
    if (newBody->getStmtType() != StatementNodeType::Return) {
        std::vector<StatementNode*> nodes = {topBody};
        nodes.push_back(new ReturnStatementNode(getDefaultForType(sym->type)));
        topBody = new CompoundStatementNode(nodes);
    }
    FunctionDeclarationNode* newDeclNode = new FunctionDeclarationNode(
        node->getName(), node->getAttribs(), node->getParams(), node->getReturnType(), topBody);
    this->tables.pop();
    return newDeclNode;
}
DeclarationNode* Sema::checkParamDecl(ParameterDeclarationNode* node) {
    if (this->getCurrentTable()->lookup(node->getName())) {
        std::printf("Shadow of global declaration `%s`\n", node->getName().c_str());
        std::exit(1);
    }
    (void)this->checkTypeSpec(node->getType());
    return node;
}
DeclarationNode* Sema::checkVarDecl(VariableDeclarationNode* node) {
    if (this->getCurrentTable()->lookup(node->getName())) {
        std::printf("Attempted to redeclare variable `%s`\n", node->getName().c_str());
        std::exit(1);
    }
    Symbol* sym = new Symbol;
    sym->type   = this->checkTypeSpec(node->getType());
    sym->name   = node->getName();
    sym->kind   = DeclarationNodeType::Variable;
    sym->attrs  = node->getAttribs();
    this->getCurrentTable()->insert(sym);
    ExpressionNode* newVal = nullptr;
    if (node->getValue().has_value()) {
        newVal = this->checkExpression(node->getValue().value());
        if (newVal->getValCatagory() == ValueCatagory::Lvalue) {
            newVal = new LtoRValueCastExpression(newVal);
        }
        if (*convertExpressionToType(this->getCurrentTable(), newVal) != *sym->type) {
            newVal = new CastExpressionNode(newVal, sym->type);
        }
    } else {
        newVal = getDefaultForType(node->getType());
    }
    if (!exprCanBeFolded(newVal) && this->getCurrentTable()->name == "@GlobalScope") {
        std::printf("Initializer element of global var `%s` is not constant\n", sym->name.c_str());
        std::exit(1);
    }
    return new VariableDeclarationNode(node->getName(), node->getAttribs(), sym->type,
                                       std::make_optional(newVal));
}
DeclarationNode* Sema::checkDeclarationNode(DeclarationNode* node) {
    switch (node->getDeclType()) {
    case DeclarationNodeType::Function: {
        return this->checkFunctionDecl(reinterpret_cast<FunctionDeclarationNode*>(node));
    } break;
    case DeclarationNodeType::Parameter: {
        return this->checkParamDecl(reinterpret_cast<ParameterDeclarationNode*>(node));
    } break;
    case DeclarationNodeType::Variable: {
        return this->checkVarDecl(reinterpret_cast<VariableDeclarationNode*>(node));
    } break;
    default: {
        std::printf("Unhandled Sema declaration check type %llu\n", node->getDeclType());
        std::exit(1);
    } break;
    }
}
StatementNode* Sema::checkCompoundStatement(CompoundStatementNode* node) {
    static size_t blocks;
    SymbolTable*  tempTable = new SymbolTable;
    tempTable->name         = "tempBlockScope" + std::to_string(blocks++);
    tempTable->parent       = this->getCurrentTable();
    tempTable->symbols.clear();
    this->tables.push(tempTable);
    std::vector<StatementNode*> newNodes;
    for (StatementNode* child : node->getNodes()) {
        newNodes.push_back(this->checkStatement(child));
    }
    this->tables.pop();
    return new CompoundStatementNode(newNodes);
}
StatementNode* Sema::checkReturnStatement(ReturnStatementNode* node) {
    SymbolTable* checkTable = this->getCurrentTable();
    while (checkTable && checkTable->name.starts_with("tempBlockScope")) {
        checkTable = checkTable->parent;
    }
    if (!checkTable) {
        std::printf("Invalid use of return\n");
        std::exit(1);
    }
    ExpressionNode* newRetExpr = this->checkExpression(node->getExpr());
    if (newRetExpr->getValCatagory() == ValueCatagory::Lvalue) {
        newRetExpr = new LtoRValueCastExpression(newRetExpr);
    }
    TypeSpec* funcType    = this->getCurrentTable()->lookup(checkTable->name)->type;
    TypeSpec* retExprType = convertExpressionToType(this->getCurrentTable(), newRetExpr);
    if (*funcType != *retExprType) {
        newRetExpr = new CastExpressionNode(newRetExpr, funcType);
    }
    return new ReturnStatementNode(newRetExpr);
}
StatementNode* Sema::checkStatement(StatementNode* node) {
    switch (node->getStmtType()) {
    case StatementNodeType::Compound: {
        return this->checkCompoundStatement(reinterpret_cast<CompoundStatementNode*>(node));
    } break;
    case StatementNodeType::Declaration: {
        return new DeclarationStatementNode(this->checkDeclarationNode(
            reinterpret_cast<DeclarationStatementNode*>(node)->getDeclNode()));
    } break;
    case StatementNodeType::Return: {
        return this->checkReturnStatement(reinterpret_cast<ReturnStatementNode*>(node));
    } break;
    default: {
        std::printf("Unhandled Sema stmt type %llu\n", node->getStmtType());
        std::exit(1);
    } break;
    }
}
TypeSpec* Sema::checkTypeSpec(TypeSpec* type) {
    if (!this->getCurrentTable()->isTypeAllowed(type->getName())) {
        std::printf("Attempted to use invalid type `%s`\n", type->getName().c_str());
        std::exit(1);
    }
    return type;
}
static bool canHaveOperatorApplied(TypeSpec* lhs, TypeSpec* rhs, std::string _operator) {
    // Arithmetic operators
    // if (_operator == "+" || _operator == "-" || _operator == "*" || _operator == "/" || _operator
    // == "%") {
    //     return lhs->isInteger() && rhs->isInteger();
    // }

    if (_operator == "*" || _operator == "+") {
        return lhs->isInteger() && rhs->isInteger();
    }

    // // Comparison operators
    // if (_operator == "==" || _operator == "!=" ||
    //     _operator == "<" || _operator == "<=" ||
    //     _operator == ">" || _operator == ">=") {
    //     return lhs->isInteger() && rhs->isInteger();
    // }

    // // Bitwise operators
    // if (_operator == "&" || _operator == "|" || _operator == "^" ||
    //     _operator == "<<" || _operator == ">>") {
    //     return lhs->isInteger() && rhs->isInteger();
    // }
    std::printf("Invalid or unhandled operator `%s`\n", _operator.c_str());
    std::exit(1);
}
ExpressionNode* Sema::checkBinaryExpression(BinaryExpressionNode* node) {
    ExpressionNode* lhs     = this->checkExpression(node->getLhs());
    ExpressionNode* rhs     = this->checkExpression(node->getRhs());
    TypeSpec*       lhsType = convertExpressionToType(this->getCurrentTable(), lhs);
    TypeSpec*       rhsType = convertExpressionToType(this->getCurrentTable(), rhs);
    if (!canHaveOperatorApplied(lhsType, rhsType, node->getOperator())) {
        std::printf("Invalid operator `%s` for types `%s` and `%s`\n", node->getOperator().c_str(),
                    lhsType->getName().c_str(), rhsType->getName().c_str());
        std::exit(1);
    }
    if (lhs->getValCatagory() == ValueCatagory::Lvalue) {
        lhs = new LtoRValueCastExpression(lhs);
    }
    if (rhs->getValCatagory() == ValueCatagory::Lvalue) {
        rhs = new LtoRValueCastExpression(rhs);
    }
    TypeSpec* commonType       = getBiggestType(lhsType, rhsType);
    auto      needsLiteralCast = [&](ExpressionNode* expr, TypeSpec* exprType) {
        return expr->getExprType() == ExpressionNodeType::NumericLiteral &&
               commonType->getName() != "i32" && exprType->getName() != commonType->getName();
    };
    if (needsLiteralCast(lhs, lhsType)) {
        lhs = new CastExpressionNode(lhs, commonType);
    } else if (lhsType->getName() != commonType->getName() &&
               lhs->getExprType() != ExpressionNodeType::NumericLiteral) {
        lhs = new CastExpressionNode(lhs, commonType);
    }
    if (needsLiteralCast(rhs, rhsType)) {
        rhs = new CastExpressionNode(rhs, commonType);
    } else if (rhsType->getName() != commonType->getName() &&
               rhs->getExprType() != ExpressionNodeType::NumericLiteral) {
        rhs = new CastExpressionNode(rhs, commonType);
    }
    return new BinaryExpressionNode(lhs, rhs, node->getOperator());
}

ExpressionNode* Sema::checkExpression(ExpressionNode* node) {
    switch (node->getExprType()) {
    case ExpressionNodeType::Binary: {
        return this->checkBinaryExpression(reinterpret_cast<BinaryExpressionNode*>(node));
    } break;
    case ExpressionNodeType::IdentifierLiteral: {
        if (this->getCurrentTable()->lookup(
                reinterpret_cast<IdentifierLiteralExpressionNode*>(node)->getValue()) == nullptr) {
            std::printf(
                "Use of undeclared variable or function `%s`\n",
                reinterpret_cast<IdentifierLiteralExpressionNode*>(node)->getValue().c_str());
            std::exit(1);
        }
        return node;
    } break;
    case ExpressionNodeType::Unary:
    case ExpressionNodeType::NumericLiteral: {
        return node;
    } break;
    default: {
        std::printf("Unhandled Sema expr type %llu\n", node->getExprType());
        std::exit(1);
    } break;
    }
}
AstNode* Sema::checkTopAstNode(AstNode* node) {
    switch (node->getAstType()) {
    case AstNodeType::Declaration: {
        return this->checkDeclarationNode(reinterpret_cast<DeclarationNode*>(node));
    } break;
    default: {
        std::printf("Unhandled Sema check type %llu\n", node->getAstType());
        std::exit(1);
    } break;
    }
}
void Sema::doChecks() {
    this->newAst = new Ast;
    for (AstNode* node : this->oldAst->getNodes()) {
        this->newAst->addNode(this->checkTopAstNode(node));
    }
}
Ast* Sema::getNewAst() {
    this->doChecks();
    return this->newAst;
}
}; // namespace language