#include "ast.h"
#include "lexer.h"

namespace language {
class Parser {
  public:
    Parser(Lexer* lexer);
    ~Parser();
    Ast* getAst();

  private:
    ExpressionNode*               parseExpression();
    ExpressionNode*               parseCastExpression();
    ExpressionNode*               parseAssignmentExpression();
    ExpressionNode*               parseInfixExpression(size_t precedence);
    ExpressionNode*               parseUnaryExpression();
    ExpressionNode*               parsePostFixExpression();
    ExpressionNode*               parsePrimaryExpression();
    StatementNode*                parseStatement();
    StatementNode*                parseReturnStatement();
    StatementNode*                parseCompoundStatement();
    StatementNode*                parseIfStatement();
    std::vector<DeclarationNode*> parseDecl();
    std::vector<DeclarationNode*> parseImportDecl();
    DeclarationNode*              parseFuncDecl();
    DeclarationNode*              parseClassDecl();
    DeclarationNode*              parseParamDecl();
    DeclarationNode*              parseVarDecl();
    std::vector<AttributeNode*>   parseAttributes();
    TypeSpec*                     parseTypeSpecWithColon();
    TypeSpec*                     parseTypeSpec();
    void                          parse();
    void                          advance();
    Token*                        expect(TokenType type, bool advance);
    Token*                        expect(std::vector<TokenType> types, bool advance);
    Token*                        getCurrentToken();
    Token*                        peek(size_t lookAhead);
    std::vector<Token*>           tokens;
    size_t                        tokensIndex;
    Ast*                          retAst;
};
}; // namespace language