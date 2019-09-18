#ifndef _UTILS_H_
#define _UTILS_H_
char *strcasestrc(const char *s, const char *p, const char t);
int ends_with(const char * haystack, const char * needle);
char *get_xml_tag_value(char *buffer, const char *tag);
char *escape_tag(const char *tag, int force_alloc);
char *trim(char *str);
#endif
