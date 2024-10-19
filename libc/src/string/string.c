// SPDX-License-Identifier: BSD-3-Clause
#include <stdlib.h>
#include <string.h>

char *strcpy(char *destination, const char *source)
{
	/* TODO: Implement strcpy(). */
    char *dst = destination;
    for ( ; source[0] != '\0' ; source++, dst++ ) {
        dst[0] = source[0];
    }
    dst[0] = '\0';
	return destination;
}

char *strncpy(char *destination, const char *source, size_t len)
{
	/* TODO: Implement strncpy(). */
    char *dst = destination;
    for ( int i = 0 ; i < (int) len ; i++, source++, dst++ ) {
        dst[0] = source[0];
    }
	return destination;
}

char *strcat(char *destination, const char *source)
{
	/* TODO: Implement strcat(). */
    char *dst = destination;
    for ( ; dst[0] != '\0' ; dst++ ) {}
    for ( ; source[0] != '\0' ; dst++, source++) {
        dst[0] = source[0];
    }
    dst[0] = '\0';
	return destination;
}

char *strncat(char *destination, const char *source, size_t len)
{
	/* TODO: Implement strncat(). */
    char *dst = destination;
    for ( ; dst[0] != '\0' ; dst++ ) {}
    for ( int i = 0 ; i < (int)len ; dst++, source++, i++ ) {
        dst[0] = source[0];
    }
    dst[0] = '\0';
	return destination;
}

int strcmp(const char *str1, const char *str2)
{
	/* TODO: Implement strcmp(). */
    for ( ; str1[0] != '\0' && str2[0] != '\0' ; str1++, str2++ ) {
        if ( str1[0] != str2[0] )
            return str1[0] - str2[0];
    }
	return str1[0] - str2[0];
}

int strncmp(const char *str1, const char *str2, size_t len)
{
	/* TODO: Implement strncmp(). */
	for ( int i = 0 ; i < (int)len ; str1++, str2++, i++ ) {
        if ( str1[0] != str2[0] )
            return str1[0] - str2[0];
    }
	return 0;
}

size_t strlen(const char *str)
{
	size_t i = 0;

	for (; *str != '\0'; str++, i++)
		;

	return i;
}

char *strchr(const char *str, int c)
{
	/* TODO: Implement strchr(). */
    for ( ; str[0] != '\0' ; str++ ) {
        if ( str[0] == c )
            return (char*)str;
    }
	return NULL;
}

char *strrchr(const char *str, int c)
{
	/* TODO: Implement strrchr(). */
    char *start = (char*)str;
    for ( ; str[0] != '\0' ; str++ ) {}
    for ( ; str != start ; str-- ) {
        if ( str[0] == c )
            return (char*)str;
    }
    // Start of string isnt covered by for
    if ( str[0] == c )
            return (char*)str;
	return NULL;
}

char *strstr(const char *haystack, const char *needle)
{
	/* TODO: Implement strstr(). */
    int l = strlen(needle);
    char *solution = (char*)haystack;
    for ( int s = 0 ; haystack[0] != '\0' ; haystack++ ) {
        if ( haystack[0] == needle[s] ) {
            if ( s == 0 )
                solution = (char*)haystack;
            s++;
            if ( s == l )
                return (char*)solution;
        } else {
            s = 0;
        }
    }
	return NULL;
}


char *strrstr(const char *haystack, const char *needle)
{
	/* TODO: Implement strrstr(). */
      int l = strlen(needle);
    char *solutionFinal = NULL;
    char *solution = NULL;
    for ( int s = 0 ; haystack[0] != '\0' ; haystack++ ) {
        if ( haystack[0] == needle[s] ) {
            if ( s == 0 )
                solution = (char*)haystack;
            s++;
            if ( s == l )
                solutionFinal = (char*)solution;
        } else {
            s = 0;
        }
    }
	return solutionFinal;
}

void *memcpy(void *destination, const void *source, size_t num)
{
	/* TODO: Implement memcpy(). */
    char *dst = (char*)destination;
    char *src = (char*)source;
    for ( int i = 0 ; i < (int)num ; i++, dst++, src++ ) {
        dst[0] = src[0];
    }
	return destination;
}

void *memmove(void *destination, const void *source, size_t num)
{
	/* TODO: Implement memmove(). */
    char *src = (char*)source;
    char *buf = malloc(num + 1);
    for ( int i = 0 ; i < (int)num ; i++, src++ ) {
        buf[i] = src[0];
    }
    char *dst = (char*)destination;
    for ( int i = 0 ; i < (int)num ; i++, dst++ ) {
        dst[0] = buf[i];
    }
    free(buf);
	return destination;
}

int memcmp(const void *ptr1, const void *ptr2, size_t num)
{
	/* TODO: Implement memcmp(). */
    unsigned char *s1 = (unsigned char*)ptr1;
    unsigned char *s2 = (unsigned char*)ptr2;
    for ( int i = 0 ; i < (int)num ; i++, s1++, s2++ ) {
        if ( s1[0] != s2[0] )
            return s1[0] - s2[0];
    }
	return 0;
}

void *memset(void *source, int value, size_t num)
{
	/* TODO: Implement memset(). */
    char *src = source;
    char setter = value;
    for ( int i = 0 ; i < (int)num ; i++, src++ ) {
        src[0] = setter;
    }
	return source;
}
