#ifndef VEE_COMMON_H
#define VEE_COMMON_H

#include <glib.h>

static inline gchar* vee_str_trim(gchar *str, const gchar *chars_to_trim) {
    if (!str) return NULL;

    // 修剪开头
    while (*str && strchr(chars_to_trim, *str)) {
        str++;
    }

    // 修剪结尾
    gchar *end = str + strlen(str) - 1;
    while (end >= str && strchr(chars_to_trim, *end)) {
        *end = '\0';
        end--;
    }

    return str;
}

#endif
