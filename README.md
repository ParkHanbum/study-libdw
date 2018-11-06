# Study libdw
some examples to study libdw. 
libdw is library to handle the DWARF information powered by redhat, Inc.

if you need information about DWARF, refer below link.

[DWARF - wikipedia](https://en.wikipedia.org/wiki/DWARF)


### Prepare to build

You must install the libdw before build this.
```
$ sudo apt-get install libdw-dev
```

### Build

```
$ make all
gcc -o target gvar.c -g -pg -Ddev=1  -ldw -lelf
gcc -o sample sample.c -g -pg -Ddev=1  -ldw -lelf
gcc -o test_resolve test_resolve_variables.c resolve_variables.c -g -pg -Ddev=1  -ldw -lelf
gcc -o show-die-info show-die-info.c -g -pg -Ddev=1  -ldw -lelf
```


### Generated files descriptions.

#### target
target program to test the examples.

#### show-die-info
this program has been extracted from elfutils.
show the DIE(Dwarf Information Entry) information from file that given as argument.


```
$ ./show-die-info target
file: target
New CU: off = 0, hsize = 11, ab = 0, as = 8, os = 4
     DW_TAG_typedef
      Name      : size_t
      Offset    : 45
      CU offset : 45
      Attrs     : name decl_file decl_line type
[SIBLING]

     DW_TAG_base_type
      Name      : long unsigned int
      Offset    : 56
      CU offset : 56
      Attrs     : name byte_size encoding
      byte size : 8
[SIBLING]

     DW_TAG_base_type
      Name      : unsigned char
      Offset    : 63
      CU offset : 63
      Attrs     : name byte_size encoding
      byte size : 1
[SIBLING]
```

#### test_resolve
This program parses the DIE information and extracts the globally declared variable types and names of the target program.

```
$ ./test_resolve target

  int  signed sys_nerr 4
  const char const * const[]  signed_char sys_errlist 1
  int  signed gvar_a 4
  long int  signed gvar_b 8
  char *  signed_char gvar_c 1
  char [128]  signed_char gvar_d 1
  int [64]  signed gvar_f 4
  const char const * const[]  signed_char gvar_expr 1
  const char * [10]  signed_char gvar_g 1
  struct global  void gs 48
  {
            int  signed a 4
            long int  signed b 8
            char *  signed_char c 1
            char [20]  signed_char arr 1
  }
  struct global2  void gs2 160
  {
            int  signed a 4
            long int  signed b 8
            char *  signed_char c 1
            struct inner1  void gb1 48
            {
                      int  signed a 4
                      unsigned int  unsigned ua 4
                      long int  signed b 8
                      char *  signed_char c 1
                      char [20]  signed_char arr 1
            }
            char [20]  signed_char arr 1
            struct inner2  void gb2 48
            {
                      int  signed a 4
                      long int  signed b 8
                      char *  signed_char c 1
                      char [20]  signed_char arr 1
            }
            struct inner1 *  void pgb1 48
            struct inner2 *  void pgb2 48
  }
```
