#ifndef FTRANSFER_H
#define FTRANSFER_H
#include <stdlib.h>


//extern int stat (const char *__restrict __file, struct stat *__restrict __buf);

extern int ftopen(const char *__file, int __oflag, size_t __size=0, ...);
extern int ftstat(const int __fd, struct stat *__restrict __buf);
extern size_t ftlseek( int __fd, size_t __offset, int __whence );
extern size_t ftwrite(int __fd, const void *__buf, size_t __n);
extern size_t ftread(int __fd, void *__buf, size_t __nbytes);
extern int ftclose (int __fd);
extern int ftstop( );


#endif // FTRANSFER_H
