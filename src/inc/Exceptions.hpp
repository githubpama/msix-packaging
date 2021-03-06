//
//  Copyright (C) 2017 Microsoft.  All rights reserved.
//  See LICENSE file in the project root for full license information.
// 
#pragma once

#include <cstdint>
#include <string>
#include <exception>
#include <cassert>
#include <functional>

#include "Log.hpp"
#include "MSIXWindows.hpp"
#include "xercesc/util/PlatformUtils.hpp"
#include "xercesc/sax/ErrorHandler.hpp"
#include "xercesc/sax/SAXParseException.hpp"
#include "xercesc/dom/DOM.hpp"

namespace MSIX {

    static const std::uint32_t ERROR_FACILITY       = 0x8BAD0000;              // Facility 2989
    static const std::uint32_t XERCES_SAX_FACILITY  = ERROR_FACILITY + 0x1000; // Xerces XMLException. 0x8BAD1000 + XMLException error code
    static const std::uint32_t XERCES_XML_FACILITY  = ERROR_FACILITY + 0x2000;
    static const std::uint32_t XERCES_DOM_FACILITY  = ERROR_FACILITY + 0x3000;

    // defines error codes
    enum class Error : std::uint32_t
    {
        //
        // Win32 error codes
        //
        OK                          = 0x00000000,
        NotImplemented              = 0x80004001,
        NoInterface                 = 0x80004002,
        Unexpected                  = 0x8000ffff,
        FileNotFound                = 0x80070002,
        OutOfMemory                 = 0x8007000E,
        NotSupported                = 0x80070032,
        InvalidParameter            = 0x80070057,
        Stg_E_Invalidpointer        = 0x80030009,

        //
        // msix specific error codes
        //

        // Basic file errors
        FileOpen                    = ERROR_FACILITY + 0x0001,
        FileSeek                    = ERROR_FACILITY + 0x0002,
        FileRead                    = ERROR_FACILITY + 0x0003,
        FileWrite                   = ERROR_FACILITY + 0x0003,
        FileCreateDirectory         = ERROR_FACILITY + 0x0004,
        FileSeekOutOfRange          = ERROR_FACILITY + 0x0005,

        // Zip format errors
        ZipCentralDirectoryHeader   = ERROR_FACILITY + 0x0011,
        ZipLocalFileHeader          = ERROR_FACILITY + 0x0012,
        Zip64EOCDRecord             = ERROR_FACILITY + 0x0013,
        Zip64EOCDLocator            = ERROR_FACILITY + 0x0014,
        ZipEOCDRecord               = ERROR_FACILITY + 0x0015,
        ZipHiddenData               = ERROR_FACILITY + 0x0016,
        ZipBadExtendedData          = ERROR_FACILITY + 0x0017,

        // Inflate errors
        InflateInitialize           = ERROR_FACILITY + 0x0021,
        InflateRead                 = ERROR_FACILITY + 0x0022,
        InflateCorruptData          = ERROR_FACILITY + 0x0023,

        // Package format errors
        MissingAppxSignatureP7X     = ERROR_FACILITY + 0x0031,
        MissingContentTypesXML      = ERROR_FACILITY + 0x0032,
        MissingAppxBlockMapXML      = ERROR_FACILITY + 0x0033,
        MissingAppxManifestXML      = ERROR_FACILITY + 0x0034,
        DuplicateFootprintFile      = ERROR_FACILITY + 0x0035,
        UnknownFileNameEncoding     = ERROR_FACILITY + 0x0036,

        // Signature errors
        SignatureInvalid            = ERROR_FACILITY + 0x0041,
        CertNotTrusted              = ERROR_FACILITY + 0x0042,
        PublisherMismatch           = ERROR_FACILITY + 0x0043,

        // Blockmap semantic errors
        BlockMapSemanticError       = ERROR_FACILITY + 0x0051,

        // AppxManifest semantic errors
        AppxManifestSemanticError   = ERROR_FACILITY + 0x0061,

        // XML parsing errors
        XercesWarning               = XERCES_SAX_FACILITY + 0x0001,
        XercesError                 = XERCES_SAX_FACILITY + 0x0002,
        XercesFatal                 = XERCES_SAX_FACILITY + 0x0003,
    };

    // Defines a common exception type to throw in exceptional cases.  DO NOT USE FOR FLOW CONTROL!
    // Throwing MSIX::Exception will break into the debugger on chk builds to aid debugging
    class Exception : public std::exception
    {
    public:
        Exception(Error error) : m_code(static_cast<std::uint32_t>(error))
        {}

        Exception(std::uint32_t error) : m_code(0x80070000 + error)
        {}

        Exception(Error error, std::string& message) :
            m_code(static_cast<std::uint32_t>(error)),
            m_message(message)
        {
            Global::Log::Append(Message());
        }

        Exception(Error error, const char* message) :
            m_code(static_cast<std::uint32_t>(error)),
            m_message(message)
        {
            Global::Log::Append(Message());
        }

        Exception(HRESULT error, std::string& message) :
            m_code(error),
            m_message(message)
        {
            Global::Log::Append(Message());
        }

        Exception(HRESULT error, const char* message) :
            m_code(error),
            m_message(message)
        {
            Global::Log::Append(Message());
        }

        uint32_t            Code() { return m_code; }
        const std::string&  Message() { return m_message; }

    protected:
        std::uint32_t   m_code;
        std::string     m_message;
    };

    class Win32Exception : public Exception
    {
    public:
        Win32Exception(DWORD error, std::string& message) :
            Exception(0x80070000 + error, message)
        {
            Global::Log::Append(Message());
        }

        Win32Exception(DWORD error, const char* message) :
            Exception(0x80070000 + error, message)
        {
            Global::Log::Append(Message());
        }
    };

    class NtStatusException : public Exception
    {
    public:
        NtStatusException(NTSTATUS error, std::string& message) :
            Exception(static_cast<std::uint32_t>(error), message)
        {
            Global::Log::Append(Message());
        }

        NtStatusException(NTSTATUS error, const char* message) :
            Exception(static_cast<std::uint32_t>(error), message)
        {
            Global::Log::Append(Message());
        }
    };

    class ParsingException : public XERCES_CPP_NAMESPACE::ErrorHandler
    {
    public:
        ParsingException() {};
        ~ParsingException() {};

        void warning(const XERCES_CPP_NAMESPACE::SAXParseException& exp) override
        {
            // TODO: add message, line number and column
            assert(false);
            throw Exception(MSIX::Error::XercesWarning);
        }

        void error(const XERCES_CPP_NAMESPACE::SAXParseException& exp) override
        {
            // TODO: add message, line number and column
            assert(false);
            throw Exception(MSIX::Error::XercesError);
        }

        void fatalError(const XERCES_CPP_NAMESPACE::SAXParseException& exp) override
        {
            // TODO: add message, line number and column
            assert(false);
            throw Exception(MSIX::Error::XercesFatal);
        }

        void resetErrors() override {}
    };

    // Provides an ABI exception boundary with parameter validation
    template <class Lambda>
    inline HRESULT ResultOf(Lambda lambda)
    {
        HRESULT hr = static_cast<HRESULT>(MSIX::Error::OK);
        try
        {
            lambda();
        }
        catch (MSIX::Exception& e)
        {
            hr = static_cast<HRESULT>(e.Code());
        }
        catch (std::bad_alloc&)
        {
            hr = static_cast<HRESULT>(MSIX::Error::OutOfMemory);
        }
        catch (std::exception&)
        {
            hr = static_cast<HRESULT>(MSIX::Error::Unexpected);
        }
        catch (const XERCES_CPP_NAMESPACE::XMLException& e)
        {
            hr = static_cast<HRESULT>(MSIX::XERCES_XML_FACILITY) +
                static_cast<HRESULT>(e.getCode());
        }
        catch (const XERCES_CPP_NAMESPACE::DOMException& e)
        {
            hr = static_cast<HRESULT>(MSIX::XERCES_DOM_FACILITY) +
                static_cast<HRESULT>(e.code);
        }

        return hr;
    }
}

// Helper to make code more terse and more readable at the same time.
#define ThrowErrorIfNot(c, a, m)     \
{                                    \
    if (!(a))                        \
    {                                \
        assert(false);               \
        throw MSIX::Exception(c,m);  \
    }                                \
}

#define ThrowWin32ErrorIfNot(c, a, m)       \
{                                           \
    if (!(a))                               \
    {                                       \
        assert(false);                      \
        throw MSIX::Win32Exception(c,m);    \
    }                                       \
}

#define ThrowErrorIf(c, a, m) ThrowErrorIfNot(c,!(a), m)

#define ThrowHrIfFailed(a)                              \
{                                                       \
    HRESULT hr = a;                                     \
    if (FAILED(hr))                                     \
    {   assert(false);                                  \
        throw MSIX::Exception(hr, "COM Call failed");   \
    }                                                   \
}
