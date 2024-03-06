# stack
A Header-only Implementation of a Generic Stack

No need to build or link. To use, simply add the header to a source file like so:

```
#define GSTACK_IMPLEMENTATION
#define TEST_MAIN
#include "gstack.h"
```

All documentation can be found in `gstack.h`. It also includes a test `main()`
program if `TEST_MAIN` is defined before its inclusion.
