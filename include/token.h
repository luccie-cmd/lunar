#if !defined(_LANGUAGE_SYNTAX_TOKEN_H_)
#define _LANGUAGE_SYNTAX_TOKEN_H_
#include <string>

namespace language {
enum struct TokenType {
    Invalid          = 0,
    Eof              = 1,
    Openparen        = '(',
    Closeparen       = ')',
    Openbrace        = '{',
    Closebrace       = '}',
    Semicolon        = ';',
    Colon            = ':',
    At               = '@',
    Dot              = '.',
    Comma            = ',',
    Equal            = '=',
    Plus             = '+',
    Percent          = '%',
    Star             = '*',
    Minus            = '-',
    __MultibyteStart = 255,
    ColonColon,
    EqualEqual,
    Identifier,
    LitString,
    LitNumber,
    __KeywordsStart = 300,
    Import,
    Class,
    Func,
    Var,
    Attrib,
    If,
    Else,
    As,
    Return,
    Void,
    U64,
    U32,
    I32,
    String,
    Variadic,
};
class Token {
  private:
    std::string value;
    TokenType   type;

  public:
    Token(std::string _value, TokenType _type) {
        this->value = _value;
        this->type  = _type;
    }
    ~Token() {}
    std::string& get_value() {
        return this->value;
    };
    TokenType get_type() {
        return this->type;
    };
    void set_value(std::string _value) {
        this->value = _value;
    }
    void set_type(TokenType _type) {
        this->type = _type;
    }
};
} // namespace language

#endif // _LANGUAGE_SYNTAX_TOKEN_H_
