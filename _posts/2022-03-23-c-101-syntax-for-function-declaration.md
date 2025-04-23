---
title: 'C 101: syntax for function declaration'
date: '2022-03-23T11:21:14+01:00'
layout: post
categories: c
---

In the early days of [K&R C](https://en.wikipedia.org/wiki/C_(programming_language)#K&R_C) programming the type for functions parameters was optional and declared separately:
```c
void old(a, b, c)
 int a;
 bool b;
 char *c;
{
 printf("a=%i b=%d c=%p\n", a, b, c);
}
```

For [ANSI C](https://en.wikipedia.org/wiki/ANSI_C) the syntax was changed, which allows the types to be specified directly before the arguments:
```c
void new(int a, bool b, char *c) {
 printf("a=%i b=%d c=%p\n", a, b, c);
}
```

They nearly looks the same, but there is a subtile draw-back to the old syntax: The [gcc](https://gcc.gnu.org/) compile will not report an error if you do not specify all arguments:

```c
#include <stdio.h>
#include <stdbool.h>
int main(void) {
#if 1
 old(); /* missed ? */
 old(0); /* missed ? */
 old(0, false); /* missed ? */
#endif
 old(0, false, ""); /* okay ? */
#if 1
 new(); /* error ? */
 new(0); /* error ? */
 new(0, false); /* error ? */
#endif
 new(0, false, ""); \* okay ? */
 return 0;
}
```

Even a `-Wall -pedantic -ansi -Werror` will not find it. Using [clang](https://clang.llvm.org/) on the other hand reports an error.

So watch out if you for example look at [CrudeSAML](https://github.com/univention/crudesaml/blob/master/cy2_saml.c#L109-L114), which uses both syntax at the same time.

PS: It you ask yourself what values are passed if they are not specified: random data respectively whatever is currently on your stack; so this is a security vulnerability as it discloses the content of your stack, which might contain confidential data.

*[gcc]: GNU Compiler Collection
