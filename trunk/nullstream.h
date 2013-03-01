//
// Copyright Maciej Sobczak, 2002
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#ifndef NULLSTREAM_H_INCLUDED
#define NULLSTREAM_H_INCLUDED

#include <streambuf>
#include <ostream>

// null stream buffer class
template <class charT, class traits = std::char_traits<charT> >
class NullBuffer : public std::basic_streambuf<charT, traits>
{
public:
typedef typename std::basic_streambuf<charT, traits>::int_type int_type;

NullBuffer() {}

private:
virtual int_type overflow(int_type c)
{
	// just ignore the character
	return traits::not_eof(c);
}
};

// generic null output stream class
template <class charT, class traits = std::char_traits<charT> >
class GenericNullStream
: private NullBuffer<charT, traits>,
public std::basic_ostream<charT, traits>
{
public:
GenericNullStream()
: std::basic_ostream<charT, traits>(this)
{
}
};

// helper declarations for narrow and wide streams
typedef GenericNullStream<char> NullStream;
typedef GenericNullStream<wchar_t> WNullStream;

#endif // NULLSTREAM_H_INCLUDED
