#if !defined(_LUNAR_ERROR_HH_)
#define _LUNAR_ERROR_HH_

namespace lunar{
namespace error{
enum struct LexerError{
    NONE,
    NO_DATA,
    INVALID_CHAR,
    NO_CLOSING_QUOTE,
};
};
};

#endif // _LUNAR_ERROR_HH_
