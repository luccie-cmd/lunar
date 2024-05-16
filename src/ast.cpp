#include "ast.hh"
#include <fmt/core.h>
#include <assert.h>

namespace lunar{
AstType AstMembersToAstType(AstMembers node){
    if (std::holds_alternative<Statement*>(node)){
        return AstType::STATEMENT;
    }
    return AstType::INVALID;
}
std::vector<std::pair<std::string, Type>> str_types = {
    {"int", Type::INT},
    {"str", Type::STRING},
    {"list", Type::LIST},
};
Type StringToType(std::string string){
    for(auto [name, type] : str_types){
        if (name == string){
            return type;
        }
    }
    return Type::UNKNOWN;
}

Statement::~Statement(){}
void AstNode::print(){
    fmt::print("TODO: Print the node\n");
}
void Ast::print(){
    fmt::print("TODO: Print the ast\n");
}
};