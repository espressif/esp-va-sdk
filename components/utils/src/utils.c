#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
int ends_with(const char * haystack, const char *needle)
{
    const char * end;
    int nlen = strlen(needle);
    int hlen = strlen(haystack);

    if( nlen > hlen )
        return 0;
    end = haystack + hlen - nlen;

    return (strcasecmp(end, needle) ? 0 : 1);
}

char *trim(char *str)
{
    int i;
    int len;

    if (!str)
        return(NULL);

    len = strlen(str);
    for (i=len-1; i >= 0 && isspace((unsigned char)str[i]); i--)
    {
        str[i] = '\0';
        len--;
    }
    while (isspace((unsigned char) *str))
    {
        str++;
        len--;
    }

    if (str[0] == '"' && str[len-1] == '"')
    {
        str[0] = '\0';
        str[len-1] = '\0';
        str++;
    }

    return str;
}

/* Find the first occurrence of p in s, where s is terminated by t */
char *strstrc(const char *s, const char *p, const char t)
{
    char *endptr;
    size_t slen, plen;

    endptr = strchr(s, t);
    if (!endptr)
        return strstr(s, p);

    plen = strlen(p);
    slen = endptr - s;
    while (slen >= plen)
    {
        if (*s == *p && strncmp(s+1, p+1, plen-1) == 0)
            return (char*)s;
        s++;
        slen--;
    }

    return NULL;
}

static char *strcasestr(const char *s, const char *find)
{

  /** Less code size, but quadratic performance in the worst case.  */
    char c, sc;
    size_t len;

    if ((c = *find++) != 0) {
        c = tolower((unsigned char)c);
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == 0)
                    return (NULL);
            } while ((char)tolower((unsigned char)sc) != c);
        } while (strncasecmp(s, find, len) != 0);
        s--;
    }
    return ((char *)s);
}
char *strcasestrc(const char *s, const char *p, const char t)
{
    char *endptr;
    size_t slen, plen;

    endptr = strchr(s, t);
    if (!endptr)
        return strcasestr(s, p);

    plen = strlen(p);
    slen = endptr - s;
    while (slen >= plen)
    {
        if (*s == *p && strncasecmp(s+1, p+1, plen-1) == 0)
            return (char*)s;
        s++;
        slen--;
    }

    return NULL;
}

char *modify_string(char * string, const char * before, const char * after, short like)
{
    int oldlen, newlen, chgcnt = 0;
    char *s, *p, *t;

    oldlen = strlen(before);
    newlen = strlen(after);
    if( newlen+like > oldlen )
    {
        s = string;
        while( (p = strstr(s, before)) )
        {
            chgcnt++;
            s = p+oldlen;
        }
        s = realloc(string, strlen(string)+((newlen-oldlen)*chgcnt)+1+like);
        /* If we failed to realloc, return the original alloc'd string */
        if( s )
            string = s;
        else
            return string;
    }

    s = string;
    while( s )
    {
        p = strcasestr(s, before);
        if( !p )
            return string;
        memmove(p + newlen, p + oldlen, strlen(p + oldlen) + 1);
        memcpy(p, after, newlen);
        if( like )
        {
            t = p+newlen;
            while(isspace((unsigned char)*t) )
                t++;
            if( *t == '"' )
            {
                if( like == 2 )
                {
                    memmove(t+2, t+1, strlen(t+1)+1);
                    *++t = '%';
                }
                while( *++t != '"' )
                    continue;
                memmove(t+1, t, strlen(t)+1);
                *t = '%';
            }
        }
        s = p + newlen;
    }

    return string;
}

char *escape_tag(const char *tag, int force_alloc)
{
    char *esc_tag = NULL;

    if( strchr(tag, '&') || strchr(tag, '<') || strchr(tag, '>') || strchr(tag, '"') )
    {
        esc_tag = strdup(tag);
        esc_tag = modify_string(esc_tag, "&", "&amp;", 0);
        esc_tag = modify_string(esc_tag, "<", "&lt;", 0);
        esc_tag = modify_string(esc_tag, ">", "&gt;", 0);
        // esc_tag = modify_string(esc_tag, "\"", "&amp;quot;", 0);
    }
    else if( force_alloc )
        esc_tag = strdup(tag);

    return esc_tag;
}

void strip_ext(char * name)
{
    char * period;

    period = strrchr(name, '.');
    if( period )
        *period = '\0';
}

int is_video(const char * file)
{
    return (ends_with(file, ".mpg") || ends_with(file, ".mpeg")  ||
        ends_with(file, ".avi") || ends_with(file, ".divx")  ||
        ends_with(file, ".asf") || ends_with(file, ".wmv")   ||
        ends_with(file, ".mp4") || ends_with(file, ".m4v")   ||
        ends_with(file, ".mts") || ends_with(file, ".m2ts")  ||
        ends_with(file, ".m2t") || ends_with(file, ".mkv")   ||
        ends_with(file, ".vob") || ends_with(file, ".ts")    ||
        ends_with(file, ".flv") || ends_with(file, ".xvid")  ||
#ifdef TIVO_SUPPORT
        ends_with(file, ".TiVo") ||
#endif
        ends_with(file, ".mov") || ends_with(file, ".3gp"));
}

int is_audio(const char * file)
{
    return (ends_with(file, ".mp3") || ends_with(file, ".flac") ||
        ends_with(file, ".wma") || ends_with(file, ".asf")  ||
        ends_with(file, ".fla") || ends_with(file, ".flc")  ||
        ends_with(file, ".m4a") || ends_with(file, ".aac")  ||
        ends_with(file, ".mp4") || ends_with(file, ".m4p")  ||
        ends_with(file, ".wav") || ends_with(file, ".ogg")  ||
        ends_with(file, ".pcm") || ends_with(file, ".3gp"));
}

int is_image(const char * file)
{
    return (ends_with(file, ".jpg") || ends_with(file, ".jpeg"));
}

int is_playlist(const char * file)
{
    return (ends_with(file, ".m3u") || ends_with(file, ".pls"));
}

char *get_xml_tag_value(char *buffer, const char *tag)
{

    int tag_len = strlen(tag);
    char *open_tag = malloc(tag_len + 3);
    char *close_tag = malloc(tag_len + 4);
    char *found_tag = NULL;
    configASSERT(open_tag);
    configASSERT(close_tag);
    sprintf(open_tag, "<%s>", tag);
    sprintf(close_tag, "</%s>", tag);

    char *found_open = strcasestr(buffer, open_tag);
    char *found_close = strcasestr(buffer, close_tag);
    if(found_open && found_close) {
        found_tag = found_open + tag_len + 2;
        found_close[0] = 0;//terminal string
    }
    free(open_tag);
    free(close_tag);

    return found_tag;
}
