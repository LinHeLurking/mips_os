#include "string.h"

int strlen(char *src)
{
	int i;
	for (i = 0; src[i] != '\0'; i++)
	{
	}
	return i;
}

void memcpy(uint8_t *dest, uint8_t *src, uint32_t len)
{
	for (int i = 0; i < len; ++i)
	{
		dest[i] = src[i];
	}
}

void memset(void *dest, uint8_t val, uint32_t len)
{
	uint8_t *dst = (uint8_t *)dest;

	for (int i = 0; i < len; ++i)
	{
		dst[i] = val;
	}
}

void bzero(void *dest, uint32_t len)
{
	memset(dest, 0, len);
}

int strcmp(char *str1, char *str2)
{
	while (*str1 && *str2 && (*str1 == *str2))
	{
		str1++;
		str2++;
	};

	return *str1 - *str2;
}

char *strcpy(char *dest, char *src)
{
	char *tmp = dest;

	while (*src)
	{
		*dest++ = *src++;
	}

	*dest = '\0';

	return tmp;
}

int atoi(char *s)
{
	int sign = 1;
	int i;
	int abs = 0;
	int base = 10;
	while (*s == ' ')
		++s;
	if (*s == '-')
	{
		++s;
		sign = -1;
	}
	if (s[0] == '0' && s[1] == 'x')
	{
		base = 16;
		s += 2;
	}
	else if (s[0] == '0' && s[1] == 'b')
	{
		base = 2;
		s += 2;
	}
	for (i = 0; s[i]; ++i)
	{
		if (s[i] == ' ')
		{
			continue;
		}
		abs = abs * base + s[i] - '0';
	}
	return sign * abs;
}
