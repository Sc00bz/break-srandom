#include <stdio.h>
#include <stdlib.h>
#include "csprng.h"

#ifdef _WIN32
	HCRYPTPROV Csprng::m_hCryptProv = 0;
#else
	FILE *Csprng::m_fin = NULL;
#endif
int Csprng::m_init = Csprng::init();

#ifdef _WIN32

void Csprng::get(void *buffer, size_t size)
{
	if (size > 0)
	{
		if (!CryptGenRandom(m_hCryptProv, (DWORD) size, (BYTE*) buffer))
		{
			fprintf(stderr, "Error CryptGenRandom\n");
			exit(1);
		}
	}
}

int Csprng::init()
{
	static bool first = true;

	if (first)
	{
		first = false;
		if (!CryptAcquireContext(&m_hCryptProv, NULL, NULL, PROV_RSA_FULL, 0))
		{
			fprintf(stderr, "Error CryptAcquireContext\n");
			exit(1);
		}
	}
	return 0;
}

#else

void Csprng::get(void *buffer, size_t size)
{
	if (size <= 0 || fread(buffer, size, 1, m_fin) != 1)
	{
		fprintf(stderr, "fread \"/dev/urandom\"\n");
		exit(1);
	}
}

int Csprng::init()
{
	static bool first = true;

	if (first)
	{
		first = false;
		m_fin = fopen("/dev/urandom", "rb");
		if (m_fin == NULL)
		{
			perror("fopen \"/dev/urandom\"");
			exit(1);
		}
	}
	return 0;
}

#endif
