#include "parser.hh"

namespace lunar{
AstNode* Parser::nextNode(){
    current = this->currentToken();
    if(current.get_type() == TokenType::TEOF){
        return nullptr;
    }
    switch(current.get_type()){
        case TokenType::IMPORT:
        case TokenType::OPENBRACE: {
            AstMembers node = this->parseStatement();
            return new AstNode(node);
        } break;
        case TokenType::FUNC: {
            AstMembers node = this->parseFuncDecl();
            return new AstNode(node);
        } break;
        case TokenType::IDENTIFIER: {
            if(this->peekToken(1).get_type() == TokenType::OPENPAREN){
                AstMembers node = this->parseCallExpr();
                return new AstNode(node);
            }
        } break;
    }
    fmt::print("ERROR {}: Invalid token `{}`\n", idx, current.get_value());
    exit(1);
}
Statement* Parser::parseStatement(){
    Statement* node = nullptr;
    Token token = this->currentToken();
    switch(token.get_type()){
        case TokenType::IMPORT: {
            node = this->parseImportStatement();
        } break;
        case TokenType::OPENBRACE: {
            node = this->parseCompoundStatement();
        } break;
        default: {
            fmt::print("ICE: Unhandled statement token\n");
            fmt::print("or parser failed to detect statement identifiers correctly `{}`\n", token.get_value());
            fmt::print("int token type {}\n", (int)token.get_type());
            exit(1);
        } break;
    }
    return node;
}
ImportStatement* Parser::parseImportStatement(){
    this->expect(TokenType::IMPORT, "ICE: Token pointer for parser was probably invalid and or it advanced without the parser knowing\n");
    std::string path = this->expect(TokenType::IDENTIFIER, "Error: Expected a module name\n").get_value();
    this->expect(TokenType::SEMICOLON, "Error: import statement has no ending `;`\n");
    return new ImportStatement(path);
}
CompoundStatement* Parser::parseCompoundStatement(){
    this->expect(TokenType::OPENBRACE, "ICE: Token pointer for parser was probably invalid and or it advanced without the parser knowing\n");
    Ast* ast = new Ast;
    while(this->currentToken().get_type() != TokenType::CLOSEBRACE && this->currentToken().get_type() != TokenType::TEOF){
        ast->pushNode(this->nextNode());
    }
    if(this->currentToken().get_type() == TokenType::TEOF){
        fmt::print("Error: Expected `{}` but got end of file\n", '}');
        exit(1);
    }
    this->expect(TokenType::CLOSEBRACE, "ICE: current token was not lexed correct\n");
    return new CompoundStatement(ast);
}
FuncDecl* Parser::parseFuncDecl(){
    this->expect(TokenType::FUNC, "ICE: Token pointer for parser was probably invalid and or it advanced without the parser knowing\n");
    std::string name = this->expect(TokenType::IDENTIFIER, "Error: Expected a function name\n").get_value();
    FuncDecl* funcDecl = new FuncDecl(name);
    this->expect(TokenType::OPENPAREN, "Error: Expected `(`\n");
    while(this->currentToken().get_type() != TokenType::CLOSEPAREN && this->currentToken().get_type() != TokenType::TEOF){
        funcDecl->pushParam(this->parseFuncParam());
    }
    if(this->currentToken().get_type() == TokenType::TEOF){
        fmt::print("Error: Expected `)` but got end of file\n");
        exit(1);
    }
    this->expect(TokenType::CLOSEPAREN, "Error: Expected `)`\n");
    TypeAnnotation* return_Type = this->parseTypeAnnotation();
    funcDecl->setReturnType(return_Type);
    if(this->currentToken().get_type() == TokenType::SEMICOLON){
        this->expect(TokenType::SEMICOLON, "ICE: Parser failed to correctly detect a semicolon\n");
        return funcDecl;
    }
    this->expect(TokenType::RIGHT_ARROW, "Error: Expected `=>` to start parsing compound expression\n");
    Statement* body = this->parseStatement();
}
FuncParam* Parser::parseFuncParam(){
    std::string param_name = this->expect(TokenType::IDENTIFIER, "Error: Expected a function parameter name\n").get_value();
    TypeAnnotation *annotation = this->parseTypeAnnotation();
    return new FuncParam(param_name, annotation);
}
TypeAnnotation* Parser::parseTypeAnnotation(){
    this->expect(TokenType::COLON, "Error: Expected a `:` to start a type\n");
    std::string type_name = this->expect(TokenType::IDENTIFIER, "Error: Expected a type name\n").get_value();
    return new TypeAnnotation(StringToType(type_name));
}
Ast* Parser::parseAst(){
    Ast* ast = new Ast;
    while(this->currentToken().get_type() != TokenType::TEOF){
        AstNode* node = this->nextNode();
        if(node == nullptr){
            break;
        }
        ast->pushNode(node);
    }
    return ast;
}
};