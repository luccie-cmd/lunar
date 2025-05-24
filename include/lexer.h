#if !defined(_LANGUAGE_SYNTAX_LEXER_H_)
#define _LANGUAGE_SYNTAX_LEXER_H_
#include "token.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace language {
class Lexer {
  public:
    Lexer(std::string _data) {
        this->data     = _data;
        this->curr     = 0;
        this->next_char();
    }
    void                removeWhitespace();
    Token*              nextToken();
    Token*              lexIdentifierOrKeyword();
    Token*              lexString();
    Token*              lexNumber(uint8_t base);
    std::vector<Token*> lexAllTokens();

  private:
    std::string   data;
    std::uint64_t curr;
    char          c;
    inline bool   at_eof() {
        return (this->c == '\0');
    }
    inline void next_char() {
        if (this->curr < this->data.size()) {
            this->c = this->data.at(this->curr++);
        } else {
            this->c = '\0';
        }
    }
};
} // namespace language

#endif // _LANGUAGE_SYNTAX_LEXER_H_
