#include "lexer.hh"
#include <vector>
#include <utility>

bool IsStart(char c) {
    return std::isalpha(c) || c == '_' || c == '$';
}

/// Check if a character is allowed in an identifier.
bool IsContinue(char c) {
    return IsStart(c) || isdigit(c);
}

bool IsBinary(char c) { return c == '0' || c == '1'; }
bool IsDecimal(char c) { return c >= '0' && c <= '9'; }
bool IsOctal(char c) { return c >= '0' && c <= '7'; }
bool IsHex(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }

namespace lunar{
static std::vector<std::pair<TokenType, std::string>> keywords = {
    {TokenType::VAR, "var"},
    {TokenType::FUNC, "func"},
    {TokenType::IMPORT, "import"},
};
// Tries to determine if a string is an identifier || a keyword
static TokenType get_keyword_or_identifier(std::string value){
    for(auto [type, name] : keywords){
        if(name == value){
            return type;
        }
    }
    return TokenType::IDENTIFIER;
}
void Lexer::remove_whitespace(){
    while(isspace(this->c)){ this->next_char(); };
}
Token Lexer::next_token(){
    this->remove_whitespace();
    // First going to check for the EOF
    if(this->at_eof()){
        return Token("EOF", TokenType::TEOF);
    }
    Token ret("Invalid", TokenType::INVALID);
    switch(this->c){
        case '@':
        case '(':
        case ')':
        case '{':
        case '}':
        case ';':
        case ',':
        case '*':
        case '/':
        case '.':
        case '#':
        case ':': {
            ret.set_type(static_cast<TokenType>(this->c));
            ret.set_value(std::string(1, this->c));
            this->next_char();
        } break;
        case '+': {
            this->next_char();
            if(this->c == '+'){
                this->next_char();
                ret.set_type(TokenType::PLUSPLUS);
            } else{
                ret.set_type(TokenType::PLUS);
            }
        } break;
        case '-': {
            this->next_char();
            if(this->c == '-'){
                this->next_char();
                ret.set_type(TokenType::MINUSMINUS);
            } else{
                ret.set_type(TokenType::MINUS);
            }
        } break;
        case '<': {
            this->next_char();
            if (this->c == '=') {
                this->next_char();
                ret.set_type(TokenType::LESSEQUAL);
            } else if (this->c == '<') {
                this->next_char();
                if (this->c == '=') {
                    this->next_char();
                    ret.set_type(TokenType::LESSLESSEQUAL);
                } else {
                    ret.set_type(TokenType::LESSLESS);
                }
            } else {
                ret.set_type(TokenType::LESS);
            }
        } break;
        case '>': {
            this->next_char();
            if (this->c == '=') {
                this->next_char();
                ret.set_type(TokenType::GREATEREQUAL);
            } else if (this->c == '>') {
                this->next_char();
                if (this->c == '=') {
                    this->next_char();
                    ret.set_type(TokenType::GREATERGREATEREQUAL);
                } else {
                    ret.set_type(TokenType::GREATERGREATER);
                }
            } else {
                ret.set_type(TokenType::GREATER);
            }
        } break;
        case '=': {
            this->next_char();
            if(this->c == '='){
                this->next_char();
                ret.set_type(TokenType::EQUALEQUAL);
            } else if(this->c == '>'){
                this->next_char();
                ret.set_type(TokenType::RIGHT_ARROW);
            } else{
                ret.set_type(TokenType::EQUAL);
            }
        } break;
        case '\"': {
            Token string = this->lex_string();
            ret.set_type(string.get_type());
            ret.set_value(string.get_value());
        } break;
        default: {
            if(IsStart(this->c)){
                Token identifer_or_keyword = this->lex_identifier_or_keyword();
                ret.set_type(identifer_or_keyword.get_type());
                ret.set_value(identifer_or_keyword.get_value());
            } else if(IsDecimal(this->c)){
                Token number = this->lex_number();
                ret.set_type(number.get_type());
                ret.set_value(number.get_value());
            } else{
                fmt::print("Invalid character found with the value of `{}`\n", this->c);
            }
        } break;
    }
    return ret;
}
Token Lexer::lex_identifier_or_keyword(){
    std::string buffer(1, this->c);
    this->next_char();
    while(IsContinue(this->c)){
        buffer.push_back(this->c);
        this->next_char();
    }
    TokenType tt = get_keyword_or_identifier(buffer);
    return Token(buffer, tt);
}
Token Lexer::lex_string(){
    this->next_char();
    std::string buffer(1, this->c);
    this->next_char();
    while(this->c != '\"'){
        buffer.push_back(this->c);
        this->next_char();
        if(this->at_eof()){
            fmt::print("ERROR: Missing terminating \" character\n");
            exit(1);
        }
    }
    this->next_char(); // Skip the "
    TokenType tt = TokenType::LIT_STRING;
    return Token(buffer, tt);
}
Token Lexer::lex_number(){
    std::string buffer(1, this->c);
    this->next_char();
    while(IsDecimal(this->c)){
        buffer.push_back(this->c);
        this->next_char();
    }
    TokenType tt = TokenType::LIT_NUMBER;
    return Token(buffer, tt);
}
std::vector<Token> Lexer::lex_all_tokens(){
    std::vector<Token> tokens;
    Token token = this->next_token();
    while(token.get_type() != TokenType::TEOF){
        tokens.push_back(token);
        if(token.get_type() == TokenType::INVALID){
            break;
        }
        token = this->next_token();
    }
    // Push EOF onto the tokens
    tokens.push_back(token);
    return tokens;
}
};