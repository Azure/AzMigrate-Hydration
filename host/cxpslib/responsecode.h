
///
///  \file responsecode.h
///
///  \brief mapping internal codes to protocl codes and vice-versa
///

#ifndef RESPONSECODE_H
#define RESPONSECODE_H

/// \brief used for managing/mapping internal to protocol and vice-versa response codes
class ResponseCode {
public:

    /// \brief holds code and text of code
    struct Code {
        Code(int code,                                ///< code
             std::string const& str = std::string())  ///< text version of code
            : m_code(code),
              m_str(str)
            {}

        /// \ copy constructor
        Code(Code const& code) ///< code to copy
            {
                this->m_code = code.m_code;
                this->m_str = code.m_str;
            }

        /// \brief assigment operator
        Code& operator=(Code const& code) ///< code to assign to this code
            {
                if (this != &code) {
                    this->m_code = code.m_code;
                    this->m_str = code.m_str;
                }
                return *this;
            }

        int m_code;          ///< code
        std::string m_str;   ///< text string for code
    };

    /// \brief for mapping internal to protocol and vice-versa
    typedef std::map<int, Code> CodeLookUp_t;

    /// \brief internal codes
    /// the names indicate the code
    enum Codes {
        RESPONSE_OK = 0,
        RESPONSE_NOT_FOUND,
        RESPONSE_REQUESTER_ERROR,
        RESPONSE_INTERNAL_ERROR,
        RESPONSE_UNKNOWN,
        RESPONSE_REQUESTER_THROTTLED
    };

    ResponseCode()
        : m_internalOk(RESPONSE_OK, "OK"),
          m_internalNotfound(RESPONSE_NOT_FOUND, "NOT FOUND"),
          m_internalRequesterError(RESPONSE_REQUESTER_ERROR, "REQUESTER ERROR"),
          m_internalError(RESPONSE_INTERNAL_ERROR, "INTERNAL ERROR"),
          m_internalUnknown(RESPONSE_UNKNOWN, "UNKNONW RESPONSE"),
          m_internalInlineThrottled(RESPONSE_REQUESTER_THROTTLED, "THROTTLED")
        { }

    virtual ~ResponseCode() {}

    /// \brief get the protocol code for a given internal code
    virtual Code internalToProtocol(int code) = 0;

    /// \brief get the internal code for a given protocol code
    virtual Code protocolToInternal(int code) = 0;

    /// \brief get the internal OK code
    Code internalOk() {
        return m_internalOk;
    }

    /// \brief get the internal not found code
    Code internalNotFound() {
        return m_internalNotfound;
    }

    /// \brief get the internal requester error code
    Code internalRequesterError() {
        return m_internalRequesterError;
    }

    /// \brief get the internal error code
    Code internalError() {
        return m_internalError;
    }

    /// \brief get the internal unknonw code
    Code internalUnknown() {
        return m_internalUnknown;
    }

    Code internalInlineThrottled() {
        return m_internalInlineThrottled;
    }

protected:

private:

    Code m_internalOk;              ///< holds internal OK code
    Code m_internalNotfound;        ///< holds internal not found code
    Code m_internalRequesterError;  ///< holds internal requester error code
    Code m_internalError;           ///< holds internal error code
    Code m_internalUnknown;         ///< holds internal unknown code
    Code m_internalInlineThrottled; ///< holds inline throttled code
};

/// \brief HTTP response codes and mappings to internal codes
class HttpResponseCode : public ResponseCode {
public:
    HttpResponseCode()
        : m_code500(500, "500 INTERNAL ERROR")
        {

            Code code200(200, "200 OK");
            Code code404(404, "404 NOT FOUND");
            Code code400(400, "400 BAD REQUEST");
            Code code507(507, "507 THROTTLED");

            m_byInternal.insert(std::make_pair(RESPONSE_OK, code200));
            m_byInternal.insert(std::make_pair(RESPONSE_NOT_FOUND, code404));
            m_byInternal.insert(std::make_pair(RESPONSE_REQUESTER_ERROR, code400));
            m_byInternal.insert(std::make_pair(RESPONSE_INTERNAL_ERROR, m_code500));
            m_byInternal.insert(std::make_pair(RESPONSE_REQUESTER_THROTTLED, code507));

            m_byProtocol.insert(std::make_pair(code200.m_code, internalOk()));
            m_byProtocol.insert(std::make_pair(code404.m_code, internalNotFound()));
            m_byProtocol.insert(std::make_pair(code400.m_code, internalRequesterError()));
            m_byProtocol.insert(std::make_pair(m_code500.m_code, internalError()));
            m_byProtocol.insert(std::make_pair(code507.m_code, internalInlineThrottled()));
        }

    virtual ~HttpResponseCode() {}

    /// \brief get the protocol code for the given internal code
    virtual Code internalToProtocol(int code)  ///< code internal code to look up corrosponding protocol code
        {
            CodeLookUp_t::iterator iter(m_byInternal.find(code));
            if (m_byInternal.end() == iter) {
                return m_code500;
            } else {
                return (*iter).second;
            }
        }

    /// \brief get the internal code for the given protocol code
    virtual Code protocolToInternal(int code) ///< code protocol code to look up corrosponding internal code
        {
            CodeLookUp_t::iterator iter(m_byProtocol.find(code));
            if (m_byProtocol.end() == iter) {
                return internalUnknown();
            } else {
                return (*iter).second;
            }
        };


protected:

private:
    Code m_code500;  ///< holds protocol 500 code

    CodeLookUp_t m_byInternal;  ///< holds internal code to protocol mapping
    CodeLookUp_t m_byProtocol;  ///< holds protovol code to internal mapping

};


#endif // RESPONSECODE_H
