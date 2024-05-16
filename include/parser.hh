#if !defined(_LUNAR_PARSER_HH_)
#define _LUNAR_PARSER_HH_
#include <vector>
#include "token.hh"
#include "lexer.hh"
#include "ast.hh"

namespace lunar{
class Parser{
    private:
        std::vector<Token> tokens;
        Token current, next;
        size_t idx;
        inline Token nextToken(){
            try{
                return tokens.at(idx++);
            } catch(std::out_of_range e){
                (void)e;
                return Token("\0", TokenType::TEOF);
            }
        }
        inline Token peekToken(int peek_count){
            return tokens.at(idx+peek_count-1);
        }
        inline Token currentToken(){
            return peekToken(0);
        }
        inline Token expect(TokenType type, std::string error){
            if(currentToken().get_type() != type){
                fmt::print("{} {}", idx, error);
                exit(1);
            }
            return nextToken();
        }
        ImportStatement* parseImportStatement();
        CompoundStatement* parseCompoundStatement();
        Statement* parseStatement();
        FuncDecl* parseFuncDecl();
        FuncParam* parseFuncParam();
        TypeAnnotation* parseTypeAnnotation();
    public:
        Parser(Lexer lexer) :current("Invalid", TokenType::INVALID), next("Invalid", TokenType::INVALID){
            tokens = lexer.lex_all_tokens();
            idx = 2;
            current = this->nextToken();
            next = this->nextToken();
        }
        AstNode* nextNode();
        Ast* parseAst();
};
};

#endif // _LUNAR_PARSER_HH_
