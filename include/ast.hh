#if !defined(_LUNAR_AST_HH_)
#define _LUNAR_AST_HH_
#include <string>
#include <variant>
#include <optional>
#include <vector>

namespace lunar{
class Ast;
enum struct AstType{
    INVALID,
    STATEMENT,
    FUNCTION_DECLERATION,
    FUNCTION_PARAM,
    TYPE_ANNOTATION,
};
enum struct StatementType{
    IMPORT,
    COMPOUND,
};
enum struct Type{
    UNKNOWN,
    INT,
    STRING,
    LIST,
};
Type StringToType(std::string string);
class TypeAnnotation{
    private:
        Type type;
        std::optional<TypeAnnotation*> child_type;
    public:
        TypeAnnotation(Type type) {
            this->type = type;
        }
        Type get_type() { return this->type; }
        std::optional<TypeAnnotation*> get_child_type() { return this->child_type; }
        void set_child_type(std::optional<TypeAnnotation*> type) { this->child_type = type; }
};
class FuncParam{
    private:
        std::string name;
        TypeAnnotation *type_annotation;
    public:
        FuncParam(std::string name, TypeAnnotation *type_annotation) :type_annotation(type_annotation){
            this->name = name;
        }
        std::string get_name() { return this->name; }
        TypeAnnotation *get_type_annotation() { return this->type_annotation; }
};
class FuncDecl{
    private:
        std::string name;
        std::vector<FuncParam*> params;
        TypeAnnotation *return_type;
    public:
        FuncDecl(std::string name) :return_type(nullptr){
            this->name = name;
        }
        void pushParam(FuncParam* param){ this->params.push_back(param); }
        void setReturnType(TypeAnnotation* type){ this->return_type = type; }
        std::string getName(){ return this->name; } 
};
class Statement{
    private:
        StatementType stmt_type;
    public:
        virtual ~Statement() = 0;
        StatementType getStmtType() { return this->stmt_type; }
        void setStmtType(StatementType type) { this->stmt_type = type; }
};
class ImportStatement : public Statement{
    private:
        std::string path;
    public:
        ImportStatement(std::string path){
            this->path = path;
            this->setStmtType(StatementType::IMPORT);
        }
        std::string getPath() { return this->path; }
        void setPath(std::string path) { this->path = path; }
};
class CompoundStatement : public Statement{
    private:
        Ast *ast;
    public:
        CompoundStatement(Ast *ast):ast(ast) { this->setStmtType(StatementType::COMPOUND); }
        Ast *getAst() { return ast; }
};
using AstMembers = std::variant<Statement*, FuncDecl*>;
AstType AstMembersToAstType(AstMembers node);
class AstNode{
    private:
        AstType ast_type;
        AstMembers members;
    public:
        AstNode(AstMembers node){
            this->members = node;
            this->ast_type = AstMembersToAstType(node);
        }
        template<typename T>
        T* ToType(AstType type) { if(this->ast_type == type) 
                                  return std::get<T*>(members); 
                              return nullptr; 
        }
        AstType getAstType() { return this->ast_type; }
        void setAstType(AstType type) { this->ast_type = type; }
        void print();
};
class Ast{
    private:
        std::vector<AstNode*> nodes;
    public:
        Ast(){}
        void pushNode(AstNode* node) { this->nodes.push_back(node); }
        std::vector<AstNode*> getNodes() { return nodes; }
        void print();
};
};

#endif // _LUNAR_AST_HH_
