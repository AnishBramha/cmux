#include "string_view.h"


string_view sv_from_c_str(char* c_str) {

    return (string_view) {

        .str = c_str,
        .len = strlen(c_str),
    };
}


void sv_chop_nleft(string_view* sv, size_t n) {

    n = (n > sv->len) ? sv->len : n;
    sv->len -= n;
    sv->str += n;
}

#define sv_chop_left(sv) sv_chop_nleft(sv, 1)


void sv_chop_nright(string_view* sv, size_t n) {

    n = (n > sv->len) ? sv->len : n;
    sv->len -= n;
}


#define sv_chop_right(sv) sv_chop_nright(sv, 1)


void sv_chop_n(string_view* sv, size_t n) {

    sv_chop_nleft(sv, n);
    sv_chop_nright(sv, n);
}

#define sv_chop(sv) sv_chop_n(sv, 1)


void sv_trim_left(string_view* sv) {

    while (sv->len > 0 && isspace(sv->str[0]))
        sv_chop_left(sv);
}


void sv_trim_right(string_view* sv) {

    while (sv->len > 0 && isspace(sv->str[sv->len - 1]))
        sv_chop_right(sv);
}


void sv_trim(string_view* sv) {

    sv_trim_left(sv);
    sv_trim_right(sv);
}


string_view sv_split_by_delim(string_view* sv, char delim) {

    size_t i = 0;
    while (i < sv->len && sv->str[i] != delim)
        i++;

    if (i < sv->len) {

        string_view token = {
            
            .str = sv->str,
            .len = i,
        };

        sv_chop_nleft(sv, i + 1);
        return token;
    }

    string_view token = *sv;
    sv_chop_nleft(sv, sv->len);

    return token;
}


string_view sv_split_by_type(string_view* sv, int (*istype)(int c)) {


    size_t i = 0;
    while (i < sv->len && !istype(sv->str[i]))
        i++;

    if (i < sv->len) {

        string_view token = {
            
            .str = sv->str,
            .len = i,
        };

        sv_chop_nleft(sv, i + 1);
        return token;
    }

    string_view token = *sv;
    sv_chop_nleft(sv, sv->len);

    return token;
}




