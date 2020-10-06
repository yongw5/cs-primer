## 标准IO函数
```
/* @param
 * path：文件路径
 * mode：打开方式（r, r+, w, w+, a, a+）
 * return：成功返回 FILE 指针，失败返回 NULL，并且修改 errno 值*/
#include <stdio.h>
FILE *fopen(const char *path, const char *mode);

/* @param
 * fp：打开的文件指针
 * return：成功返回 0，失败返回 EOF，并且设置 errno
 */
int fclose(FILE *fp);

/* @param
 * stream：打开的文件指针（可以是 stdout，stdin，stderr）
 * return：成功返回字符对应的 ASCII 值,失败返回 EOF
 */
int fgetc(FILE *stream);

/* @param
 * c: 要输出的字符的 ASCII 值
 * stream：打开的文件指针
 * return：成功返回字符对应的 ASCII 值,失败返回 EOF
 */
int fputc(int c, FILE *stream);

/* @param
 * s:目的地址
 * size：读取 size 个字节（最多读取 size-1 个，最后一个字符 '\0'）
 * stream：打开的文件指针（可以是stdout，stdin，stderr）
 * return：成功返回 s,失败返回 NULL，设置 errno
 */
char *fgets(char *s, int size, FILE *stream);

/* @param
 * s: 要输出字符
 * stream：打开的文件指针
 * return：成功返回非负整数,失败返回 EOF，设置 errno
 */
int fputs(const char *s, FILE *stream);

/* @param
 * buffer: 保存读取数据的地址
 * size：每个对象的大小（字节数）
 * count：读取多少个对象
 * return：成功返回读取的对象个数, 如果返回值比 count 小，必须用 feof 或 ferror 来决定发生什么情况
 */
size_t fread(void *buffer, size_t size, size_t count, FILE *stream)

/* @param
 * buffer: 保存输出数据的地址
 * size：每个对象的大小（字节数）
 * count：输出多少个对象
 * return：成功返回读取的对象个数, 如果返回值比 count 小，必须用 feof 或 ferror 来决定发生什么情况
 */
size_t fwrite( const void *buffer, size_t size, size_t count, FILE *stream );

int printf(const char *format, ...);

int fprintf(FIFL *stream, const char *format, ...);

int scanf(const char *format, ...);

int fseek(FILE *stream, long offset, int whence);

long ftell(FILE *stream);

void rewind(FILE *stream);

int fflush(FILE *stream);
```

## 标准 IO 函数的两个优点
- 具有良好的移植性
- 可以利用缓冲提高性能

## 标准 IO 函数的几个缺点
- 不容易进行双向通信
- 有时可能频繁调用fflush函数
- 需要以FILE结构体指针的形式返回文件描述符

## 利用 fdopen() 函数转换为 FILE 结构体指针
```
#include <stdio.h>
/* @param
 * fildes：需要转换的文件描述符
 * mode：将要创建的 FILE 结构体指针的模式信息
 * return：成功返回 FILE 结构体指针，失败返回 NULL
 */
FILE *fdopen(int fildes, cont char *mode);
```

## 利用 fileno() 将函数转换为文件描述符
```
#include <stdio.h>
int fileno(FILE *stream); // 成功时返回转换后的文件描述符，失败时返回-1
```
