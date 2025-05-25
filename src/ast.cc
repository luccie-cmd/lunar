#include <ast.h>
#define TAB_WIDTH 4

namespace language {
Ast::Ast() {}
Ast::~Ast() {
    for (AstNode* node : this->nodes) {
        delete node;
    }
}
std::vector<AstNode*> Ast::getNodes() {
    return this->nodes;
}
void Ast::addNode(AstNode* node) {
    if (node) {
        this->nodes.push_back(node);
    }
}
void Ast::print() {
    std::printf("AST:\n");
    for (AstNode* node : this->nodes) {
        node->print(0);
    }
}
static void printIndent(size_t indent) {
    for (size_t i = 0; i < indent; ++i) {
        if (i % 4 == 0) {
            std::putchar('|');
        } else {
            std::putchar(' ');
        }
    }
}
AstNode::AstNode(AstNodeType nodeType) {
    this->__astNodeType = nodeType;
}
AstNode::~AstNode() {}
AstNodeType AstNode::getAstType() {
    return this->__astNodeType;
}
AttributeNode::AttributeNode(AttributeType type, AttributeData data)
    : AstNode(AstNodeType::Attribute) {
    this->type = type;
    this->data = data;
}
AttributeNode::~AttributeNode() {}
void AttributeNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Attribute:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Type: %llu\n", this->type);
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Data: %zu\n", this->data.index());
}
StatementNode::StatementNode(StatementNodeType __stmtType) : AstNode(AstNodeType::Statement) {
    this->__stmtNodeType = __stmtType;
}
StatementNode::~StatementNode() {}
StatementNodeType StatementNode::getStmtType() {
    return this->__stmtNodeType;
}
ReturnStatementNode::ReturnStatementNode(ExpressionNode* expr)
    : StatementNode(StatementNodeType::Return) {
    this->retExpr = expr;
}
ReturnStatementNode::~ReturnStatementNode() {
    delete this->retExpr;
}
void ReturnStatementNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Statement:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Return:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Expression:\n");
    this->retExpr->print(indent + (TAB_WIDTH * 3));
}
ExpressionNode* ReturnStatementNode::getExpr() {
    return this->retExpr;
}
IfStatementNode::IfStatementNode(ExpressionNode* condition, StatementNode* trueBody,
                                 std::optional<StatementNode*> falseBody)
    : StatementNode(StatementNodeType::If) {
    this->condition = condition;
    this->trueBody  = trueBody;
    this->falseBody = falseBody;
}
IfStatementNode::~IfStatementNode() {
    delete this->condition;
    delete this->trueBody;
    if (this->falseBody.has_value()) {
        delete this->falseBody.value();
    }
}
void IfStatementNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Statement:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- If:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Condition:\n");
    this->condition->print(indent + (TAB_WIDTH * 3));
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- True body:\n");
    this->trueBody->print(indent + (TAB_WIDTH * 3));
    if (this->falseBody.has_value()) {
        printIndent(indent + (TAB_WIDTH * 2));
        std::printf("|- False body:\n");
        this->falseBody.value()->print(indent + (TAB_WIDTH * 3));
    }
}
ExpressionStatementNode::ExpressionStatementNode(ExpressionNode* expr)
    : StatementNode(StatementNodeType::Expression) {
    this->expr = expr;
}
ExpressionStatementNode::~ExpressionStatementNode() {
    delete this->expr;
}
void ExpressionStatementNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Statement:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Expression:\n");
    this->expr->print(indent + TAB_WIDTH * 2);
}
CompoundStatementNode::CompoundStatementNode(std::vector<StatementNode*> nodes)
    : StatementNode(StatementNodeType::Compound) {
    this->nodes = nodes;
}
CompoundStatementNode::~CompoundStatementNode() {
    for (StatementNode* node : this->nodes) {
        delete node;
    }
}
void CompoundStatementNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Statement:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Compound:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Body:\n");
    for (StatementNode* stmt : this->nodes) {
        stmt->print(indent + (TAB_WIDTH * 3));
    }
}
std::vector<StatementNode*> CompoundStatementNode::getNodes() {
    return this->nodes;
}
DeclarationStatementNode::DeclarationStatementNode(DeclarationNode* declNode)
    : StatementNode(StatementNodeType::Declaration) {
    this->declNode = declNode;
}
DeclarationStatementNode::~DeclarationStatementNode() {
    delete this->declNode;
}
void DeclarationStatementNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Statement:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Declaration:\n");
    this->declNode->print(indent + TAB_WIDTH * 2);
}
DeclarationNode* DeclarationStatementNode::getDeclNode() {
    return this->declNode;
}
ExpressionNode::ExpressionNode(ExpressionNodeType exprType) : AstNode(AstNodeType::Expression) {
    this->__exprNodeType = exprType;
    switch (this->__exprNodeType) {
    case ExpressionNodeType::MemberAccess:
    case ExpressionNodeType::StringLiteral:
    case ExpressionNodeType::IdentifierLiteral:
        this->__valCatagory = ValueCatagory::Lvalue;
        break;
    case ExpressionNodeType::Assignment:
    case ExpressionNodeType::FunctionCall:
    case ExpressionNodeType::Binary:
    case ExpressionNodeType::Unary:
    case ExpressionNodeType::Cast:
    case ExpressionNodeType::NumericLiteral:
    case ExpressionNodeType::LtoRValue:
        this->__valCatagory = ValueCatagory::Rvalue;
        break;
    }
}
ExpressionNode::~ExpressionNode() {}
ExpressionNodeType ExpressionNode::getExprType() {
    return this->__exprNodeType;
}
ValueCatagory ExpressionNode::getValCatagory() {
    return this->__valCatagory;
}
UnaryExpressionNode::UnaryExpressionNode(std::string unaryOp, ExpressionNode* expr)
    : ExpressionNode(ExpressionNodeType::Unary) {
    this->unaryOp = unaryOp;
    this->expr    = expr;
}
UnaryExpressionNode::~UnaryExpressionNode() {
    delete this->expr;
}
void UnaryExpressionNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Expression:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Value catagory: `%s`\n",
                this->__valCatagory == ValueCatagory::Lvalue ? "L value" : "R value");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Unary:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Operator: `%s`\n", this->unaryOp.c_str());
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Expression:\n");
    this->expr->print(indent + (TAB_WIDTH * 3));
}
BinaryExpressionNode::BinaryExpressionNode(ExpressionNode* lhs, ExpressionNode* rhs,
                                           std::string _operator)
    : ExpressionNode(ExpressionNodeType::Binary) {
    this->lhs       = lhs;
    this->rhs       = rhs;
    this->_operator = _operator;
}
BinaryExpressionNode::~BinaryExpressionNode() {
    delete this->lhs;
    delete this->rhs;
}
void BinaryExpressionNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Expression:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Value catagory: `%s`\n",
                this->__valCatagory == ValueCatagory::Lvalue ? "L value" : "R value");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Binary:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Operator: `%s`\n", this->_operator.c_str());
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Lhs:\n");
    this->lhs->print(indent + (TAB_WIDTH * 3));
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Rhs:\n");
    this->rhs->print(indent + (TAB_WIDTH * 3));
}
ExpressionNode* BinaryExpressionNode::getLhs() {
    return this->lhs;
}
ExpressionNode* BinaryExpressionNode::getRhs() {
    return this->rhs;
}
std::string BinaryExpressionNode::getOperator() {
    return this->_operator;
}
FunctionCallExpressionNode::FunctionCallExpressionNode(ExpressionNode*              callee,
                                                       std::vector<ExpressionNode*> arguments)
    : ExpressionNode(ExpressionNodeType::FunctionCall) {
    this->callee    = callee;
    this->arguments = arguments;
}
FunctionCallExpressionNode::~FunctionCallExpressionNode() {
    delete this->callee;
    for (ExpressionNode* arg : this->arguments) {
        delete arg;
    }
}
void FunctionCallExpressionNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Expression:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Value catagory: `%s`\n",
                this->__valCatagory == ValueCatagory::Lvalue ? "L value" : "R value");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Function call:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Callee:\n");
    this->callee->print(indent + (TAB_WIDTH * 3));
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Arguments:\n");
    for (ExpressionNode* arg : this->arguments) {
        arg->print(indent + (TAB_WIDTH * 3));
    }
}
AssignmentExpressionNode::AssignmentExpressionNode(ExpressionNode* assignee, ExpressionNode* value)
    : ExpressionNode(ExpressionNodeType::Assignment) {
    this->assignee = assignee;
    this->value    = value;
}
AssignmentExpressionNode::~AssignmentExpressionNode() {
    delete this->assignee;
    delete this->value;
}
void AssignmentExpressionNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Expression:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Value catagory: `%s`\n",
                this->__valCatagory == ValueCatagory::Lvalue ? "L value" : "R value");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Assignment:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Assignee:\n");
    this->assignee->print(indent + (TAB_WIDTH * 3));
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Value:\n");
    this->value->print(indent + (TAB_WIDTH * 3));
}
CastExpressionNode::CastExpressionNode(ExpressionNode* value, TypeSpec* type)
    : ExpressionNode(ExpressionNodeType::Cast) {
    this->value = value;
    this->type  = type;
    if (this->value->getExprType() == ExpressionNodeType::NumericLiteral &&
        this->type->getName() == "i32") {
        std::printf("ICE: Invalid cast please fix:\n");
        value->print(0);
        type->print(0);
        std::exit(1);
    }
}
CastExpressionNode::~CastExpressionNode() {
    delete this->value;
    delete this->type;
}
void CastExpressionNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Expression:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Value catagory: `%s`\n",
                this->__valCatagory == ValueCatagory::Lvalue ? "L value" : "R value");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Cast:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Value:\n");
    this->value->print(indent + (TAB_WIDTH * 3));
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Type:\n");
    this->type->print(indent + (TAB_WIDTH * 3));
}
ExpressionNode* CastExpressionNode::getValue() {
    return this->value;
}
TypeSpec* CastExpressionNode::getType() {
    return this->type;
}
MemberAccessExpressionNode::MemberAccessExpressionNode(ExpressionNode* parent,
                                                       ExpressionNode* property)
    : ExpressionNode(ExpressionNodeType::MemberAccess) {
    this->parent   = parent;
    this->property = property;
}
MemberAccessExpressionNode::~MemberAccessExpressionNode() {
    delete this->parent;
    delete this->property;
}
void MemberAccessExpressionNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Expression:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Value catagory: `%s`\n",
                this->__valCatagory == ValueCatagory::Lvalue ? "L value" : "R value");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Member access:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Parent:\n");
    this->parent->print(indent + (TAB_WIDTH * 3));
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Property:\n");
    this->property->print(indent + (TAB_WIDTH * 3));
}
ExpressionNode* MemberAccessExpressionNode::getParent() {
    return this->parent;
}
ExpressionNode* MemberAccessExpressionNode::getProperty() {
    return this->property;
}
IdentifierLiteralExpressionNode::IdentifierLiteralExpressionNode(std::string value)
    : ExpressionNode(ExpressionNodeType::IdentifierLiteral) {
    this->value = value;
}
IdentifierLiteralExpressionNode::~IdentifierLiteralExpressionNode() {}
void IdentifierLiteralExpressionNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Expression:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Value catagory: `%s`\n",
                this->__valCatagory == ValueCatagory::Lvalue ? "L value" : "R value");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Identifier literal:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Value: `%s`\n", this->value.c_str());
}
std::string IdentifierLiteralExpressionNode::getValue() {
    return this->value;
}
NumericLiteralExpressionNode::NumericLiteralExpressionNode(std::string value)
    : ExpressionNode(ExpressionNodeType::NumericLiteral) {
    this->value = value;
}
NumericLiteralExpressionNode::~NumericLiteralExpressionNode() {}
void NumericLiteralExpressionNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Expression:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Value catagory: `%s`\n",
                this->__valCatagory == ValueCatagory::Lvalue ? "L value" : "R value");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Numeric literal:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Value: `%s`\n", this->value.c_str());
}
std::string NumericLiteralExpressionNode::getValue() {
    return this->value;
}
StringLiteralExpressionNode::StringLiteralExpressionNode(std::string value)
    : ExpressionNode(ExpressionNodeType::StringLiteral) {
    this->value = value;
}
StringLiteralExpressionNode::~StringLiteralExpressionNode() {}
void StringLiteralExpressionNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Expression:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Value catagory: `%s`\n",
                this->__valCatagory == ValueCatagory::Lvalue ? "L value" : "R value");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- String literal:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Value: `%s`\n", this->value.c_str());
}
LtoRValueCastExpression::LtoRValueCastExpression(ExpressionNode* node)
    : ExpressionNode(ExpressionNodeType::LtoRValue) {
    this->Lvalue = node;
}
LtoRValueCastExpression::~LtoRValueCastExpression() {
    delete this->Lvalue;
}
void LtoRValueCastExpression::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Expression:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Value catagory: `%s`\n",
                this->__valCatagory == ValueCatagory::Lvalue ? "L value" : "R value");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- L to R value cast:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- L value:\n");
    this->Lvalue->print(indent + (TAB_WIDTH * 3));
}
ExpressionNode* LtoRValueCastExpression::getExpr() {
    return this->Lvalue;
}
DeclarationNode::DeclarationNode(DeclarationNodeType declType) : AstNode(AstNodeType::Declaration) {
    this->__declNodeType = declType;
}
DeclarationNode::~DeclarationNode() {}
DeclarationNodeType DeclarationNode::getDeclType() {
    return this->__declNodeType;
}
ClassDeclarationNode::ClassDeclarationNode(std::string name, StatementNode* body)
    : DeclarationNode(DeclarationNodeType::Class) {
    this->body = body;
    this->name = name;
}
ClassDeclarationNode::~ClassDeclarationNode() {
    delete this->body;
}
void ClassDeclarationNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Declaration:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Class:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Name: `%s`\n", this->name.c_str());
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Members:\n");
    this->body->print(indent + (TAB_WIDTH * 3));
}
ParameterDeclarationNode::ParameterDeclarationNode(std::string name, TypeSpec* type)
    : DeclarationNode(DeclarationNodeType::Parameter) {
    this->name = name;
    this->type = type;
}
ParameterDeclarationNode::~ParameterDeclarationNode() {
    delete this->type;
}
void ParameterDeclarationNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Declaration:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Parameter:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Name: `%s`\n", this->name.c_str());
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Type:\n");
    this->type->print(indent + (TAB_WIDTH * 3));
}
std::string ParameterDeclarationNode::getName() {
    return this->name;
}
TypeSpec* ParameterDeclarationNode::getType() {
    return this->type;
}
FunctionDeclarationNode::FunctionDeclarationNode(std::string                   name,
                                                 std::vector<AttributeNode*>   attrs,
                                                 std::vector<DeclarationNode*> params,
                                                 TypeSpec* returnType, StatementNode* body)
    : DeclarationNode(DeclarationNodeType::Function) {
    this->name       = name;
    this->attrs      = attrs;
    this->params     = params;
    this->returnType = returnType;
    this->body       = body;
}
FunctionDeclarationNode::~FunctionDeclarationNode() {
    for (AttributeNode* node : this->attrs) {
        delete node;
    }
    for (DeclarationNode* node : this->params) {
        delete node;
    }
    delete this->returnType;
    delete this->body;
}
void FunctionDeclarationNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Declaration:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Function:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Name: `%s`\n", this->name.c_str());
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Return type:\n");
    this->returnType->print(indent + (TAB_WIDTH * 3));
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Attributes:\n");
    for (AttributeNode* attr : this->attrs) {
        attr->print(indent + (TAB_WIDTH * 3));
    }
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Parameters:\n");
    for (DeclarationNode* param : this->params) {
        param->print(indent + (TAB_WIDTH * 3));
    }
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Body:\n");
    this->body->print(indent + (TAB_WIDTH * 3));
}
StatementNode* FunctionDeclarationNode::getBody() {
    return this->body;
}
std::string FunctionDeclarationNode::getName() {
    return this->name;
}
TypeSpec* FunctionDeclarationNode::getReturnType() {
    return this->returnType;
}
std::vector<AttributeNode*> FunctionDeclarationNode::getAttribs() {
    return this->attrs;
}
std::vector<DeclarationNode*> FunctionDeclarationNode::getParams() {
    return this->params;
}
VariableDeclarationNode::VariableDeclarationNode(std::string                    name,
                                                 std::vector<AttributeNode*>    attribs,
                                                 TypeSpec*                      type,
                                                 std::optional<ExpressionNode*> value)
    : DeclarationNode(DeclarationNodeType::Variable) {
    this->name    = name;
    this->value   = value;
    this->attribs = attribs;
    this->type    = type;
}
VariableDeclarationNode::~VariableDeclarationNode() {
    for (AttributeNode* node : this->attribs) {
        delete node;
    }
    delete this->type;
    if (this->value.has_value()) {
        delete this->value.value();
    }
}
void VariableDeclarationNode::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Declaration:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Variable:\n");
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Name: `%s`\n", this->name.c_str());
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Type:\n");
    this->type->print(indent + (TAB_WIDTH * 3));
    printIndent(indent + (TAB_WIDTH * 2));
    std::printf("|- Attributes:\n");
    for (AttributeNode* attr : this->attribs) {
        attr->print(indent + (TAB_WIDTH * 3));
    }
    if (this->value.has_value()) {
        printIndent(indent + (TAB_WIDTH * 2));
        std::printf("|- Value:\n");
        this->value.value()->print(indent + (TAB_WIDTH * 3));
    }
}
std::string VariableDeclarationNode::getName() {
    return this->name;
}
std::vector<AttributeNode*> VariableDeclarationNode::getAttribs() {
    return this->attribs;
}
TypeSpec* VariableDeclarationNode::getType() {
    return this->type;
}
std::optional<ExpressionNode*> VariableDeclarationNode::getValue() {
    return this->value;
}
TypeSpec::TypeSpec(size_t pointerLevel, std::string name) : AstNode(AstNodeType::TypeSpec) {
    this->pointerLevel = pointerLevel;
    this->name         = name;
}
TypeSpec::~TypeSpec() {}
void TypeSpec::print(size_t indent) {
    printIndent(indent);
    std::printf("|- Typespec:\n");
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Pointer count: %zu\n", this->pointerLevel);
    printIndent(indent + TAB_WIDTH);
    std::printf("|- Type name: `%s`\n", this->name.c_str());
}
std::string TypeSpec::getName() {
    return this->name;
}
bool TypeSpec::isInteger() {
    if (this->pointerLevel > 0) {
        return true;
    }
    return this->name == "u64" || this->name == "u32" || this->name == "i64" || this->name == "i32";
}
bool TypeSpec::isUnsigned() {
    if (this->pointerLevel > 0) {
        return true;
    }
    return this->name == "u64" || this->name == "u32";
}
size_t TypeSpec::getBitSize() {
    if (this->pointerLevel > 0) {
        return 64;
    }
    if (this->name == "u32" || this->name == "i32") {
        return 32;
    }
    if (this->name == "u64" || this->name == "i64") {
        return 64;
    }
    if (this->name == "String") {
        return 64;
    }
    std::printf("TODO: Type %s\n", this->name.c_str());
    std::exit(1);
}
size_t TypeSpec::getPointerCount() {
    return this->pointerLevel;
}
}; // namespace language