# hello-world

Naming

General Naming Rules

Names should be descriptive; avoid abbreviation.

int price_count_reader;    // No abbreviation.
int num_errors;            // "num" is a widespread convention.
int num_dns_connections;   // Most people know what "DNS" stands for.

File Names
Filenames should be all lowercase and can include underscores (_).

Type Names
Type names start with a capital letter and have a capital letter for each new word, with no underscores: MyExcitingClass, MyExcitingEnum.

Variable Names
The names of variables and data members are all lowercase, with underscores between words. 

Constant Names
Variables declared constexpr or const, and whose value is fixed for the duration of the program, are named with a leading "k" followed by mixed case. 

Function Names
Regular functions have mixed case; "cheap" functions may use lower case with underscores.
Ordinarily, functions should start with a capital letter and have a capital letter for each new word. Such names should not have underscores. Prefer to capitalize acronyms as single words (i.e. StartRpc(), not StartRPC()).

Enumerator Names
Enumerators (for both scoped and unscoped enums) should be named either like constants or like macros: either kEnumName or ENUM_NAME.
Preferably, the individual enumerators should be named like constants.
enum UrlTableErrors {
  kOK = 0,
  kErrorOutOfMemory,
  kErrorMalformedInput,
};


Macro Names
They should be named with all capitals and underscores.
#define ROUND(x) ...
#define PI_ROUNDED 3.0

