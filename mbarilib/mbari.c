/**********************************************************************

  mbari.c -

  Copyright (C) 1993-2000 Yukihiro Matsumoto
  Copyright (C) 2000  Network Applied Communication Laboratory, Inc.
  Copyright (C) 2000  Information-technology Promotion Agency, Japan

  Revised:  9/11/23 brent@mbari.org
    added optimized network byte order conversions

  Revised:  9/5/23 brent@mbari.org
    added GatewayPort module

  Revised:  9/1/04 brent@mbari.org
    this just adds the kernel.doze method that clears thread.critical
    atomically just before sleeping

**********************************************************************/

#include "ruby.h"
#include "rubysig.h"
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifndef NT
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#else
struct timeval {
        long    tv_sec;         /* seconds */
        long    tv_usec;        /* and microseconds */
};
#endif
#endif /* NT */
#include <ctype.h>

struct timeval rb_time_interval _((VALUE));

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_GETPRIORITY
# include <sys/resource.h>
#endif
#include "st.h"

#ifdef __EMX__
#undef HAVE_GETPGRP
#endif


extern int rb_thread_critical;  /* Thread.critical from eval.c */

static VALUE
rb_doze(argc, argv)
    int argc;
    VALUE *argv;
{
    int beg, end;

    beg = time(0);
    rb_thread_critical = 0;  /* like Thread.stop, exit critical section */
    if (argc == 0)
        rb_thread_sleep_forever();
    else if (argc == 1)
        rb_thread_wait_for(rb_time_interval(argv[0]));
    else
        rb_raise(rb_eArgError, "wrong # of arguments");

    end = time(0) - beg;

    return INT2FIX(end);
}


static VALUE rb_mPort;
static ID framingError;

#define zero INT2FIX(0)
#define ff INT2FIX(0xff)

static unsigned long num2ulong(VALUE u)
{
    unsigned long result = NUM2ULONG(u);
    if (rb_funcall(u, '<', 1, INT2FIX(0)))
        rb_raise(rb_eRangeError, "unsigned value %ld < 0", result);
    return result;
}

static VALUE
getcPort(io)
    VALUE io;
{
    VALUE rsp = rb_io_getc(io);
    if (rsp == ff)
      switch(rsp=rb_io_getc(io)) {
        case ff:
         break;
        case zero:
         rsp = rb_funcall(io, framingError, 1, rb_io_getc(io));
        default:
         rsp = rb_funcall(io, framingError, 1, Qnil);
      }
    return rsp;
}

static VALUE
getBlockPort(argc, argv, io)
    int argc;
    VALUE *argv;
    VALUE io;
{
    VALUE len, result;
    char *cursor, *end;
    size_t count;
    int args = rb_scan_args(argc, argv, "11", &len, &result);
    count = num2ulong(len);
    if (args > 1) {
        size_t prefixLen;
        StringValue(result);
        prefixLen = RSTRING(result)->len;
        rb_str_resize(result, prefixLen + count);
        cursor = RSTRING(result)->ptr + prefixLen;
    }else{
        result = rb_str_new(NULL, count);
        cursor = RSTRING(result)->ptr;
    }
    end = cursor + count;
    while(cursor < end) {
        *cursor++ = FIX2LONG(getcPort(io));
}
    return result;
}


static VALUE escape(VALUE str, VALUE specials)
/*
 * In str, prefix each occurence of any character in specials with a its 1st
 * If str is unchanged, returns str, else returns a new String
 */
{
  char *s = RSTRING(str)->ptr;
  char *send = s + RSTRING(str)->len;
  StringValue(specials);
  if (RSTRING(specials)->len) for (; s < send; s++) {
    if (memchr(RSTRING(specials)->ptr, *s, RSTRING(specials)->len)) {
      size_t unEscLen = s - RSTRING(str)->ptr;
      VALUE tmp = rb_str_new(0, RSTRING(str)->len*2 - unEscLen);
      char *t = RSTRING(tmp)->ptr;
      /* copy upto metacharacter */
      memcpy(t, RSTRING(str)->ptr, unEscLen);
      t += unEscLen;
      *t++ = RSTRING(specials)->ptr[0];  //insert 1st escape char
      *t++ = *s++;

      for (; s < send; s++) {
        int c = *s;
	    if (memchr(RSTRING(specials)->ptr, c, RSTRING(specials)->len))
          *t++ = RSTRING(specials)->ptr[0];
        *t++ = c;
      }
      rb_str_resize(tmp, t - RSTRING(tmp)->ptr);
      OBJ_INFECT(tmp, str);
      return tmp;
    }
  }
  return str;
}


static VALUE unescape(VALUE str, VALUE prefix)
/*
 * In str, remove all escapes previously inserted by the escape method
 * prefix should be Fixnum char code (usually specials.ord from the escape call)
 * returns str
 */
{
  unsigned meta = NUM2UINT(prefix);
  unsigned char *s = RSTRING(str)->ptr;
  unsigned char *d = s;
  unsigned char *send = s + RSTRING(str)->len;
  for (; s < send; s++) {
    if (*s == meta) {
      s++;
      if (s == send)
        rb_raise(rb_eArgError, "escaped String ends with '%c'", meta);
    }
    *d++ = *s;
  }
  if (d < s)
    rb_str_resize(str, (char *)d - RSTRING(str)->ptr);
  return str;
}


static unsigned char *checkIndex(VALUE s, VALUE idx, size_t len)
{
    long i = NUM2LONG(idx);
    if (i < 0)
        i += RSTRING(s)->len;
    if (i < 0 || i+len > RSTRING(s)->len)
      rb_raise(rb_eRangeError, "index outside String");
    return RSTRING(s)->ptr+i;
}

/* return unsigned integer representation of 1 byte string
*/
static VALUE asCardinal1(VALUE s, VALUE idx)
{
    unsigned char *byte = checkIndex(s,idx,1);
    return ULONG2NUM(byte[0]);
}

/* return signed integer representation of 1 byte string
*/
static VALUE asInteger1(VALUE s, VALUE idx)
{
    unsigned char *byte = checkIndex(s,idx,1);
    return LONG2FIX((signed char)byte[0]);
}

/* return unsigned integer representation of 2 byte string
*  most significant byte first
*/
static VALUE asCardinal2(VALUE s, VALUE idx)
{
    unsigned char *byte = checkIndex(s,idx,2);
    return ULONG2NUM(byte[0]<<8 | byte[1]);
}

/* return signed integer representation of 2 byte string
*  most significant byte first
*/
static VALUE asInteger2(VALUE s, VALUE idx)
{
    unsigned char *byte = checkIndex(s,idx,2);
    return LONG2FIX(((signed char)(byte[0])<<8) | byte[1]);
}

/* return unsigned integer representation of 3 byte string
*  most significant byte first
*/
static VALUE asCardinal3(VALUE s, VALUE idx)
{
    unsigned char *byte = checkIndex(s,idx,3);
    return ULONG2NUM(byte[0]<<16 | byte[1]<<8 | byte[2]);
}

/* return signed integer representation of 3 byte string
*  most significant byte first
*/
static VALUE asInteger3(VALUE s, VALUE idx)
{
    unsigned char *byte = checkIndex(s,idx,3);
    return LONG2FIX(((signed char)(byte[0])<<16) | byte[1]<<8 | byte[2]);
}

/* return unsigned integer representation of 4 byte string
*  most significant byte first
*/
static VALUE asCardinal4(VALUE s, VALUE idx)
{
    unsigned char *byte = checkIndex(s,idx,4);
    return ULONG2NUM(byte[0]<<24 | byte[1]<<16 | byte[2]<<8 | byte[3] );
}

/* return signed integer representation of 4 byte string
*  most significant byte first
*/
static VALUE asInteger4(VALUE s, VALUE idx)
{
    unsigned char *byte = checkIndex(s,idx,4);
    return LONG2NUM(((signed char)(byte[0])<<24) |
                        byte[1]<<16 | byte[2]<<8 | byte[3] );
}


static unsigned char *growIndex(VALUE s, VALUE idx, size_t len)
{
    long i = NUM2LONG(idx);
    StringValue(s);
    if (i < 0)
        i += RSTRING(s)->len;
    if (i < 0)
        rb_raise(rb_eRangeError, "index before start of String");
    if (i+len > RSTRING(s)->len)
        rb_str_resize(s, i+len);
    else
        rb_str_modify(s);
    return RSTRING(s)->ptr+i;
}

static void unsignedOutOfRange(unsigned bits, unsigned long val)
{
    rb_raise(rb_eRangeError,
        "%lu won't fit in unsigned %u bit field", val, bits);
}

static void signedOutOfRange(unsigned bits, unsigned long val)
{
    rb_raise(rb_eRangeError,
        "%ld won't fit in signed %u bit field", (long)val, bits);
}

/* return s modified such that s[idx,1] contains unsigned byte u
*/
static VALUE asUnsigned1(VALUE u, VALUE s, VALUE idx)
{
    unsigned long ul = num2ulong(u);
    if (ul <= 0xff) {
      *growIndex(s,idx,1) = ul;
      return s;
    }
    unsignedOutOfRange(8, ul);
}

/* return s modified such that s[idx,1] contains signed byte i
*/
static VALUE asSigned1(VALUE i, VALUE s, VALUE idx)
{
    unsigned long l = NUM2LONG(i);
    if (l+0x80 <= 0xff) {
      *growIndex(s,idx,1) = l;
      return s;
    }
    signedOutOfRange(8, l);
}

/* return s modified such that s[idx,2] contains unsigned short word u
*  most significant byte first
*/
static VALUE asUnsigned2(VALUE u, VALUE s, VALUE idx)
{
    unsigned long ul = num2ulong(u);
    if (ul <= 0xffff) {
      unsigned char *byte = growIndex(s,idx,2);
      byte[0]=ul>>8; byte[1]=ul;
      return s;
    }
    unsignedOutOfRange(16, ul);
}

/* return s modified such that s[idx,2] contains signed short word i
*  most significant byte first
*/
static VALUE asSigned2(VALUE i, VALUE s, VALUE idx)
{
    unsigned long l = NUM2LONG(i);
    if (l+0x8000 <= 0xffff) {
      unsigned char *byte = growIndex(s,idx,2);
      byte[0]=l>>8; byte[1]=l;
      return s;
    }
    signedOutOfRange(16, l);
 }

/* return s modified such that s[idx,3] contains unsigned 3-bytes u
*  most significant byte first
*/
static VALUE asUnsigned3(VALUE u, VALUE s, VALUE idx)
{
    unsigned long ul = num2ulong(u);
    if (ul <= 0xffffff) {
      unsigned char *byte = growIndex(s,idx,3);
      byte[2]=ul; byte[1]=ul>>8; byte[0]=ul>>16;
      return s;
    }
    unsignedOutOfRange(24, ul);
}

/* return s modified such that s[idx,3] contains signed 3-bytes u
*  most significant byte first
*/
static VALUE asSigned3(VALUE i, VALUE s, VALUE idx)
{
    unsigned long l = NUM2LONG(i);
    if (l+0x800000 <= 0xffffff) {
      unsigned char *byte = growIndex(s,idx,3);
      byte[2]=l; byte[1]=l>>8; byte[0]=l>>16;
      return s;
    }
    signedOutOfRange(24, l);
}

/* return s modified such that s[idx,4] contains unsigned word u
*  most significant byte first
*/
static VALUE asUnsigned4(VALUE u, VALUE s, VALUE idx)
{
    unsigned long ul = num2ulong(u);
    if (ul <= 0xffffffff) {
      unsigned char *byte = growIndex(s,idx,4);
      byte[3]=ul; byte[2]=ul>>8; byte[1]=ul>>16; byte[0]=ul>>24;
      return s;
    }
    unsignedOutOfRange(32, ul);
}

/* return s modified such that s[idx,4] contains signed word i
*  most significant byte first
*/
static VALUE asSigned4(VALUE i, VALUE s, VALUE idx)
{
    unsigned long l = NUM2LONG(i);
    if (l+0x80000000 <= 0xffffffff) {
      unsigned char *byte = growIndex(s,idx,4);
      byte[3]=l; byte[2]=l>>8; byte[1]=l>>16; byte[0]=l>>24;
      return s;
    }
    signedOutOfRange(32, l);
}

void
Init_mbarilib()
{
    rb_define_global_function("doze", rb_doze, -1);

    rb_mPort = rb_define_module("GatewayPort");
    framingError = rb_intern("framingError");
    rb_define_method(rb_mPort, "getc", getcPort, 0);
    rb_define_method(rb_mPort, "getBlock", getBlockPort, -1);

    rb_define_method(rb_cString, "escape", escape, 1);
    rb_define_method(rb_cString, "unescape", unescape, 1);

    rb_define_method(rb_cString, "asCardinal1", asCardinal1, 1);
    rb_define_method(rb_cString, "asCardinal2", asCardinal2, 1);
    rb_define_method(rb_cString, "asCardinal3", asCardinal3, 1);
    rb_define_method(rb_cString, "asCardinal4", asCardinal4, 1);
    rb_define_method(rb_cString, "asInteger1", asInteger1, 1);
    rb_define_method(rb_cString, "asInteger2", asInteger2, 1);
    rb_define_method(rb_cString, "asInteger3", asInteger3, 1);
    rb_define_method(rb_cString, "asInteger4", asInteger4, 1);

    rb_define_method(rb_cInteger, "asUnsigned1", asUnsigned1, 2);
    rb_define_method(rb_cInteger, "asUnsigned2", asUnsigned2, 2);
    rb_define_method(rb_cInteger, "asUnsigned3", asUnsigned3, 2);
    rb_define_method(rb_cInteger, "asUnsigned4", asUnsigned4, 2);
    rb_define_method(rb_cInteger, "asSigned1", asSigned1, 2);
    rb_define_method(rb_cInteger, "asSigned2", asSigned2, 2);
    rb_define_method(rb_cInteger, "asSigned3", asSigned3, 2);
    rb_define_method(rb_cInteger, "asSigned4", asSigned4, 2);
 }

