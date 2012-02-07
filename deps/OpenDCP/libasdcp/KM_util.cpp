/*
Copyright (c) 2005-2011, John Hurst
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
  /*! \file    KM_util.cpp
    \version $Id: KM_util.cpp,v 1.38 2011/05/13 01:50:19 jhurst Exp $
    \brief   Utility functions
  */

#include <KM_util.h>
#include <KM_prng.h>
#include <KM_memio.h>
#include <KM_fileio.h>
#include <KM_log.h>
#include <KM_tai.h>
#include <KM_mutex.h>
#include <ctype.h>
#include <list>
#include <map>
#include <string>

const char*
Kumu::Version()
{
  return PACKAGE_VERSION;
}


//------------------------------------------------------------------------------------------

// Result_t Internals

struct map_entry_t
{
  int rcode;
  Kumu::Result_t* result;
};


// WIN32 does not init this in time for use with Result_t(...) below, so it is
// now a pointer that Result_t(...) fills in when it needs it.
static Kumu::Mutex* s_MapLock = 0;

static ui32_t s_MapSize = 0;
static const ui32_t MapMax = 2048;
static struct map_entry_t s_ResultMap[MapMax];


//
const Kumu::Result_t&
Kumu::Result_t::Find(int v)
{
  if ( v == 0 )
    return RESULT_OK;

  assert(s_MapLock);
  AutoMutex L(*s_MapLock);

  for ( ui32_t i = 0; i < s_MapSize; ++i )
    {
      if ( s_ResultMap[i].rcode == v )
	return *s_ResultMap[i].result;
    }

  return RESULT_UNKNOWN;
}

//
Kumu::Result_t
Kumu::Result_t::Delete(int v)
{
  if ( v < -99 || v > 99 )
    {
      DefaultLogSink().Error("Cannot delete core result code: %ld\n", v);
      return RESULT_FAIL;
    }

  assert(s_MapLock);
  AutoMutex L(*s_MapLock);

  for ( ui32_t i = 0; i < s_MapSize; ++i )
    {
      if ( s_ResultMap[i].rcode == v )
	{
	  for ( ++i; i < s_MapSize; ++i )
	    s_ResultMap[i-1] = s_ResultMap[i];

	  --s_MapSize;
	  return RESULT_OK;
	}
    }

  return RESULT_FALSE;
}

//
unsigned int
Kumu::Result_t::End()
{
  return s_MapSize;
}

//
const Kumu::Result_t&
Kumu::Result_t::Get(unsigned int i)
{
  return *s_ResultMap[i].result;
}

//
Kumu::Result_t::Result_t(int v, const char* s, const char* l) : value(v), symbol(s), label(l)
{
  assert(l);
  assert(s);

  if ( v == 0 )
    return;

  // This may seem tricky, but it is certain that the static values declared in KM_error.h will
  // be created (and thus this method will be called) before main(...) is called.  It is not
  // until then that threads could be created, thus the mutex will exist before multi-threaded
  // access could occur.
  if ( s_MapLock == 0 )
    s_MapLock = new Kumu::Mutex;

  assert(s_MapLock);
  AutoMutex L(*s_MapLock);

  for ( ui32_t i = 0; i < s_MapSize; ++i )
    {
      if ( s_ResultMap[i].rcode == v )
	return;
    }

  assert(s_MapSize+1 < MapMax);

  s_ResultMap[s_MapSize].rcode = v;
  s_ResultMap[s_MapSize].result = this;
  ++s_MapSize;

  return;
}

Kumu::Result_t::~Result_t() {}


//------------------------------------------------------------------------------------------
// DTrace internals

static int s_DTraceSequence = 0;

Kumu::DTrace_t::DTrace_t(const char* Label, Kumu::Result_t* Watch, int Line, const char* File)
  : m_Label(Label), m_Watch(Watch), m_Line(Line), m_File(File)
{
  m_Sequence = s_DTraceSequence++;
  DefaultLogSink().Debug("@enter %s[%d] (%s at %d)\n", m_Label, m_Sequence, m_File, m_Line);
}

Kumu::DTrace_t::~DTrace_t()
{
  if ( m_Watch != 0  )
    DefaultLogSink().Debug("@exit %s[%d]: %s\n", m_Label, m_Sequence, m_Watch->Label());
  else
    DefaultLogSink().Debug("@exit %s[%d]\n", m_Label, m_Sequence);
}

//------------------------------------------------------------------------------------------


const char  fill = '=';
const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const byte_t decode_map[] =
{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 62,   0xff, 0xff, 0xff, 63,
  52,   53,   54,   55,   56,   57,   58,   59,
  60,   61,   0xff, 0xff, 0xff, 0xfe, 0xff, 0xff,
  0xff, 0,    1,    2,    3,    4,    5,    6,
  7,    8,    9,    10,   11,   12,   13,   14,
  15,   16,   17,   18,   19,   20,   21,   22,
  23,   24,   25,   0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 26,   27,   28,   29,   30,   31,   32,
  33,   34,   35,   36,   37,   38,   39,   40,
  41,   42,   43,   44,   45,   46,   47,   48,
  49,   50,   51,   0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};


// Convert a binary string to NULL-terminated UTF-8 hexadecimal, returns the buffer
// if the binary buffer was large enough to hold the result. If the output buffer
// is too small or any of the pointer arguments are NULL, the subroutine will
// return 0.
//
const char*
Kumu::base64encode(const byte_t* buf, ui32_t buf_len, char* strbuf, ui32_t strbuf_len)
{
  ui32_t out_char = 0;
  ui32_t i, block_len, diff;

  if ( buf == 0 || strbuf == 0 )
    return 0;

  if ( strbuf_len < base64_encode_length(buf_len) + 1 )
    return 0;

  block_len = buf_len;

  while ( block_len % 3 )
    block_len--;

  for ( i = 0; i < block_len; i += 3 )
    {
      strbuf[out_char++] = base64_chars[( buf[0] >> 2 )];
      strbuf[out_char++] = base64_chars[( ( ( buf[0] & 0x03 ) << 4 ) | ( buf[1] >> 4 ) )];
      strbuf[out_char++] = base64_chars[( ( ( buf[1] & 0x0f ) << 2 ) | ( buf[2] >> 6 ) )];
      strbuf[out_char++] = base64_chars[( buf[2] & 0x3f )];
      buf += 3;
    }

  if ( i < buf_len )
    {
      diff = buf_len - i;
      assert(diff > 0);
      assert(diff < 3);
      
      strbuf[out_char++] = base64_chars[( buf[0] >> 2 )];

      if ( diff == 1 )
	{
	  strbuf[out_char++] = base64_chars[( ( ( buf[0] & 0x03 ) << 4 ) )];
	  strbuf[out_char++] = fill;
	}
      else if ( diff == 2 )
	{
	  strbuf[out_char++] = base64_chars[( ( ( buf[0] & 0x03 ) << 4 ) | ( buf[1] >> 4 ) )];
	  strbuf[out_char++] = base64_chars[( ( ( buf[1] & 0x0f ) << 2 ) )];
	}

      strbuf[out_char++] = fill;
    }

  strbuf[out_char] = 0;
  return strbuf;;
}




// Convert NULL-terminated UTF-8 Base64 string to binary, returns 0 if
// the binary buffer was large enough to hold the result. The output parameter
// 'char_count' will contain the length of the converted string. If the output
// buffer is too small or any of the pointer arguments are NULL, the subroutine
// will return -1 and set 'char_count' to the required buffer size. No data will
// be written to 'buf' if the subroutine fails.
//
i32_t
Kumu::base64decode(const char* str, byte_t* buf, ui32_t buf_len, ui32_t* char_count)
{
  register byte_t c = 0, d = 0;
  register ui32_t phase = 0, i = 0;

  if ( str == 0 || buf == 0 || char_count == 0 )
    return -1;

  while ( *str != 0 && i < buf_len )
    {
      c = decode_map[(int)*str++];
      if ( c == 0xff ) continue;
      if ( c == 0xfe ) break;

      switch ( phase++ )
	{
	case 0:
	  buf[i++] =  c << 2;
	  break;

	case 1:
	  buf[i - 1] |= c >> 4;
	  d = c;
	  break;

	case 2:
	  buf[i++] =  ( d << 4 ) | ( c >> 2 );
	  d = c;
	  break;

	case 3:
	  buf[i++] =  ( d << 6 ) | c;
	  phase = 0;
	  break;
	}
    }

  *char_count = i;
  return 0;
}

//------------------------------------------------------------------------------------------

// convert utf-8 hext string to bin
i32_t
Kumu::hex2bin(const char* str, byte_t* buf, ui32_t buf_len, ui32_t* conv_size)
{
  KM_TEST_NULL_L(str);
  KM_TEST_NULL_L(buf);
  KM_TEST_NULL_L(conv_size);

  *conv_size = 0;

  if ( str[0] == 0 ) // nothing to convert
    return 0;

  for ( int j = 0; str[j]; j++ )
    {
      if ( isxdigit(str[j]) )
	(*conv_size)++;
    }

  if ( *conv_size & 0x01 ) (*conv_size)++;
  *conv_size /= 2;

  if ( *conv_size > buf_len )// maximum possible data size
    return -1;

  *conv_size = 0;

  int phase = 0; // track high/low nybble

  // for each character, fill in the high nybble then the low
  for ( int i = 0; str[i]; i++ )
    {
      if ( ! isxdigit(str[i]) )
        continue;

      byte_t val = str[i] - ( isdigit(str[i]) ? 0x30 : ( isupper(str[i]) ? 0x37 : 0x57 ) );

      if ( phase == 0 )
        {
          buf[*conv_size] = val << 4;
          phase++;
        }
      else
        {
          buf[*conv_size] |= val;
          phase = 0;
          (*conv_size)++;
        }
    }

  return 0;
}

#ifdef CONFIG_RANDOM_UUID

// convert a memory region to a NULL-terminated hexadecimal string
//
static const char*
bin2hex_rand(const byte_t* bin_buf, ui32_t bin_len, char* str_buf, ui32_t str_len)
{
  if ( bin_buf == 0
       || str_buf == 0
       || ((bin_len * 2) + 1) > str_len )
    return 0;

  char* p = str_buf;
  Kumu::mem_ptr<byte_t> rand_buf = new byte_t[bin_len];
  Kumu::FortunaRNG RNG;
  RNG.FillRandom(rand_buf, bin_len);

  for ( ui32_t i = 0; i < bin_len; i++ )
    {
      *p = (bin_buf[i] >> 4) & 0x0f;
      *p += *p < 10 ? 0x30 : (( ((rand_buf[i] & 0x01) == 0) ? 0x61 : 0x41 ) - 10);
      p++;

      *p = bin_buf[i] & 0x0f;
      *p += *p < 10 ? 0x30 : (( (((rand_buf[i] >> 1) & 0x01) == 0) ? 0x61 : 0x41 ) - 10);
      p++;
    }

  *p = '\0';
  return str_buf;
}
#endif

// convert a memory region to a NULL-terminated hexadecimal string
//
const char*
Kumu::bin2hex(const byte_t* bin_buf, ui32_t bin_len, char* str_buf, ui32_t str_len)
{
  if ( bin_buf == 0
       || str_buf == 0
       || ((bin_len * 2) + 1) > str_len )
    return 0;

#ifdef CONFIG_RANDOM_UUID
  const char* use_random_uuid  = getenv("KM_USE_RANDOM_UUID");
  if ( use_random_uuid != 0 && use_random_uuid[0] != 0 && use_random_uuid[0] != '0' )
    return bin2hex_rand(bin_buf, bin_len, str_buf, str_len);
#endif

  char* p = str_buf;

  for ( ui32_t i = 0; i < bin_len; i++ )
    {
      *p = (bin_buf[i] >> 4) & 0x0f;
      *p += *p < 10 ? 0x30 : 0x61 - 10;
      p++;

      *p = bin_buf[i] & 0x0f;
      *p += *p < 10 ? 0x30 : 0x61 - 10;
      p++;
    }

  *p = '\0';
  return str_buf;
}


// spew a range of bin data as hex
void
Kumu::hexdump(const byte_t* buf, ui32_t dump_len, FILE* stream)
{
  if ( buf == 0 )
    return;

  if ( stream == 0 )
    stream = stderr;

  static ui32_t row_len = 16;
  const byte_t* p = buf;
  const byte_t* end_p = p + dump_len;

  for ( ui32_t line = 0; p < end_p; line++ )
    {
      fprintf(stream, "  %06x: ", line);
      ui32_t i;
      const byte_t* pp;

      for ( pp = p, i = 0; i < row_len && pp < end_p; i++, pp++ )
	fprintf(stream, "%02x ", *pp);

      while ( i++ < row_len )
	fputs("   ", stream);

      for ( pp = p, i = 0; i < row_len && pp < end_p; i++, pp++ )
	fputc((isprint(*pp) ? *pp : '.'), stream);

      fputc('\n', stream);
      p += row_len;
    }
}

//
const char*
Kumu::bin2UUIDhex(const byte_t* bin_buf, ui32_t bin_len, char* str_buf, ui32_t str_len)
{
  ui32_t i, j, k;

  if ( str_len < 34 || bin_len != UUID_Length )
    return 0;

  if ( bin2hex(bin_buf, bin_len, str_buf, str_len) == 0 )
    return 0;

  // shift the node id
  for ( k = 19, i = 12; i > 0; i-- )
    str_buf[k+i+4] = str_buf[k+i];

  // shift the time (mid+hi+clk)
  for ( k = 15, j = 3; k > 6; k -= 4, j-- )
    {
      for ( i = 4; i > 0; i-- )
        str_buf[k+i+j] = str_buf[k+i];
    }

  // add in the hyphens and trainling null
  for ( i = 8; i < 24; i += 5 )
    str_buf[i] = '-';
  
  str_buf[36] = 0;
  return str_buf;
}

//
void
Kumu::GenRandomValue(UUID& ID)
{
  byte_t tmp_buf[UUID_Length];
  GenRandomUUID(tmp_buf);
  ID.Set(tmp_buf);
}

//
void
Kumu::GenRandomUUID(byte_t* buf)
{
  FortunaRNG RNG;
  RNG.FillRandom(buf, UUID_Length);
  buf[6] &= 0x0f; // clear bits 4-7
  buf[6] |= 0x40; // set UUID version
  buf[8] &= 0x3f; // clear bits 6&7
  buf[8] |= 0x80; // set bit 7
}

//
void
Kumu::GenRandomValue(SymmetricKey& Key)
{
  byte_t tmp_buf[SymmetricKey_Length];
  FortunaRNG RNG;
  RNG.FillRandom(tmp_buf, SymmetricKey_Length);
  Key.Set(tmp_buf);
}


//------------------------------------------------------------------------------------------
// read a ber value from the buffer and compare with test value.
// Advances buffer to first character after BER value.
//
bool
Kumu::read_test_BER(byte_t **buf, ui64_t test_value)
{
  if ( buf == 0 )
    return false;

  if ( ( **buf & 0x80 ) == 0 )
    return false;

  ui64_t val = 0;
  ui8_t ber_size = ( **buf & 0x0f ) + 1;

  if ( ber_size > 9 )
    return false;

  for ( ui8_t i = 1; i < ber_size; i++ )
    {
      if ( (*buf)[i] > 0 )
        val |= (ui64_t)((*buf)[i]) << ( ( ( ber_size - 1 ) - i ) * 8 );
    }

  *buf += ber_size;
  return ( val == test_value );
}


//
bool
Kumu::read_BER(const byte_t* buf, ui64_t* val)
{
  ui8_t ber_size, i;
  
  if ( buf == 0 || val == 0 )
    return false;

  if ( ( *buf & 0x80 ) == 0 )
    return false;

  *val = 0;
  ber_size = ( *buf & 0x0f ) + 1;

  if ( ber_size > 9 )
    return false;

  for ( i = 1; i < ber_size; i++ )
    {
      if ( buf[i] > 0 )
	*val |= (ui64_t)buf[i] << ( ( ( ber_size - 1 ) - i ) * 8 );
    }

  return true;
}


static const ui64_t ber_masks[9] =
  { ui64_C(0xffffffffffffffff), ui64_C(0xffffffffffffff00), 
    ui64_C(0xffffffffffff0000), ui64_C(0xffffffffff000000),
    ui64_C(0xffffffff00000000), ui64_C(0xffffff0000000000),
    ui64_C(0xffff000000000000), ui64_C(0xff00000000000000),
    0
  };

//
ui32_t
Kumu::get_BER_length_for_value(ui64_t val)
{
  for ( ui32_t i = 0; i < 9; i++ )
    {
      if ( ( val & ber_masks[i] ) == 0 )
	return i + 1;
    }

  ui64Printer tmp_i(val);
  DefaultLogSink().Error("BER integer encoding not supported for large value %s\n", tmp_i.c_str());
  return 0;
}

//
bool
Kumu::write_BER(byte_t* buf, ui64_t val, ui32_t ber_len)
{
  if ( buf == 0 )
    return false;

  if ( ber_len == 0 )
    { // calculate default length
      if ( val < 0x01000000L )
        ber_len = 4;
      else if ( val < ui64_C(0x0100000000000000) )
        ber_len = 8;
      else
        ber_len = 9;
    }
  else
    { // sanity check BER length
      if ( ber_len > 9 )
        {
          DefaultLogSink().Error("BER integer length %u exceeds maximum size of 9\n", ber_len);
          return false;
        }
      
      if ( ( val & ber_masks[ber_len - 1] ) != 0 )
        {
	  ui64Printer tmp_i(val);
          DefaultLogSink().Error("BER integer length %u too small for value %s\n", ber_len, tmp_i.c_str());
          return false;
        }
    }

  buf[0] = 0x80 + ( ber_len - 1 );

  for ( ui32_t i = ber_len - 1; i > 0; i-- )
    {
      buf[i] = (ui8_t)(val & 0xff);
      val >>= 8;
    }

  return true;
}


//------------------------------------------------------------------------------------------
#ifdef KM_WIN32

#define TIMESTAMP_TO_SYSTIME(ts, t) \
  (t)->wYear    = (ts).Year;   /* year */ \
  (t)->wMonth   = (ts).Month;  /* month of year (1 - 12) */ \
  (t)->wDay     = (ts).Day;    /* day of month (1 - 31) */ \
  (t)->wHour    = (ts).Hour;   /* hours (0 - 23) */ \
  (t)->wMinute  = (ts).Minute; /* minutes (0 - 59) */ \
  (t)->wSecond  = (ts).Second; /* seconds (0 - 60) */ \
  (t)->wDayOfWeek = 0; \
  (t)->wMilliseconds = 0

#define SYSTIME_TO_TIMESTAMP(t, ts) \
  (ts).Year   = (t)->wYear;    /* year */ \
  (ts).Month  = (t)->wMonth;   /* month of year (1 - 12) */ \
  (ts).Day    = (t)->wDay;     /* day of month (1 - 31) */ \
  (ts).Hour   = (t)->wHour;    /* hours (0 - 23) */ \
  (ts).Minute = (t)->wMinute;  /* minutes (0 - 59) */ \
  (ts).Second = (t)->wSecond;  /* seconds (0 - 60) */

//
Kumu::Timestamp::Timestamp() :
  Year(0), Month(0),  Day(0), Hour(0), Minute(0), Second(0)
{
  SYSTEMTIME sys_time;
  GetSystemTime(&sys_time);
  SYSTIME_TO_TIMESTAMP(&sys_time, *this);
}

//
bool
Kumu::Timestamp::operator<(const Timestamp& rhs) const
{
  SYSTEMTIME lhst, rhst;
  FILETIME lft, rft;

  TIMESTAMP_TO_SYSTIME(*this, &lhst);
  TIMESTAMP_TO_SYSTIME(rhs, &rhst);
  SystemTimeToFileTime(&lhst, &lft);
  SystemTimeToFileTime(&rhst, &rft);
  return ( CompareFileTime(&lft, &rft) == -1 );
}

//
bool
Kumu::Timestamp::operator>(const Timestamp& rhs) const
{
  SYSTEMTIME lhst, rhst;
  FILETIME lft, rft;

  TIMESTAMP_TO_SYSTIME(*this, &lhst);
  TIMESTAMP_TO_SYSTIME(rhs, &rhst);
  SystemTimeToFileTime(&lhst, &lft);
  SystemTimeToFileTime(&rhst, &rft);
  return ( CompareFileTime(&lft, &rft) == 1 );
}

inline ui64_t
seconds_to_ns100(ui32_t seconds)
{
  return ((ui64_t)seconds * 10000000);
}

//
void
Kumu::Timestamp::AddDays(i32_t days)
{
  SYSTEMTIME current_st;
  FILETIME current_ft;
  ULARGE_INTEGER current_ul;

  if ( days != 0 )
    {
      TIMESTAMP_TO_SYSTIME(*this, &current_st);
      SystemTimeToFileTime(&current_st, &current_ft);
      memcpy(&current_ul, &current_ft, sizeof(current_ul));
      current_ul.QuadPart += ( seconds_to_ns100(86400) * (i64_t)days );
      memcpy(&current_ft, &current_ul, sizeof(current_ft));
      FileTimeToSystemTime(&current_ft, &current_st);
      SYSTIME_TO_TIMESTAMP(&current_st, *this);
    }
}

//
void
Kumu::Timestamp::AddHours(i32_t hours)
{
  SYSTEMTIME current_st;
  FILETIME current_ft;
  ULARGE_INTEGER current_ul;

  if ( hours != 0 )
    {
      TIMESTAMP_TO_SYSTIME(*this, &current_st);
      SystemTimeToFileTime(&current_st, &current_ft);
      memcpy(&current_ul, &current_ft, sizeof(current_ul));
      current_ul.QuadPart += ( seconds_to_ns100(3600) * (i64_t)hours );
      memcpy(&current_ft, &current_ul, sizeof(current_ft));
      FileTimeToSystemTime(&current_ft, &current_st);
      SYSTIME_TO_TIMESTAMP(&current_st, *this);
    }
}

//
void
Kumu::Timestamp::AddMinutes(i32_t minutes)
{
  SYSTEMTIME current_st;
  FILETIME current_ft;
  ULARGE_INTEGER current_ul;

  if ( minutes != 0 )
    {
      TIMESTAMP_TO_SYSTIME(*this, &current_st);
      SystemTimeToFileTime(&current_st, &current_ft);
      memcpy(&current_ul, &current_ft, sizeof(current_ul));
      current_ul.QuadPart += ( seconds_to_ns100(60) * (i64_t)minutes );
      memcpy(&current_ft, &current_ul, sizeof(current_ft));
      FileTimeToSystemTime(&current_ft, &current_st);
      SYSTIME_TO_TIMESTAMP(&current_st, *this);
    }
}

//
void
Kumu::Timestamp::AddSeconds(i32_t seconds)
{
  SYSTEMTIME current_st;
  FILETIME current_ft;
  ULARGE_INTEGER current_ul;

  if ( seconds != 0 )
    {
      TIMESTAMP_TO_SYSTIME(*this, &current_st);
      SystemTimeToFileTime(&current_st, &current_ft);
      memcpy(&current_ul, &current_ft, sizeof(current_ul));
      current_ul.QuadPart += ( seconds_to_ns100(1) * (i64_t)seconds );
      memcpy(&current_ft, &current_ul, sizeof(current_ft));
      FileTimeToSystemTime(&current_ft, &current_st);
      SYSTIME_TO_TIMESTAMP(&current_st, *this);
    }
}

#else // KM_WIN32

#include <time.h>

#define TIMESTAMP_TO_CALTIME(ts, ct)				       \
  (ct)->date.year  = (ts).Year;    /* year */			       \
  (ct)->date.month = (ts).Month;   /* month of year (1 - 12) */	       \
  (ct)->date.day   = (ts).Day;     /* day of month (1 - 31) */	       \
  (ct)->hour       = (ts).Hour;    /* hours (0 - 23) */		       \
  (ct)->minute     = (ts).Minute;  /* minutes (0 - 59) */	       \
  (ct)->second     = (ts).Second;  /* seconds (0 - 60) */	       \
  (ct)->offset     = 0;

#define CALTIME_TO_TIMESTAMP(ct, ts)				       \
  assert((ct)->offset == 0);					       \
  (ts).Year   = (ct)->date.year;    /* year */			       \
  (ts).Month  = (ct)->date.month;   /* month of year (1 - 12) */       \
  (ts).Day    = (ct)->date.day;     /* day of month (1 - 31) */	       \
  (ts).Hour   = (ct)->hour;         /* hours (0 - 23) */	       \
  (ts).Minute = (ct)->minute;       /* minutes (0 - 59) */	       \
  (ts).Second = (ct)->second;       /* seconds (0 - 60) */


//
Kumu::Timestamp::Timestamp() :
  Year(0), Month(0), Day(0), Hour(0), Minute(0), Second(0)
{
  Kumu::TAI::tai now;
  Kumu::TAI::caltime ct;
  now.now();
  ct = now;
  CALTIME_TO_TIMESTAMP(&ct, *this)
}

//
bool
Kumu::Timestamp::operator<(const Timestamp& rhs) const
{
  Kumu::TAI::caltime lh_ct, rh_ct;
  TIMESTAMP_TO_CALTIME(*this, &lh_ct)
  TIMESTAMP_TO_CALTIME(rhs, &rh_ct)

  Kumu::TAI::tai lh_tai, rh_tai;
  lh_tai = lh_ct;
  rh_tai = rh_ct;

  return ( lh_tai.x < rh_tai.x );
}

//
bool
Kumu::Timestamp::operator>(const Timestamp& rhs) const
{
  Kumu::TAI::caltime lh_ct, rh_ct;
  TIMESTAMP_TO_CALTIME(*this, &lh_ct)
  TIMESTAMP_TO_CALTIME(rhs, &rh_ct)

  Kumu::TAI::tai lh_tai, rh_tai;
  lh_tai = lh_ct;
  rh_tai = rh_ct;

  return ( lh_tai.x > rh_tai.x );
}

//
void
Kumu::Timestamp::AddDays(i32_t days)
{
  Kumu::TAI::caltime ct;
  Kumu::TAI::tai t;

  if ( days != 0 )
    {
      TIMESTAMP_TO_CALTIME(*this, &ct)
      t = ct;
      t.add_days(days);
      ct = t;
      CALTIME_TO_TIMESTAMP(&ct, *this)
    }
}

//
void
Kumu::Timestamp::AddHours(i32_t hours)
{
  Kumu::TAI::caltime ct;
  Kumu::TAI::tai t;

  if ( hours != 0 )
    {
      TIMESTAMP_TO_CALTIME(*this, &ct)
      t = ct;
      t.add_hours(hours);
      ct = t;
      CALTIME_TO_TIMESTAMP(&ct, *this)
    }
}

//
void
Kumu::Timestamp::AddMinutes(i32_t minutes)
{
  Kumu::TAI::caltime ct;
  Kumu::TAI::tai t;

  if ( minutes != 0 )
    {
      TIMESTAMP_TO_CALTIME(*this, &ct)
      t = ct;
      t.add_minutes(minutes);
      ct = t;
      CALTIME_TO_TIMESTAMP(&ct, *this)
    }
}

//
void
Kumu::Timestamp::AddSeconds(i32_t seconds)
{
  Kumu::TAI::caltime ct;
  Kumu::TAI::tai t;

  if ( seconds != 0 )
    {
      TIMESTAMP_TO_CALTIME(*this, &ct)
      t = ct;
      t.add_seconds(seconds);
      ct = t;
      CALTIME_TO_TIMESTAMP(&ct, *this)
    }
}

#endif // KM_WIN32


Kumu::Timestamp::Timestamp(const Timestamp& rhs) : IArchive()
{
  Year   = rhs.Year;
  Month  = rhs.Month;
  Day    = rhs.Day;
  Hour   = rhs.Hour;
  Minute = rhs.Minute;
  Second = rhs.Second;
}

//
Kumu::Timestamp::Timestamp(const char* datestr) : IArchive()
{
  if ( ! DecodeString(datestr) )
    {
      *this = Timestamp();
    }
}

//
Kumu::Timestamp::~Timestamp()
{
}

//
const Kumu::Timestamp&
Kumu::Timestamp::operator=(const Timestamp& rhs)
{
  Year   = rhs.Year;
  Month  = rhs.Month;
  Day    = rhs.Day;
  Hour   = rhs.Hour;
  Minute = rhs.Minute;
  Second = rhs.Second;
  return *this;
}

//
bool
Kumu::Timestamp::operator==(const Timestamp& rhs) const
{
  if ( Year == rhs.Year
       && Month  == rhs.Month
       && Day    == rhs.Day
       && Hour   == rhs.Hour
       && Minute == rhs.Minute
       && Second == rhs.Second )
    return true;

  return false;
}

//
bool
Kumu::Timestamp::operator!=(const Timestamp& rhs) const
{
  if ( Year != rhs.Year
       || Month  != rhs.Month
       || Day    != rhs.Day
       || Hour   != rhs.Hour
       || Minute != rhs.Minute
       || Second != rhs.Second )
    return true;

  return false;
}

// 
const char*
Kumu::Timestamp::EncodeString(char* str_buf, ui32_t buf_len) const
{
  return EncodeStringWithOffset(str_buf, buf_len, 0);
}

// 
const char*
Kumu::Timestamp::EncodeStringWithOffset(char* str_buf, ui32_t buf_len,
					i32_t offset_minutes) const
{
  if ( buf_len < ( DateTimeLen + 1 ) )
    return 0;

  // ensure offset is within +/- 14 hours
  if ((offset_minutes < -14 * 60) || (offset_minutes > 14 * 60))
    return 0;

  // set the apparent time
  Kumu::Timestamp tmp_t(*this);
  tmp_t.AddMinutes(offset_minutes);

  char direction = '+';
  if (offset_minutes < 0) {
    direction = '-';
    // need absolute offset from zero
    offset_minutes = -offset_minutes;
  }

  // 2004-05-01T13:20:00+00:00
  snprintf(str_buf, buf_len,
	   "%04hu-%02hu-%02huT%02hu:%02hu:%02hu%c%02hu:%02hu",
	   tmp_t.Year, tmp_t.Month, tmp_t.Day,
	   tmp_t.Hour, tmp_t.Minute, tmp_t.Second,
	   direction,
	   offset_minutes / 60,
	   offset_minutes % 60);

  return str_buf;
}

//
bool
Kumu::Timestamp::DecodeString(const char* datestr)
{
  Timestamp TmpStamp;

  if ( ! ( isdigit(datestr[0]) && isdigit(datestr[1]) && isdigit(datestr[2]) && isdigit(datestr[3]) )
       || datestr[4] != '-'
       || ! ( isdigit(datestr[5]) && isdigit(datestr[6]) )
       || datestr[7] != '-'
       || ! ( isdigit(datestr[8]) && isdigit(datestr[9]) ) )
    return false;

  ui32_t char_count = 10;
  TmpStamp.Year = atoi(datestr);
  TmpStamp.Month = atoi(datestr + 5);
  TmpStamp.Day = atoi(datestr + 8);
  TmpStamp.Hour = TmpStamp.Minute = TmpStamp.Second = 0;
 
  if ( datestr[10] == 'T' )
    {
      if ( ! ( isdigit(datestr[11]) && isdigit(datestr[12]) )
	   || datestr[13] != ':'
	   || ! ( isdigit(datestr[14]) && isdigit(datestr[15]) ) )
	return false;

      char_count += 6;
      TmpStamp.Hour = atoi(datestr + 11);
      TmpStamp.Minute = atoi(datestr + 14);

      if ( datestr[16] == ':' )
	{
	  if ( ! ( isdigit(datestr[17]) && isdigit(datestr[18]) ) )
	    return false;

	  char_count += 3;
	  TmpStamp.Second = atoi(datestr + 17);
	}

      if ( datestr[19] == '.' )
	{
	  if ( ! ( isdigit(datestr[20]) && isdigit(datestr[21]) && isdigit(datestr[22]) ) )
	    return false;
	  
	  // we don't carry the ms value
	  datestr += 4;
	}

      if ( datestr[19] == '-' || datestr[19] == '+' )
	{
	  if ( ! ( isdigit(datestr[20]) && isdigit(datestr[21]) )
	       || datestr[22] != ':'
	       || ! ( isdigit(datestr[23]) && isdigit(datestr[24]) ) )
	    return false;

	  char_count += 6;

	  ui32_t TZ_hh = atoi(datestr + 20);
	  ui32_t TZ_mm = atoi(datestr + 23);
	  if ((TZ_hh > 14) || (TZ_mm > 59) || ((TZ_hh == 14) && (TZ_mm > 0)))
	    return false;

	  i32_t TZ_offset = 60 * TZ_hh + TZ_mm;
	  if (datestr[19] == '-')
	    TZ_offset = -TZ_offset;
	  /* at this point, TZ_offset reflects the contents of the string */

	  /* a negative offset is behind UTC and so needs to increment to
	   * convert, while a positive offset must do the reverse */
	  TmpStamp.AddMinutes(-TZ_offset);
	}
      else if (datestr[19] == 'Z')
	{
	  /* act as if the offset were +00:00 */
	  char_count++;
	}
    }

  if ( datestr[char_count] != 0 )
    {
      Kumu::DefaultLogSink().Error("Unexpected extra characters in string: %s (%ld)\n",
				   datestr, char_count);
      return false;
    }

#ifdef KM_WIN32
  SYSTEMTIME st;
  FILETIME ft;
  TIMESTAMP_TO_SYSTIME(TmpStamp, &st);
  if ( SystemTimeToFileTime(&st, &ft) == 0 )
    return false;
  SYSTIME_TO_TIMESTAMP(&st, *this);
#else
  Kumu::TAI::tai t;
  Kumu::TAI::caltime ct;
  TIMESTAMP_TO_CALTIME(TmpStamp, &ct);
  t = ct; // back and forth to tai to normalize offset
  ct = t;
  CALTIME_TO_TIMESTAMP(&ct, *this)
#endif

  return true;
}

//
bool
Kumu::Timestamp::HasValue() const
{
  if ( Year || Month || Day || Hour || Minute || Second )
    return true;

  return false;
}

//
bool
Kumu::Timestamp::Unarchive(MemIOReader* Reader)
{
  assert(Reader);
  if ( ! Reader->ReadUi16BE(&Year) ) return false;	
  if ( ! Reader->ReadRaw(&Month, 6) ) return false;
  return true;
}

//
bool
Kumu::Timestamp::Archive(MemIOWriter* Writer) const
{
  assert(Writer);
  if ( ! Writer->WriteUi16BE(Year) ) return false;	
  if ( ! Writer->WriteRaw(&Month, 6) ) return false;
  return true;
}

//
long
Kumu::Timestamp::GetSecondsSinceEpoch(void) const
{
#ifdef KM_WIN32
  SYSTEMTIME timeST;
  TIMESTAMP_TO_SYSTIME(*this, &timeST);
  FILETIME timeFT;
  SystemTimeToFileTime(&timeST, &timeFT);
  ULARGE_INTEGER timeUL;
  timeUL.LowPart = timeFT.dwLowDateTime;
  timeUL.HighPart = timeFT.dwHighDateTime;

  SYSTEMTIME epochST;
  epochST.wYear = 1970;
  epochST.wMonth = 0;
  epochST.wDayOfWeek = 4;
  epochST.wDay = 1;
  epochST.wHour = 0;
  epochST.wMinute = 0;
  epochST.wSecond = 0;
  epochST.wMilliseconds = 0;
  FILETIME epochFT;
  SystemTimeToFileTime(&epochST, &epochFT);
  ULARGE_INTEGER epochUL;
  epochUL.LowPart = epochFT.dwLowDateTime;
  epochUL.HighPart = epochFT.dwHighDateTime;

  return (timeUL.QuadPart - epochUL.QuadPart) / 10000000;
#else
  Kumu::TAI::caltime ct;
  Kumu::TAI::tai t;
  TIMESTAMP_TO_CALTIME(*this, &ct);
  t = ct;

  return (long) (t.x - ui64_C(4611686018427387914));
#endif
}

//------------------------------------------------------------------------------------------

Kumu::MemIOWriter::MemIOWriter(ByteString* Buf)
  : m_p(0), m_capacity(0), m_size(0)
{
  m_p = Buf->Data();
  m_capacity = Buf->Capacity();
  assert(m_p); assert(m_capacity);
}

bool
Kumu::MemIOWriter:: WriteBER(ui64_t i, ui32_t ber_len)
{
  if ( ( m_size + ber_len ) > m_capacity )
    return false;

  if ( ! write_BER(m_p + m_size, i, ber_len) )
    return false;

  m_size += ber_len;
  return true;
}


Kumu::MemIOReader::MemIOReader(const ByteString* Buf)
  : m_p(0), m_capacity(0), m_size(0)
{
  m_p = Buf->RoData();
  m_capacity = Buf->Length();
  assert(m_p); assert(m_capacity);
}

bool
Kumu::MemIOReader::ReadBER(ui64_t* i, ui32_t* ber_len)
{
  if ( i == 0 || ber_len == 0 ) return false;

  if ( ( *ber_len = BER_length(m_p + m_size) ) == 0 )
    return false;

  if ( ( m_size + *ber_len ) > m_capacity )
    return false;

  if ( ! read_BER(m_p + m_size, i) )
    return false;

  m_size += *ber_len;
  return true;
}

//------------------------------------------------------------------------------------------

Kumu::ByteString::ByteString() : m_Data(0), m_Capacity(0), m_Length(0) {}

Kumu::ByteString::ByteString(ui32_t cap) : m_Data(0), m_Capacity(0), m_Length(0)
{
  Capacity(cap);
}

Kumu::ByteString::~ByteString()
{
  if ( m_Data != 0 )
    free(m_Data);
}


// copy the given data into the ByteString, set Length value.
// Returns error if the ByteString is too small.
Kumu::Result_t
Kumu::ByteString::Set(const byte_t* buf, ui32_t buf_len)
{
  if ( m_Capacity < buf_len )
    return RESULT_ALLOC;

  memcpy(m_Data, buf, buf_len);
  m_Length = buf_len;
  return RESULT_OK;
}


// copy the given data into the ByteString, set Length value.
// Returns error if the ByteString is too small.
Kumu::Result_t
Kumu::ByteString::Set(const ByteString& Buf)
{
  if ( m_Capacity < Buf.m_Capacity )
    return RESULT_ALLOC;

  memcpy(m_Data, Buf.m_Data, Buf.m_Length);
  m_Length = Buf.m_Length;
  return RESULT_OK;
}


// Sets the size of the internally allocate buffer.
Kumu::Result_t
Kumu::ByteString::Capacity(ui32_t cap_size)
{
  if ( m_Capacity >= cap_size )
    return RESULT_OK;

  byte_t* tmp_data = 0;
  if ( m_Data != 0 )
    {
      if ( m_Length > 0 )
	tmp_data = m_Data;
      else
	free(m_Data);
    }
		
  if ( ( m_Data = (byte_t*)malloc(cap_size) ) == 0 )
    return RESULT_ALLOC;

  if ( tmp_data != 0 )
    {
      assert(m_Length > 0);
      memcpy(m_Data, tmp_data, m_Length);
      free(tmp_data);
    }
		
  m_Capacity = cap_size;
  return RESULT_OK;
}

//
Kumu::Result_t
Kumu::ByteString::Append(const ByteString& Buf)
{
  Result_t result = RESULT_OK;
  ui32_t diff = m_Capacity - m_Length;

  if ( diff < Buf.Length() )
    result = Capacity(m_Capacity + Buf.Length());

  if ( KM_SUCCESS(result) )
    {
      memcpy(m_Data + m_Length, Buf.RoData(), Buf.Length());
      m_Length += Buf.Length();
    }

  return result;
}

//
Kumu::Result_t
Kumu::ByteString::Append(const byte_t* buf, ui32_t buf_len)
{
  Result_t result = RESULT_OK;
  ui32_t diff = m_Capacity - m_Length;

  if ( diff < buf_len )
    result = Capacity(m_Capacity + buf_len);

  if ( KM_SUCCESS(result) )
    {
      memcpy(m_Data + m_Length, buf, buf_len);
      m_Length += buf_len;
    }

  return result;
}


//
// end KM_util.cpp
//
