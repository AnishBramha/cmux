#pragma once

#include "common.h"


typedef struct {

    const char* str;
    size_t len;

} string_view;


string_view sv_from_c_str(const char* c_str);
void sv_chop_nleft(string_view* sv, size_t n);
void sv_chop_nright(string_view* sv, size_t n);
void sv_chop_n(string_view* sv, size_t n);
void sv_trim_left(string_view* sv);
void sv_trim_right(string_view* sv);
void sv_trim(string_view* sv);
string_view sv_split_by_delim(string_view* sv, char delim);
string_view sv_split_by_type(string_view* sv, int (*istype)(int c));

#define sv_to_c_str(sv) strndup(sv.str, sv.len)

#define sv_chop_left(sv) sv_chop_nleft(sv, 1)
#define sv_chop_right(sv) sv_chop_nright(sv, 1)
#define sv_chop(sv) sv_chop_n(sv, 1)

#define sv_fmt "%.*s"
#define sv_arg(sv) (int)(sv).len, (s).str





