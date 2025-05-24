#if !defined(_LANGUAGE_SEMA_H_)
#define _LANGUAGE_SEMA_H_
#include "ast.h"

#include <stack>
#include <unordered_map>

namespace language {
struct Symbol {
    std::string                 name;
    TypeSpec*                   type;
    DeclarationNodeType         kind;
    std::vector<AttributeNode*> attrs;
};
struct SymbolTable {
    std::vector<std::string>                 allowedTypes;
    std::unordered_map<std::string, Symbol*> symbols;
    SymbolTable*                             parent;
    std::string                              name;
    Symbol*                                  lookup(std::string lookupName) {
        if (this->symbols.contains(lookupName)) {
            return this->symbols.at(lookupName);
        } else {
            if (this->parent) {
                return this->parent->lookup(lookupName);
            }
            return nullptr;
        }
    }
    void insert(Symbol* symbol) {
        if (this->symbols.contains(symbol->name)) {
            std::printf("ICE: Attempted to double insert a symbol\n");
            std::exit(1);
        }
        this->symbols.insert({symbol->name, symbol});
    }
    bool isTypeAllowed(std::string type) {
        for (std::string t : this->allowedTypes) {
            if (t == type) {
                return true;
            }
        }
        if (this->parent) {
            return this->parent->isTypeAllowed(type);
        }
        return false;
    }
};
class Sema {
  public:
    Sema(Ast* oldAst);
    ~Sema();
    void doChecks();
    Ast* getNewAst();

  private:
    DeclarationNode*         checkFunctionDecl(FunctionDeclarationNode* node);
    DeclarationNode*         checkParamDecl(ParameterDeclarationNode* node);
    DeclarationNode*         checkDeclarationNode(DeclarationNode* node);
    DeclarationNode*         checkVarDecl(VariableDeclarationNode* node);
    StatementNode*           checkCompoundStatement(CompoundStatementNode* node);
    StatementNode*           checkReturnStatement(ReturnStatementNode* node);
    StatementNode*           checkStatement(StatementNode* node);
    ExpressionNode*          checkBinaryExpression(BinaryExpressionNode* node);
    ExpressionNode*          checkExpression(ExpressionNode* node);
    TypeSpec*                checkTypeSpec(TypeSpec* type);
    AstNode*                 checkTopAstNode(AstNode* node);
    Ast*                     newAst;
    Ast*                     oldAst;
    SymbolTable*             getCurrentTable();
    std::stack<SymbolTable*> tables;
};
}; // namespace language

#endif // _LANGUAGE_SEMA_H_
