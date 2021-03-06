//this Ruby differs from 1.8.7 in that methods returning Arrays of
//identifiers return them as Symbols instead of Strings
//(for improved efficiency and compatibility with Ruby 1.9 and newer)

#define RUBY_VERSION "1.8.9"
#define RUBY_RELEASE_DATE "2020-6-1"
#define RUBY_VERSION_CODE 189
#define RUBY_RELEASE_CODE 20200601
#define RUBY_PATCHLEVEL 352

#define RUBY_VERSION_MAJOR 1
#define RUBY_VERSION_MINOR 8
#define RUBY_VERSION_TEENY 9
#define RUBY_RELEASE_YEAR 2020
#define RUBY_RELEASE_MONTH 6
#define RUBY_RELEASE_DAY 1

#ifdef RUBY_EXTERN
RUBY_EXTERN const char ruby_version[];
RUBY_EXTERN const char ruby_release_date[];
RUBY_EXTERN const char ruby_platform[];
RUBY_EXTERN const int ruby_patchlevel;
RUBY_EXTERN const char *ruby_description;
RUBY_EXTERN const char *ruby_copyright;
#endif

#define RUBY_AUTHOR "Yukihiro Matsumoto"
#define RUBY_BIRTH_YEAR 1993
#define RUBY_BIRTH_MONTH 2
#define RUBY_BIRTH_DAY 24

#include "rubysig.h"

#define string_arg(s) #s

#ifdef MBARI_API
#define _mbari_rev_ "MBARI"
#else
#define _mbari_rev_ "mbari"
#endif

#define MBARI_RELEASE(wipe_sites) _mbari_rev_ "8esp8/" string_arg(wipe_sites)

#define RUBY_RELEASE_STR MBARI_RELEASE(STACK_WIPE_SITES) " on patchlevel"
#define RUBY_RELEASE_NUM RUBY_PATCHLEVEL
