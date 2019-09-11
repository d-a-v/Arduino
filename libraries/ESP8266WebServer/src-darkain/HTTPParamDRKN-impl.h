#ifndef __HTTP_PARAM_H__
#define __HTTP_PARAM_H__




#include "HTTPValue.h"


namespace WebServerDarkain {

class HTTPParam : public HTTPValue {


	////////////////////////////////////////////////////////////////////////////
	// CALCULATE THE TOTAL NUMBER OF PARAM VALUES
	////////////////////////////////////////////////////////////////////////////
	protected:
	virtual int _count(const char *buffer) const;




	////////////////////////////////////////////////////////////////////////////
	// PARSE AND TOKENIZE THE BUFFER
	////////////////////////////////////////////////////////////////////////////
	protected:
	virtual char *_parse(char *buffer, int id=0);




	////////////////////////////////////////////////////////////////////////////
	// DECODE URL ENCODING - FAST INLINE MEMORY REPLACE WITHIN THE STRING
	////////////////////////////////////////////////////////////////////////////
	protected:
	static char *_decode(char *text);

};

////////////////////////////////////////////////////////////////////////////////
// CALCULATE THE TOTAL NUMBER OF HEADER ROWS
////////////////////////////////////////////////////////////////////////////////
int HTTPParam::_count(const char *buffer) const {
	if (buffer == nullptr  ||  *buffer == '\0') return 0;

	int total = 1;

	while (*buffer) {
		if (*buffer == '&') total++;
		buffer++;
	}

	return total;
}




////////////////////////////////////////////////////////////////////////////////
// PARSE AND TOKENIZE THE BUFFER
////////////////////////////////////////////////////////////////////////////////
char *HTTPParam::_parse(char *buffer, int id) {
	if (buffer == nullptr  ||  *buffer == '\0') return buffer;

	char	*param_key		= buffer;
	char	*param_value	= nullptr;

	while (true) {
		if (*buffer == '&'  ||  *buffer == '\0') {
			bool end = (*buffer == '\0');

			*buffer++ = '\0';

			set(id++, _decode(param_key), _decode(param_value));

			if (end) break;

			param_key	= buffer;
			param_value	= nullptr;

		} else if (*buffer == '='  &&  param_value == nullptr) {
			*buffer++ = '\0';
			param_value = buffer;

		} else {
			buffer++;
		}
	}

	return buffer;
}




////////////////////////////////////////////////////////////////////////////////
// DECODE URL ENCODING - FAST INLINE MEMORY REPLACE WITHIN THE STRING
////////////////////////////////////////////////////////////////////////////////
char *HTTPParam::_decode(char *buffer) {
	if (buffer == nullptr) return buffer;

	char *src	= buffer;
	char *dst	= buffer;
	char temp[]	= "00";

	while (*src) {
		switch (*src) {
			case '%':
				if (!src[1]  ||  !src[2]) goto finished;

				temp[0] = src[1];
				temp[1] = src[2];

				src += 2;

				*dst = (char) strtol(temp, nullptr, 16);
				if (*dst == '\0') *dst = ' ';
			break;


			case '+':
				*dst = ' ';
			break;


			default:
				*dst = *src;
		}

		src++;
		dst++;
	}


finished:
	*dst = '\0';

	return buffer;
}

}; // namespace WebServerDarkain

#endif //__HTTP_PARAM_H__
