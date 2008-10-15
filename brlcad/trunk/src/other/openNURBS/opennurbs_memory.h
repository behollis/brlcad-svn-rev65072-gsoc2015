/* $Header: $ */
/* $NoKeywords: $ */
/*
//
// Copyright (c) 1993-2007 Robert McNeel & Associates. All rights reserved.
// Rhinoceros is a registered trademark of Robert McNeel & Assoicates.
//
// THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
// ALL IMPLIED WARRANTIES OF FITNESS FOR ANY PARTICULAR PURPOSE AND OF
// MERCHANTABILITY ARE HEREBY DISCLAIMED.
//				
// For complete openNURBS copyright information see <http://www.opennurbs.org>.
//
////////////////////////////////////////////////////////////////
*/

#if !defined(OPENNURBS_MEMORY_INC_)
#define OPENNURBS_MEMORY_INC_

#if defined (cplusplus) || defined(_cplusplus) || defined(__cplusplus)
extern "C" {
#endif

ON_DECL
void*  onmalloc( size_t );

ON_DECL
void*  oncalloc( size_t, size_t );

ON_DECL
void   onfree( void* );

ON_DECL
void*  onrealloc( void*, size_t );

ON_DECL
size_t onmsize( const void* );

ON_DECL
void*  onmemdup( const void*, size_t );

ON_DECL
char*  onstrdup( const char* );

#if defined(_WCHAR_T_DEFINED)
ON_DECL
wchar_t* onwcsdup( const wchar_t* );
#endif

ON_DECL
unsigned char* onmbsdup( const unsigned char* );

ON_DECL
size_t onmemoryusecount(
	  size_t* malloc_count, 
	  size_t* realloc_count, 
	  size_t* free_count, 
	  size_t* pool_count 
	  );


/* define to handle _TCHAR* ontcsdup( const _TCHAR* ) */
#if defined(_UNICODE)
#define ontcsdup onwcsdup
#elif defined(_MBCS)
#define ontcsdup onmbsdup
#else
#define ontcsdup onstrdup
#endif

#if defined (cplusplus) || defined(_cplusplus) || defined(__cplusplus)
}
#endif

#endif
