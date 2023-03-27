#pragma once

#include <type_traits>
#ifdef _WIN32
	#include <windows.h>
#else
	#include <stdio.h>
#endif
#include <stdint.h>

class Csprng
{
public:
	static void get(void *buffer, size_t size);

private:
	static int init();

#ifdef _WIN32
	static HCRYPTPROV  m_hCryptProv;
#else
	static FILE       *m_fin;
#endif
	static int         m_init;
};
