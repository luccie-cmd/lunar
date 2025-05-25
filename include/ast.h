#if !defined(_LANGUAGE_AST_H_)
#define _LANGUAGE_AST_H_
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace language {
class StatementNode;
class ExpressionNode;
class DeclarationNode;
class TypeSpec;
enum struct AstNodeType {
    Statement,
    Declaration,
    Expression,
    TypeSpec,
    Attribute,
};
class AstNode {
  public:
    AstNode(AstNodeType type);
    virtual ~AstNode()                = 0;
    virtual void print(size_t indent) = 0;
    AstNodeType  getAstType();

  protected:
    AstNodeType __astNodeType;
};
using AttributeData = std::variant<std::string, uint64_t, bool>;
enum struct AttributeType {
    Public,
    Private,
    NoMangle,
};
class AttributeNode : public AstNode {
  public:
    AttributeNode(AttributeType type, AttributeData data);
    ~AttributeNode();
    void print(size_t indent);

  private:
    AttributeType type;
    AttributeData data;
};
class TypeSpec : public AstNode {
  public:
    TypeSpec(size_t pointerLevel, std::string name);
    ~TypeSpec();
    void        print(size_t indent);
    std::string getName();
    bool        isInteger();
    bool        isUnsigned();
    size_t      getBitSize();
    size_t      getPointerCount();
    bool        operator!=(TypeSpec other) {
        return !(this->pointerLevel == other.pointerLevel && this->name == other.name);
    }

  private:
    size_t      pointerLevel;
    std::string name;
};
enum struct StatementNodeType {
    If,
    Compound,
    Expression,
    Declaration,
    Return,
};
class StatementNode : public AstNode {
  public:
    StatementNode(StatementNodeType type);
    virtual ~StatementNode()               = 0;
    virtual void      print(size_t indent) = 0;
    StatementNodeType getStmtType();

  protected:
    StatementNodeType __stmtNodeType;
};
class ReturnStatementNode : public StatementNode {
  public:
    ReturnStatementNode(ExpressionNode* expr);
    ~ReturnStatementNode();
    void            print(size_t indent);
    ExpressionNode* getExpr();

  private:
    ExpressionNode* retExpr;
};
class IfStatementNode : public StatementNode {
  public:
    IfStatementNode(ExpressionNode* condition, StatementNode* trueBody,
                    std::optional<StatementNode*> falseBody);
    ~IfStatementNode();
    void print(size_t indent);

  private:
    ExpressionNode*               condition;
    StatementNode*                trueBody;
    std::optional<StatementNode*> falseBody;
};
class CompoundStatementNode : public StatementNode {
  public:
    CompoundStatementNode(std::vector<StatementNode*> nodes);
    ~CompoundStatementNode();
    void                        print(size_t indent);
    std::vector<StatementNode*> getNodes();

  private:
    std::vector<StatementNode*> nodes;
};
class ExpressionStatementNode : public StatementNode {
  public:
    ExpressionStatementNode(ExpressionNode* expr);
    ~ExpressionStatementNode();
    void print(size_t indent);

  private:
    ExpressionNode* expr;
};
class DeclarationStatementNode : public StatementNode {
  public:
    DeclarationStatementNode(DeclarationNode* declNode);
    ~DeclarationStatementNode();
    void             print(size_t indent);
    DeclarationNode* getDeclNode();

  private:
    DeclarationNode* declNode;
};
enum struct DeclarationNodeType {
    Class,
    Function,
    Variable,
    Parameter,
};
class DeclarationNode : public AstNode {
  public:
    DeclarationNode(DeclarationNodeType type);
    virtual ~DeclarationNode()               = 0;
    virtual void        print(size_t indent) = 0;
    DeclarationNodeType getDeclType();

  protected:
    DeclarationNodeType __declNodeType;
};
class ParameterDeclarationNode : public DeclarationNode {
  public:
    ParameterDeclarationNode(std::string name, TypeSpec* type);
    ~ParameterDeclarationNode();
    void        print(size_t indent);
    std::string getName();
    TypeSpec*   getType();

  private:
    std::string name;
    TypeSpec*   type;
};
class FunctionDeclarationNode : public DeclarationNode {
  public:
    FunctionDeclarationNode(std::string name, std::vector<AttributeNode*> attrs,
                            std::vector<DeclarationNode*> params, TypeSpec* returnType,
                            StatementNode* body);
    ~FunctionDeclarationNode();
    void                          print(size_t indent);
    std::string                   getName();
    TypeSpec*                     getReturnType();
    std::vector<AttributeNode*>   getAttribs();
    std::vector<DeclarationNode*> getParams();
    StatementNode*                getBody();

  private:
    std::string                   name;
    std::vector<AttributeNode*>   attrs;
    std::vector<DeclarationNode*> params;
    TypeSpec*                     returnType;
    StatementNode*                body;
};
class ClassDeclarationNode : public DeclarationNode {
  public:
    ClassDeclarationNode(std::string name, StatementNode* body);
    ~ClassDeclarationNode();
    void print(size_t indent);

  private:
    std::string    name;
    StatementNode* body;
};
class VariableDeclarationNode : public DeclarationNode {
  public:
    VariableDeclarationNode(std::string name, std::vector<AttributeNode*> attribs, TypeSpec* type,
                            std::optional<ExpressionNode*> value);
    ~VariableDeclarationNode();
    void                           print(size_t indent);
    std::string                    getName();
    std::vector<AttributeNode*>    getAttribs();
    TypeSpec*                      getType();
    std::optional<ExpressionNode*> getValue();

  private:
    std::string                    name;
    std::vector<AttributeNode*>    attribs;
    TypeSpec*                      type;
    std::optional<ExpressionNode*> value;
};
enum struct ExpressionNodeType {
    MemberAccess,
    Assignment,
    FunctionCall,
    Binary,
    Unary,
    Cast,
    StringLiteral,
    IdentifierLiteral,
    NumericLiteral,

    LtoRValue,
};
enum struct ValueCatagory {
    Lvalue,
    Rvalue,
};
class ExpressionNode : public AstNode {
  public:
    ExpressionNode(ExpressionNodeType type);
    virtual ~ExpressionNode()               = 0;
    virtual void       print(size_t indent) = 0;
    ExpressionNodeType getExprType();
    ValueCatagory      getValCatagory();

  protected:
    ExpressionNodeType __exprNodeType;
    ValueCatagory      __valCatagory;
};
class LtoRValueCastExpression : public ExpressionNode {
  public:
    LtoRValueCastExpression(ExpressionNode* Lvalue);
    ~LtoRValueCastExpression();
    void            print(size_t indent);
    ExpressionNode* getExpr();

  private:
    ExpressionNode* Lvalue;
};
class UnaryExpressionNode : public ExpressionNode {
  public:
    UnaryExpressionNode(std::string unaryOp, ExpressionNode* expr);
    ~UnaryExpressionNode();
    void print(size_t indent);

  private:
    std::string     unaryOp;
    ExpressionNode* expr;
};
class BinaryExpressionNode : public ExpressionNode {
  public:
    BinaryExpressionNode(ExpressionNode* lhs, ExpressionNode* rhs, std::string _operator);
    ~BinaryExpressionNode();
    void            print(size_t indent);
    ExpressionNode* getLhs();
    ExpressionNode* getRhs();
    std::string     getOperator();

  private:
    ExpressionNode* lhs;
    ExpressionNode* rhs;
    std::string     _operator;
};
class FunctionCallExpressionNode : public ExpressionNode {
  public:
    FunctionCallExpressionNode(ExpressionNode* callee, std::vector<ExpressionNode*> arguments);
    ~FunctionCallExpressionNode();
    void print(size_t indent);

  private:
    ExpressionNode*              callee;
    std::vector<ExpressionNode*> arguments;
};
class IdentifierLiteralExpressionNode : public ExpressionNode {
  public:
    IdentifierLiteralExpressionNode(std::string value);
    ~IdentifierLiteralExpressionNode();
    void        print(size_t indent);
    std::string getValue();

  private:
    std::string value;
};
class NumericLiteralExpressionNode : public ExpressionNode {
  public:
    NumericLiteralExpressionNode(std::string value);
    ~NumericLiteralExpressionNode();
    void        print(size_t indent);
    std::string getValue();

  private:
    std::string value;
};
class StringLiteralExpressionNode : public ExpressionNode {
  public:
    StringLiteralExpressionNode(std::string value);
    ~StringLiteralExpressionNode();
    void print(size_t indent);

  private:
    std::string value;
};
class AssignmentExpressionNode : public ExpressionNode {
  public:
    AssignmentExpressionNode(ExpressionNode* assignee, ExpressionNode* value);
    ~AssignmentExpressionNode();
    void print(size_t indent);

  private:
    ExpressionNode* assignee;
    ExpressionNode* value;
};
class CastExpressionNode : public ExpressionNode {
  public:
    CastExpressionNode(ExpressionNode* value, TypeSpec* type);
    ~CastExpressionNode();
    void            print(size_t indent);
    ExpressionNode* getValue();
    TypeSpec*       getType();

  private:
    ExpressionNode* value;
    TypeSpec*       type;
};
class MemberAccessExpressionNode : public ExpressionNode {
  public:
    MemberAccessExpressionNode(ExpressionNode* parent, ExpressionNode* property);
    ~MemberAccessExpressionNode();
    void print(size_t indent);

    ExpressionNode* getParent();
    ExpressionNode* getProperty();

  private:
    ExpressionNode* parent;
    ExpressionNode* property;
};
class Ast {
  public:
    Ast();
    ~Ast();
    void                  addNode(AstNode* node);
    std::vector<AstNode*> getNodes();
    void                  print();

  private:
    std::vector<AstNode*> nodes;
};
}; // namespace language

#endif // _LANGUAGE_AST_H_