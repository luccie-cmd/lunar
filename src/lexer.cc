#include <lexer.h>
#include <utility>
#include <vector>

bool IsStart(char c) {
    return std::isalpha(c) || c == '_';
}

/// Check if a character is allowed in an identifier.
bool IsContinue(char c) {
    return IsStart(c) || isdigit(c);
}

bool IsBinary(char c) {
    return c == '0' || c == '1';
}
bool IsDecimal(char c) {
    return c >= '0' && c <= '9';
}
bool IsOctal(char c) {
    return c >= '0' && c <= '7';
}
bool IsHex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

namespace language {
void Lexer::removeWhitespace() {
    while (isspace(this->c)) {
        this->next_char();
    }
    while (this->c == '#' && !this->at_eof()) {
        while (this->c != '\n' && !this->at_eof()) {
            this->next_char();
        }
        this->removeWhitespace();
    }
}
Token* Lexer::nextToken() {
    this->removeWhitespace();
    // First going to check for the EOF
    if (this->at_eof()) {
        return new Token("Eof", TokenType::Eof);
    }
    Token* ret = new Token("Invalid", TokenType::Invalid);
    switch (this->c) {
    case '@':
    case '(':
    case ')':
    case '{':
    case '}':
    case ',':
    case ';':
    case '%':
    case '-':
    case '+':
    case '.':
    case '*': {
        ret->set_type(static_cast<TokenType>(this->c));
        ret->set_value(std::string(1, this->c));
        this->next_char();
    } break;
    case '=': {
        this->next_char();
        if (this->c == '=') {
            this->next_char();
            ret->set_type(TokenType::EqualEqual);
            ret->set_value("==");
        } else {
            ret->set_type(TokenType::Equal);
            ret->set_value(std::string(1, '='));
        }
    } break;
    case ':': {
        this->next_char();
        if (this->c == ':') {
            this->next_char();
            ret->set_type(TokenType::ColonColon);
            ret->set_value("::");
        } else {
            ret->set_type(TokenType::Colon);
            ret->set_value(std::string(1, ':'));
        }
    } break;
    case '\"': {
        Token* string = this->lexString();
        ret->set_type(string->get_type());
        ret->set_value(string->get_value());
    } break;
    default: {
        if (IsStart(this->c)) {
            Token* identifer_or_keyword = this->lexIdentifierOrKeyword();
            ret->set_type(identifer_or_keyword->get_type());
            ret->set_value(identifer_or_keyword->get_value());
        } else if (IsDecimal(this->c)) {
            uint8_t base = 10;
            if (this->curr < this->data.size()) {
                if (this->c == '0' && this->data.at(this->curr) == 'x') {
                    base = 16;
                    this->next_char();
                    this->next_char();
                }
            }
            Token* number = this->lexNumber(base);
            ret->set_type(number->get_type());
            ret->set_value(number->get_value());
        } else {
            std::printf("Invalid character found with the value of `%c`\n", this->c);
            std::exit(1);
        }
    } break;
    }
    return ret;
}
std::vector<std::pair<std::string, TokenType>> keywords = {
    {"import", TokenType::Import}, {"class", TokenType::Class},   {"func", TokenType::Func},
    {"var", TokenType::Var},       {"attrib", TokenType::Attrib}, {"if", TokenType::If},
    {"else", TokenType::Else},     {"as", TokenType::As},         {"return", TokenType::Return},
    {"void", TokenType::Void},     {"u64", TokenType::U64},       {"u32", TokenType::U32},
    {"i32", TokenType::I32},       {"String", TokenType::String}, {"Variadic", TokenType::Variadic},
};
TokenType getTokenTypeIdentifierKeyword(std::string buffer) {
    for (std::pair<std::string, TokenType> keyword : keywords) {
        if (buffer == keyword.first) {
            return keyword.second;
        }
    }
    return TokenType::Identifier;
}
Token* Lexer::lexIdentifierOrKeyword() {
    std::string buffer(1, this->c);
    this->next_char();
    while (IsContinue(this->c)) {
        buffer.push_back(this->c);
        this->next_char();
    }
    TokenType tt = getTokenTypeIdentifierKeyword(buffer);
    return new Token(buffer, tt);
}
Token* Lexer::lexString() {
    this->next_char();
    std::string buffer(1, this->c);
    this->next_char();
    while (this->c != '\"') {
        buffer.push_back(this->c);
        this->next_char();
        if (this->at_eof()) {
            std::printf("Missing terminating \" character\n");
        }
    }
    this->next_char(); // Skip the "
    TokenType tt = TokenType::LitString;
    return new Token(buffer, tt);
}
static bool inBase(char c, uint8_t base) {
    if (isascii(c)) {
        c = toupper(c);
    }
    if (base > 16)
        return false;
    else if (base <= 10) {
        if (!(c >= '0' && c < ('0' + base))) return false;
    } else {
        if (!((c >= '0' && c < ('0' + base)) || (c >= 'A' && c < ('A' + base - 10)))) return false;
    }
    return true;
}
Token* Lexer::lexNumber(uint8_t base) {
    std::string buffer(1, this->c);
    this->next_char();
    while (inBase(this->c, base)) {
        buffer.push_back(this->c);
        this->next_char();
    }
    TokenType tt = TokenType::LitNumber;
    return new Token(buffer, tt);
}
std::vector<Token*> Lexer::lexAllTokens() {
    std::vector<Token*> tokens;
    Token*              token = this->nextToken();
    while (token->get_type() != TokenType::Eof) {
        tokens.push_back(token);
        if (token->get_type() == TokenType::Invalid) {
            break;
        }
        token = this->nextToken();
    }
    // Push EOF onto the tokens
    tokens.push_back(token);
    return tokens;
}
}; // namespace language