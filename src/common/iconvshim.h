/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * Internal Windows fallback for iconv API compatibility.
 */

#ifndef XOREOS_COMMON_ICONVSHIM_H
#define XOREOS_COMMON_ICONVSHIM_H

#ifdef XOREOS_INTERNAL_ICONV

#ifndef _WIN32
#error "XOREOS_INTERNAL_ICONV is only supported on Windows"
#endif

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <windows.h>

typedef void *iconv_t;

namespace Common {

struct IconvContext {
	std::string toCode;
	std::string fromCode;
};

inline int iconvCodePage(const std::string &encoding) {
	if (encoding == "ASCII")
		return 20127;
	if (encoding == "UTF-8")
		return CP_UTF8;
	if (encoding == "ISO-8859-15")
		return 28605;
	if (encoding == "WINDOWS-1250")
		return 1250;
	if (encoding == "WINDOWS-1251")
		return 1251;
	if (encoding == "WINDOWS-1252")
		return 1252;
	if (encoding == "CP932")
		return 932;
	if (encoding == "CP936")
		return 936;
	if (encoding == "CP949")
		return 949;
	if (encoding == "CP950")
		return 950;
	return 0;
}

inline bool bytesToUtf16(const std::string &encoding, const char *input, size_t size,
                         std::vector<wchar_t> &out) {
	if (encoding == "UTF-16LE") {
		if ((size % 2) != 0)
			return false;
		out.resize(size / 2);
		for (size_t i = 0; i < out.size(); ++i) {
			const uint8_t lo = static_cast<uint8_t>(input[i * 2]);
			const uint8_t hi = static_cast<uint8_t>(input[i * 2 + 1]);
			out[i] = static_cast<wchar_t>((hi << 8) | lo);
		}
		return true;
	}

	if (encoding == "UTF-16BE") {
		if ((size % 2) != 0)
			return false;
		out.resize(size / 2);
		for (size_t i = 0; i < out.size(); ++i) {
			const uint8_t hi = static_cast<uint8_t>(input[i * 2]);
			const uint8_t lo = static_cast<uint8_t>(input[i * 2 + 1]);
			out[i] = static_cast<wchar_t>((hi << 8) | lo);
		}
		return true;
	}

	const int codePage = iconvCodePage(encoding);
	if (codePage == 0)
		return false;

	const DWORD flags = (codePage == CP_UTF8) ? MB_ERR_INVALID_CHARS : 0;
	const int wlen = MultiByteToWideChar(codePage, flags, input, static_cast<int>(size), nullptr, 0);
	if (wlen <= 0)
		return false;

	out.resize(wlen);
	return MultiByteToWideChar(codePage, flags, input, static_cast<int>(size), out.data(), wlen) > 0;
}

inline bool utf16ToBytes(const std::string &encoding, const wchar_t *input, size_t wcharCount,
                         std::vector<char> &out) {
	if (encoding == "UTF-16LE") {
		out.resize(wcharCount * 2);
		for (size_t i = 0; i < wcharCount; ++i) {
			const uint16_t v = static_cast<uint16_t>(input[i]);
			out[i * 2] = static_cast<char>(v & 0xFF);
			out[i * 2 + 1] = static_cast<char>((v >> 8) & 0xFF);
		}
		return true;
	}

	if (encoding == "UTF-16BE") {
		out.resize(wcharCount * 2);
		for (size_t i = 0; i < wcharCount; ++i) {
			const uint16_t v = static_cast<uint16_t>(input[i]);
			out[i * 2] = static_cast<char>((v >> 8) & 0xFF);
			out[i * 2 + 1] = static_cast<char>(v & 0xFF);
		}
		return true;
	}

	const int codePage = iconvCodePage(encoding);
	if (codePage == 0)
		return false;

	const DWORD flags = (codePage == CP_UTF8) ? WC_ERR_INVALID_CHARS : 0;
	const int blen = WideCharToMultiByte(codePage, flags, input, static_cast<int>(wcharCount), nullptr, 0, nullptr, nullptr);
	if (blen <= 0)
		return false;

	out.resize(blen);
	return WideCharToMultiByte(codePage, flags, input, static_cast<int>(wcharCount), out.data(), blen, nullptr, nullptr) > 0;
}

} // End of namespace Common

inline iconv_t iconv_open(const char *tocode, const char *fromcode) {
	if (!tocode || !fromcode) {
		errno = EINVAL;
		return reinterpret_cast<iconv_t>(-1);
	}

	std::unique_ptr<Common::IconvContext> ctx(new Common::IconvContext);
	ctx->toCode = tocode;
	ctx->fromCode = fromcode;

	// Conversion manager only requires UTF-8 <-> known encodings.
	const bool valid = (ctx->toCode == "UTF-8") || (ctx->fromCode == "UTF-8");
	if (!valid) {
		errno = EINVAL;
		return reinterpret_cast<iconv_t>(-1);
	}

	return reinterpret_cast<iconv_t>(ctx.release());
}

inline int iconv_close(iconv_t cd) {
	if (!cd || cd == reinterpret_cast<iconv_t>(-1)) {
		errno = EINVAL;
		return -1;
	}

	Common::IconvContext *ctx = reinterpret_cast<Common::IconvContext *>(cd);
	delete ctx;
	return 0;
}

inline size_t iconv(iconv_t cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft) {
	if (!cd || cd == reinterpret_cast<iconv_t>(-1)) {
		errno = EINVAL;
		return static_cast<size_t>(-1);
	}

	if (!inbuf || !*inbuf || !inbytesleft) {
		// Reset state (stateless shim).
		return 0;
	}

	if (!outbuf || !*outbuf || !outbytesleft) {
		errno = EINVAL;
		return static_cast<size_t>(-1);
	}

	Common::IconvContext *ctx = reinterpret_cast<Common::IconvContext *>(cd);

	std::vector<wchar_t> wide;
	if (!Common::bytesToUtf16(ctx->fromCode, *inbuf, *inbytesleft, wide)) {
		errno = EILSEQ;
		return static_cast<size_t>(-1);
	}

	std::vector<char> converted;
	if (!Common::utf16ToBytes(ctx->toCode, wide.data(), wide.size(), converted)) {
		errno = EILSEQ;
		return static_cast<size_t>(-1);
	}

	if (converted.size() > *outbytesleft) {
		errno = E2BIG;
		return static_cast<size_t>(-1);
	}

	std::memcpy(*outbuf, converted.data(), converted.size());

	*inbuf += *inbytesleft;
	*inbytesleft = 0;
	*outbuf += converted.size();
	*outbytesleft -= converted.size();

	return 0;
}

#endif // XOREOS_INTERNAL_ICONV

#endif // XOREOS_COMMON_ICONVSHIM_H
