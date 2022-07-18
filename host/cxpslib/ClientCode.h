#ifndef CLIENT_CODE_H
#define CLIENT_CODE_H

/// \brief request result codes
enum ClientCode {
    CLIENT_RESULT_OK = 0,
    CLIENT_RESULT_MORE_DATA,
    CLIENT_RESULT_NOT_FOUND,
    CLIENT_RESULT_ERROR,
    CLIENT_RESULT_NOSPACE
};

#endif // !CLIENT_CODE_H



