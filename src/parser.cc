#include <cstring>
#include <filesystem>
#include <iostream>
#include <parser.h>
#include <string>

namespace language {
Parser::Parser(Lexer* lexer) {
    this->tokensIndex = 0;
    this->tokens      = lexer->lexAllTokens();
}
Parser::~Parser() {
    delete this->retAst;
}
Ast* Parser::getAst() {
    this->retAst = new Ast;
    this->parse();
    return this->retAst;
}
Token* Parser::peek(size_t lookAhead) {
    return this->tokens.size() < this->tokensIndex + lookAhead
               ? this->tokens.at(this->tokensIndex + lookAhead)
               : *this->tokens.end().base();
}
Token* Parser::getCurrentToken() {
    return this->tokens.at(this->tokensIndex);
}
Token* Parser::expect(TokenType type, bool advance) {
    if (this->getCurrentToken()->get_type() != type) {
        std::printf("Expected %llu but got %llu\n", type, this->getCurrentToken()->get_type());
        std::exit(1);
    }
    Token* retToken = this->getCurrentToken();
    if (advance) {
        this->advance();
    }
    return retToken;
}

Token* Parser::expect(std::vector<TokenType> types, bool advance) {
    bool found = false;
    for (TokenType t : types) {
        if (this->getCurrentToken()->get_type() == t) {
            found = true;
            break;
        }
    }
    if (!found) {
        std::printf("Expected:\n");
        for (TokenType t : types) {
            std::printf("-   %llu\n", t);
        }
        std::printf("but got %llu\n", this->getCurrentToken()->get_type());
        std::exit(1);
    }
    Token* retToken = this->getCurrentToken();
    if (advance) {
        this->advance();
    }
    return retToken;
}
static bool isBinaryOp(TokenType type) {
    if (type == TokenType::Plus || type == TokenType::Minus || type == TokenType::Star ||
        type == TokenType::Percent || type == TokenType::EqualEqual) {
        return true;
    }
    return false;
}
static bool isUnaryOp(TokenType type) {
    if (type == TokenType::Minus) {
        return true;
    }
    return false;
}
static size_t getPrecedence(TokenType type) {
    switch (type) {
    case TokenType::EqualEqual:
        return 8;
    case TokenType::Plus:
    case TokenType::Minus:
        return 11;
    case TokenType::Percent:
    case TokenType::Star:
        return 12;
    default:
        return -1;
    }
}
static std::vector<std::pair<TokenType, std::string>> tokenTypeTypesString = {
    {TokenType::U64, "u64"}, {TokenType::U32, "u32"},       {TokenType::Void, "void"},
    {TokenType::I32, "i32"}, {TokenType::String, "String"}, {TokenType::Variadic, "Variadic"}};
static std::string tokenTypeTypeToString(TokenType t) {
    for (std::pair<TokenType, std::string> type : tokenTypeTypesString) {
        if (t == type.first) {
            return type.second;
        }
    }
    std::printf("Invalid typename %llu\n", t);
    std::exit(1);
}
TypeSpec* Parser::parseTypeSpec() {
    size_t pointerCount = 0;
    while (this->getCurrentToken()->get_type() == TokenType::Star) {
        pointerCount++;
        this->advance();
    }
    std::string typeName;
    if (this->getCurrentToken()->get_type() == TokenType::Identifier) {
        typeName = this->expect(TokenType::Identifier, true)->get_value();
    } else {
        typeName = tokenTypeTypeToString(this->getCurrentToken()->get_type());
        this->advance();
    }
    return new TypeSpec(pointerCount, typeName);
}
TypeSpec* Parser::parseTypeSpecWithColon() {
    this->expect(TokenType::Colon, true);
    return this->parseTypeSpec();
}
StatementNode* Parser::parseIfStatement() {
    this->advance();
    this->expect(TokenType::Openparen, true);
    ExpressionNode* condition = this->parseExpression();
    this->expect(TokenType::Closeparen, true);
    StatementNode* trueBody  = this->parseStatement();
    StatementNode* falseBody = nullptr;
    if (this->getCurrentToken()->get_type() == TokenType::Else) {
        this->advance();
        falseBody = this->parseStatement();
    }
    return new IfStatementNode(condition, trueBody,
                               falseBody ? std::make_optional(falseBody) : std::nullopt);
}
StatementNode* Parser::parseReturnStatement() {
    this->advance();
    ExpressionNode* expr = this->parseExpression();
    this->expect(TokenType::Semicolon, true);
    return new ReturnStatementNode(expr);
}
StatementNode* Parser::parseStatement() {
    switch (this->getCurrentToken()->get_type()) {
    case TokenType::Var:
    case TokenType::Class:
    case TokenType::Func: {
        return new DeclarationStatementNode(this->parseDecl().at(0));
    } break;
    case TokenType::Return: {
        return this->parseReturnStatement();
    } break;
    case TokenType::If: {
        return this->parseIfStatement();
    } break;
    case TokenType::Openbrace: {
        return this->parseCompoundStatement();
    } break;
    default: {
        StatementNode* ret = new ExpressionStatementNode(this->parseExpression());
        this->expect(TokenType::Semicolon, true);
        return ret;
    } break;
    }
}
StatementNode* Parser::parseCompoundStatement() {
    this->expect(TokenType::Openbrace, true);
    std::vector<StatementNode*> nodes;
    while (this->getCurrentToken()->get_type() != TokenType::Closebrace) {
        nodes.push_back(this->parseStatement());
    }
    this->expect(TokenType::Closebrace, true);
    return new CompoundStatementNode(nodes);
}
ExpressionNode* Parser::parsePrimaryExpression() {
    switch (this->getCurrentToken()->get_type()) {
    case TokenType::Identifier: {
        return new IdentifierLiteralExpressionNode(
            this->expect(TokenType::Identifier, true)->get_value());
    } break;
    case TokenType::LitString: {
        return new StringLiteralExpressionNode(
            this->expect(TokenType::LitString, true)->get_value());
    } break;
    case TokenType::LitNumber: {
        return new NumericLiteralExpressionNode(
            this->expect(TokenType::LitNumber, true)->get_value());
    } break;
    case TokenType::Openparen: {
        this->advance();
        ExpressionNode* expr = this->parseExpression();
        this->expect(TokenType::Closeparen, true);
        return expr;
    } break;
    case TokenType::Import: {
        this->advance();
        return new IdentifierLiteralExpressionNode("import");
    } break;
    case TokenType::Class: {
        this->advance();
        return new IdentifierLiteralExpressionNode("class");
    } break;
    case TokenType::Func: {
        this->advance();
        return new IdentifierLiteralExpressionNode("func");
    } break;
    case TokenType::Var: {
        this->advance();
        return new IdentifierLiteralExpressionNode("var");
    } break;
    case TokenType::Attrib: {
        this->advance();
        return new IdentifierLiteralExpressionNode("attrib");
    } break;
    case TokenType::If: {
        this->advance();
        return new IdentifierLiteralExpressionNode("if");
    } break;
    case TokenType::Else: {
        this->advance();
        return new IdentifierLiteralExpressionNode("else");
    } break;
    case TokenType::As: {
        this->advance();
        return new IdentifierLiteralExpressionNode("as");
    } break;
    case TokenType::Return: {
        this->advance();
        return new IdentifierLiteralExpressionNode("return");
    } break;
    case TokenType::Void: {
        this->advance();
        return new IdentifierLiteralExpressionNode("void");
    } break;
    case TokenType::U64: {
        this->advance();
        return new IdentifierLiteralExpressionNode("u64");
    } break;
    case TokenType::U32: {
        this->advance();
        return new IdentifierLiteralExpressionNode("u32");
    } break;
    case TokenType::I32: {
        this->advance();
        return new IdentifierLiteralExpressionNode("i32");
    } break;
    default: {
        std::printf("Invalid primary expression `%s`\n",
                    this->getCurrentToken()->get_value().c_str());
        std::exit(1);
    } break;
    }
}
ExpressionNode* Parser::parsePostFixExpression() {
    ExpressionNode* lhs        = this->parsePrimaryExpression();
    bool            shouldExit = false;
    while (!shouldExit) {
        switch (this->getCurrentToken()->get_type()) {
        case TokenType::Dot:
        case TokenType::ColonColon: {
            this->advance();
            ExpressionNode* rhs = this->parsePrimaryExpression();
            lhs                 = new MemberAccessExpressionNode(lhs, rhs);
        } break;
        case TokenType::Openparen: {
            this->advance();
            std::vector<ExpressionNode*> arguments;
            while (this->getCurrentToken()->get_type() != TokenType::Closeparen) {
                arguments.push_back(this->parseExpression());
                if (this->getCurrentToken()->get_type() != TokenType::Closeparen) {
                    this->expect(TokenType::Comma, true);
                }
            }
            this->expect(TokenType::Closeparen, true);
            lhs = new FunctionCallExpressionNode(lhs, arguments);
        } break;
        default: {
            shouldExit = true;
        } break;
        }
    }
    return lhs;
}
ExpressionNode* Parser::parseUnaryExpression() {
    while (isUnaryOp(this->getCurrentToken()->get_type())) {
        std::string unaryOp = this->getCurrentToken()->get_value();
        this->advance();
        return new UnaryExpressionNode(unaryOp, this->parseUnaryExpression());
    }
    return this->parsePostFixExpression();
}
ExpressionNode* Parser::parseInfixExpression(size_t minPrecedence) {
    ExpressionNode* lhs = this->parseUnaryExpression();
    while (isBinaryOp(this->getCurrentToken()->get_type())) {
        std::string currentOp  = this->getCurrentToken()->get_value();
        size_t      precedence = getPrecedence(this->getCurrentToken()->get_type());
        if (precedence < minPrecedence) break;
        this->advance();
        ExpressionNode* rhs = this->parseInfixExpression(precedence + 1);
        lhs                 = new BinaryExpressionNode(lhs, rhs, currentOp);
    }
    return lhs;
}
ExpressionNode* Parser::parseAssignmentExpression() {
    ExpressionNode* lhs = this->parseInfixExpression(0);
    while (this->getCurrentToken()->get_type() == TokenType::Equal) {
        this->advance();
        ExpressionNode* rhs = this->parseExpression();
        lhs                 = new AssignmentExpressionNode(lhs, rhs);
    }
    return lhs;
}
ExpressionNode* Parser::parseCastExpression() {
    ExpressionNode* lhs = this->parseAssignmentExpression();
    while (this->getCurrentToken()->get_type() == TokenType::As) {
        this->advance();
        TypeSpec* typespec = this->parseTypeSpec();
        lhs                = new CastExpressionNode(lhs, typespec);
    }
    return lhs;
}
ExpressionNode* Parser::parseExpression() {
    return this->parseCastExpression();
}
std::vector<std::pair<AttributeType, std::string>> attribToName{
    {AttributeType::Public, "public"},
    {AttributeType::Private, "private"},
    {AttributeType::NoMangle, "no_mangle"},
};
AttributeType getAttribType(std::string name) {
    for (std::pair<AttributeType, std::string> attrib : attribToName) {
        if (attrib.second == name) {
            return attrib.first;
        }
    }
    std::printf("Invalid attribute `%s`\n", name.c_str());
    std::exit(1);
}
std::vector<AttributeNode*> Parser::parseAttributes() {
    if (this->getCurrentToken()->get_type() != TokenType::At) {
        return {};
    }
    this->advance();
    this->expect(TokenType::Attrib, true);
    this->expect(TokenType::Openparen, true);
    std::vector<AttributeNode*> attribNodes;
    while (this->getCurrentToken()->get_type() != TokenType::Closeparen) {
        AttributeType attribType = getAttribType(this->getCurrentToken()->get_value());
        this->advance();
        if (this->getCurrentToken()->get_type() == TokenType::Openparen) {
            std::printf("TODO: Special treatment for `section`\n");
            std::exit(1);
        }
        attribNodes.push_back(new AttributeNode(attribType, true));
    }
    this->expect(TokenType::Closeparen, true);
    return attribNodes;
}
// TODO: Be able to make the user specify the include path
std::vector<std::string> includePaths = {"/usr/include/lunar"};
bool directoryContainsFile(std::string directory, std::string fileName, bool handleDir) {
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        std::string entryString = entry.path().string();
        if (!handleDir) {
            if (!entryString.ends_with(".lng")) {
                continue;
            }
        }
        std::string base = entryString.substr(entryString.find_last_of('/') + 1);
        if (base == fileName) {
            return true;
        }
    }
    return false;
}
std::string findFileByExpression(std::vector<std::string> dirs, ExpressionNode* node,
                                 bool directory = false) {
    if (node->getExprType() != ExpressionNodeType::MemberAccess &&
        node->getExprType() != ExpressionNodeType::IdentifierLiteral) {
        std::printf("Expected an identifier or member access (name::name) for import name but "
                    "got %llu\n",
                    node->getExprType());
        std::exit(1);
    }
    if (node->getExprType() == ExpressionNodeType::IdentifierLiteral) {
        std::string fileName = reinterpret_cast<IdentifierLiteralExpressionNode*>(node)->getValue();
        if (!directory) {
            fileName += ".lng";
        }
        for (std::string incPath : dirs) {
            if (directoryContainsFile(incPath, fileName, directory)) {
                return incPath + "/" + fileName;
            }
        }
        std::printf("No such file or directory to import `%s`\n", fileName.c_str());
        std::exit(1);
    } else if (node->getExprType() == ExpressionNodeType::MemberAccess) {
        ExpressionNode* parentNode =
            reinterpret_cast<MemberAccessExpressionNode*>(node)->getParent();
        ExpressionNode* propertyNode =
            reinterpret_cast<MemberAccessExpressionNode*>(node)->getProperty();
        std::string parentString = findFileByExpression(dirs, parentNode, true);
        std::string childString  = findFileByExpression({parentString}, propertyNode, directory);
        return childString;
    }
    __builtin_unreachable();
}
static std::string readFile(std::string path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        fprintf(stderr, "%s `%s`\n", std::strerror(errno), path.c_str());
        exit(1);
    }
    std::fseek(f, 0, SEEK_END);
    size_t length = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    char* buffer = new char[length]();
    std::fread(buffer, 1, length, f);
    std::fclose(f);
    std::string ret = std::string(buffer, length);
    delete[] buffer;
    return ret;
}
// TODO: Rework this bullshit
std::vector<DeclarationNode*> Parser::parseImportDecl() {
    this->advance();
    ExpressionNode* nameExpr = this->parsePostFixExpression();
    this->expect(TokenType::Semicolon, true);
    std::string                   file   = findFileByExpression(includePaths, nameExpr);
    Lexer*                        lexer  = new Lexer(readFile(file));
    Parser*                       parser = new Parser(lexer);
    Ast*                          ast    = parser->getAst();
    std::vector<DeclarationNode*> nodes;
    for (AstNode* node : ast->getNodes()) {
        if (node->getAstType() != AstNodeType::Declaration) {
            std::printf("Top level node of translation unit %s wasn't a declaration\n",
                        file.c_str());
            std::exit(1);
        }
        nodes.push_back(reinterpret_cast<DeclarationNode*>(node));
    }
    return nodes;
}
DeclarationNode* Parser::parseClassDecl() {
    this->advance();
    std::string    name = this->expect(TokenType::Identifier, true)->get_value();
    StatementNode* body = this->parseCompoundStatement();
    return new ClassDeclarationNode(name, body);
}
DeclarationNode* Parser::parseParamDecl() {
    std::string name = this->expect(TokenType::Identifier, true)->get_value();
    TypeSpec*   type = this->parseTypeSpecWithColon();
    return new ParameterDeclarationNode(name, type);
}
DeclarationNode* Parser::parseVarDecl() {
    this->advance();
    std::vector<AttributeNode*> attrs = this->parseAttributes();
    std::string                 name  = this->expect(TokenType::Identifier, true)->get_value();
    TypeSpec*                   type  = this->parseTypeSpecWithColon();
    ExpressionNode*             value = nullptr;
    if (this->getCurrentToken()->get_type() == TokenType::Equal) {
        this->advance();
        value = this->parseExpression();
    }
    this->expect(TokenType::Semicolon, true);
    return new VariableDeclarationNode(name, attrs, type,
                                       value ? std::make_optional(value) : std::nullopt);
}
DeclarationNode* Parser::parseFuncDecl() {
    this->advance();
    std::vector<AttributeNode*> attrs = this->parseAttributes();
    std::string                 name  = this->expect(TokenType::Identifier, true)->get_value();
    this->expect(TokenType::Openparen, true);
    std::vector<DeclarationNode*> params;
    while (this->getCurrentToken()->get_type() != TokenType::Closeparen) {
        params.push_back(this->parseParamDecl());
        if (this->getCurrentToken()->get_type() != TokenType::Closeparen) {
            this->expect(TokenType::Comma, true);
        }
    }
    this->expect(TokenType::Closeparen, true);
    TypeSpec* returnType;
    if (name == "new" || name == "delete") {
        if (name == "new") {
            returnType = new TypeSpec(0, "IMPLICIT THIS");
        }
        if (name == "delete") {
            returnType = new TypeSpec(0, "IMPLICIT VOID");
        }
    } else {
        returnType = this->parseTypeSpecWithColon();
    }
    StatementNode* body = this->parseStatement();
    return new FunctionDeclarationNode(name, attrs, params, returnType, body);
}
std::vector<DeclarationNode*> Parser::parseDecl() {
    switch (this->getCurrentToken()->get_type()) {
    case TokenType::Import: {
        return this->parseImportDecl();
    } break;
    case TokenType::Func: {
        return {this->parseFuncDecl()};
    } break;
    case TokenType::Class: {
        return {this->parseClassDecl()};
    } break;
    case TokenType::Var: {
        return {this->parseVarDecl()};
    } break;
    default: {
        std::printf("Invalid declaration token `%s`\n",
                    this->getCurrentToken()->get_value().c_str());
        std::exit(1);
    } break;
    }
}
void Parser::advance() {
    this->tokensIndex++;
}
void Parser::parse() {
    while (this->getCurrentToken()->get_type() != TokenType::Eof) {
        for (DeclarationNode* declNode : this->parseDecl()) {
            this->retAst->addNode(declNode);
        }
    }
}
}; // namespace language