/* Copyright (C) 2011-2012 SBA Research gGmbh

   This file is part of the Slibc Library.

   The Slibc Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The Slibc library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the Slibc Library; if not, see
   <http://www.gnu.org/licenses/>.  
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/file.h> 
#include <sys/stat.h>
#include <unistd.h>

#include "slibc.h"

/// the prefix of temporary files generated by slibc functions
/// can be at max 6 characters long
#define SLIBC_TMP_FILE_PREFIX "SC"


errno_t tmpfile_s(FILE * restrict * restrict streamptr)
{
	char tmp_filename[PATH_MAX];
	int fd = -1;

	_CONSTRAINT_VIOLATION_IF((!streamptr),
							 EINVAL,
							 EINVAL);

	if (slibc_get_tmp_dir(tmp_filename, sizeof(tmp_filename)))
		return -1;

	if (strcat_s(tmp_filename, sizeof(tmp_filename), SLIBC_TMP_FILE_PREFIX "XXXXXX"))
		return -1;

	fd = mkstemp(tmp_filename);
	if (fd == -1) 
	{
		/* Handle error */
		*streamptr = NULL;
		return -1;
	}
 
	/*
	 * Unlink immediately to hide the file name.
	 * The race condition here is inconsequential if the file
	 * is created with exclusive permissions (glibc >= 2.0.7)
	 */
	if (unlink(tmp_filename) == -1) 
	{
		/* Handle error */
		*streamptr = NULL;
		return -1;
	}
 
	*streamptr = fdopen(fd, "w+");
	if (*streamptr == NULL) 
	{
		close(fd);
		return -1;
	}
 
	return 0;
}


errno_t tmpnam_s(char *s, rsize_t maxsize)
{
	char tmp_dir[PATH_MAX];

	_CONSTRAINT_VIOLATION_CLEANUP_IF(\
		(!s || maxsize > RSIZE_MAX || maxsize < L_tmpnam_s),
		if(s && maxsize > 0) {s[0] = '\0';},
		EINVAL,
		EINVAL);		

	if (slibc_get_tmp_dir(tmp_dir, sizeof(tmp_dir)))
		return -1;

	// since maxsize is at least L_tmpnam, it is safe to pass it as an argument
	char *fn = tempnam(tmp_dir, SLIBC_TMP_FILE_PREFIX);
	if (!fn)
	{
		s[0] = '\0';
		return -1;
	}
	else
	{
		errno_t ret = strcpy_s(s, maxsize, fn);
		free(fn);
		return ret;
	}
}



static inline void get_s_cleanup(char *s)
{
	if(s)
		s[0] = '\0';
	// read and discard characters from stdin until \n,EOF, or read error
	char c;
	while((c = getc(stdin)) != EOF && c !='\n');
}

char *gets_s(char *s, rsize_t n)
{
	_CONSTRAINT_VIOLATION_IF( (!s || n > RSIZE_MAX || n == 0),
							  EINVAL,
							  NULL);

	char ch;
	rsize_t i = 0;
	for(; (i < n-1) && ((ch = getchar()) != EOF) && (ch != '\n'); i++ )
		s[i] = ch;

	s[i] = '\0';

	if (ch == EOF && i == 0)
	{
		// return NUll if no characters have been read
		return NULL;
	}
	else if (ferror(stdin))
	{
		// in case of a read error
		return NULL;
	}
	else if (ch == '\n' || ch == EOF)
		return s;
	else
	{
		// the standard considers it a runtime constraint if no newline or EOF/read-error
		// occurs within n
		get_s_cleanup(s);
		_CONSTRAINT_VIOLATION("Encountered no newline or EOF within the first n chars", EINVAL, NULL);
	}
}


errno_t fopen_s(FILE * restrict * restrict streamptr,
				const char * restrict filename,
				const char * restrict mode)
{
	_CONSTRAINT_VIOLATION_IF(streamptr == 0, EINVAL, EINVAL);
	_CONSTRAINT_VIOLATION_CLEANUP_IF( !mode || !filename, 
									  *streamptr = (FILE *) NULL, 
									  EINVAL, EINVAL);
	
	FILE * ret = NULL;

	if(mode[0] == 'u')
	{
		// u means default permissions, as the normal fopen would do it
		ret = fopen(filename, mode+1);
		if(!ret)
		{
			*streamptr = (FILE *) NULL;
			return -1;
		}
	} 
	else 
	{
		// NOT U
		// STANDARD: not u - set file permissions that prevent other users from access the file
		//        to the extent supported by the underlying plattform
		// IMPLEMENTATION: we ignore that requirement for now as there is no good
		// way to do it.

		ret = fopen(filename, mode);
		if(!ret)
		{
			*streamptr = (FILE *) NULL;
			return -1;
		}
	}
	// STD: To the extent that it is possible the standard asks for 
	// files to be opened with exclusive (also known as non-shared) access
	// IMPLEMENTATION: We don't do anything at the moment. 

	// was the file opened for writing?
	/* int write = 0; */
	/* for(int i=0; i< 3 && *mode; i++) */
	/* { */
	/* 	if (*mode == 'w' || *mode == 'r' || *mode == '+') */
	/* 		write = 1; */
	/* } */
	/* if (write) */
	/* { */
	/* 	// The standard wants files opened for writing opened with exclusive access. */
	/* 	(void) flock(fileid,LOCK_EX | LOCK_NB); */
	/* } */

	*streamptr = ret;
	return 0;
}

errno_t freopen_s(FILE * restrict * restrict newstreamptr,
				  const char * restrict filename,
				  const char * restrict mode,
				  FILE * restrict stream)
{
	_CONSTRAINT_VIOLATION_IF(newstreamptr == 0, EINVAL, EINVAL);
	_CONSTRAINT_VIOLATION_CLEANUP_IF( !mode || !stream, 
									  *newstreamptr = (FILE *) NULL, 
									  EINVAL, EINVAL);
	
	if(filename) 
	{
		(void) fclose(stream);
		if (fopen_s(newstreamptr, filename, mode) == 0)
		{
			return 0;
		}
		else
		{
			*newstreamptr = (FILE *) NULL;
			return -1;
		}
	} 
	else 
	{
		// A null argument for filename should lead to a change of mode
		//
		// According to the standard it is implementaiton-defined which
		// changes of mode are permitted.

		// we don't do anything at the moment.
		return 0;

		/* int fileid = fileno(stream); */
		/* if(mode[0] == 'u') */
		/* { */
		/* 	char * newmode = (char *) malloc(sizeof(char) * strlen(mode)); */
		/* 	rsize_t i; */
		/* 	for(i = 1; i< strlen(mode); i++) */
		/* 		newmode[i-1] = mode[i]; */
		/* 	int dupfile = dup(fileid); */
		/* 	fclose(stream); */
		/* 	FILE * ret = fdopen(dupfile, mode); */
		/* 	stream = ret; */
		/* 	if(!ret) */
		/* 	{ */
		/* 		*newstreamptr = (FILE *) NULL; */
		/* 		return -1; */
		/* 	} */
		/* 	*newstreamptr = stream; */
		/* 	return 0; */
		/* } else { */
		/* 	int dupfile = dup(fileid); */
		/* 	fclose(stream); */
		/* 	FILE * ret = fdopen(dupfile, mode); */
		/* 	fileid = fileno(ret); */
		/* 	int lock = flock(fileid,LOCK_EX | LOCK_NB); */
		/* 	if(lock == 0) { */
		/* 		stream = ret; */
		/* 		*newstreamptr = stream; */
		/* 		return 0; */
		/* 	} else { */
		/* 		stream = (FILE *) NULL; */
		/* 		*newstreamptr = (FILE *) NULL; */
		/* 		return -1; */
		/* 	} */
		/* } */
	}
}
