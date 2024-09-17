//-----------------------------------------------------------------------------
// Finds a string in another string with a case insensitive test w/ length validation
//-----------------------------------------------------------------------------
char const* V_strnistr( char const* pStr, char const* pSearch, int n )
{
	if (!pStr || !pSearch) 
		return 0;

	char const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		if ( n <= 0 )
			return 0;

		// Skip over non-matches
		if (tolower(*pLetter) == tolower(*pSearch))
		{
			int n1 = n - 1;

			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				if ( n1 <= 0 )
					return 0;

				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (tolower(*pMatch) != tolower(*pTest))
					break;

				++pMatch;
				++pTest;
				--n1;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
		--n;
	}

	return 0;
}

int V_snprintf(char* pDest, int maxLen, char const* pFormat, ...)
{
	va_list marker;

	va_start(marker, pFormat);
	int len = _vsnprintf(pDest, maxLen, pFormat, marker);
	va_end(marker);

	// Len < 0 represents an overflow
	if (len < 0)
	{
		len = maxLen;
		pDest[maxLen - 1] = 0;
	}

	return len;
}

int V_vsnprintf(char* pDest, int maxLen, char const* pFormat, va_list params)
{
	int len = _vsnprintf(pDest, maxLen, pFormat, params);

	if (len < 0)
	{
		len = maxLen;
		pDest[maxLen - 1] = 0;
	}

	return len;
}

int V_strncmp(const char* s1, const char* s2, int count)
{
	while (count-- > 0)
	{
		if (*s1 != *s2)
			return *s1 < *s2 ? -1 : 1; // string different
		if (*s1 == '\0')
			return 0; // null terminator hit - strings the same
		s1++;
		s2++;
	}

	return 0; // count characters compared the same
}

const char* V_strnchr( const char* pStr, char c, int n )
{
	char const* pLetter = pStr;
	char const* pLast = pStr + n;

	// Check the entire string
	while ( (pLetter < pLast) && (*pLetter != 0) )
	{
		if (*pLetter == c)
			return pLetter;
		++pLetter;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *in - 
//			inputbytes - 
//			*out - 
//			outsize - 
//-----------------------------------------------------------------------------
void V_binarytohex(const byte* in, int inputbytes, char* out, int outsize)
{
	char doublet[10];
	int i;

	out[0] = 0;

	for (i = 0; i < inputbytes; i++)
	{
		unsigned char c = in[i];
		snprintf(doublet, sizeof(doublet), "%02x", c);
		V_strncat(out, doublet, outsize, COPY_ALL_CHARACTERS);
	}
}

//-----------------------------------------------------------------------------
// Purpose: If COPY_ALL_CHARACTERS == max_chars_to_copy then we try to add the whole pSrc to the end of pDest, otherwise
//  we copy only as many characters as are specified in max_chars_to_copy (or the # of characters in pSrc if thats's less).
// Input  : *pDest - destination buffer
//			*pSrc - string to append
//			destBufferSize - sizeof the buffer pointed to by pDest
//			max_chars_to_copy - COPY_ALL_CHARACTERS in pSrc or max # to copy
// Output : char * the copied buffer
//-----------------------------------------------------------------------------
char* V_strncat(char* pDest, const char* pSrc, size_t destBufferSize, int max_chars_to_copy)
{
	size_t charstocopy = (size_t)0;
	size_t len = strlen(pDest);
	size_t srclen = strlen(pSrc);
	if (max_chars_to_copy <= COPY_ALL_CHARACTERS)
	{
		charstocopy = srclen;
	}
	else
	{
		charstocopy = (size_t)std::min(max_chars_to_copy, (int)srclen);
	}

	if (len + charstocopy >= destBufferSize)
	{
		charstocopy = destBufferSize - len - 1;
	}

	if (!charstocopy)
	{
		return pDest;
	}

	char* pOut = strncat(pDest, pSrc, charstocopy);
	pOut[destBufferSize - 1] = 0;
	return pOut;
}

void V_strncpy(char* pDest, char const* pSrc, int maxLen)
{
	strncpy(pDest, pSrc, maxLen);
	if (maxLen > 0)
	{
		pDest[maxLen - 1] = 0;
	}
}

char* V_stristr(char* pStr, char const* pSearch)
{
	return (char*)V_stristr((char const*)pStr, pSearch);
}

//-----------------------------------------------------------------------------
// Finds a string in another string with a case insensitive test
//-----------------------------------------------------------------------------
char const* V_stristr(char const* pStr, char const* pSearch)
{
	if (!pStr || !pSearch)
		return 0;

	char const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		// Skip over non-matches
		if (tolower((unsigned char)*pLetter) == tolower((unsigned char)*pSearch))
		{
			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (tolower((unsigned char)*pMatch) != tolower((unsigned char)*pTest))
					break;

				++pMatch;
				++pTest;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
	}

	return 0;
}
