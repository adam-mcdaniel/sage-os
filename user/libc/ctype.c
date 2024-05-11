
int isdigit(char c)
{
	return c >= '0' && c <= '9';
}

int isalpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isalnum(char c)
{
	return isdigit(c) || isalpha(c);
}

int isupper(char c)
{
	return c >= 'A' && c <= 'Z';
}

int islower(char c)
{
	return c >= 'a' && c <= 'z';
}

int toupper(char c)
{
	if (islower(c))
	{
		return c - 'a' + 'A';
	}
	return c;
}

int tolower(char c)
{
	if (isupper(c))
	{
		return c - 'A' + 'a';
	}
	return c;
}

int isspace(char c)
{
	return c == ' ' || c == '\t' || c == '\n';
}