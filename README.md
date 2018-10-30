# Futhark Reader

Futhark reader is a c-implementation (ports well to cuda) of a parser able to
parse a compiled futhark-dataset. ie. the output of `futhar-dataset --binary`.

using this lib requires you to edit the header first as the implementation is
not as generic as I would've liked. (ie. edit the variables in the `InPutFormat`
struct to reflect the expected input values of the binary file.

Example usage is in [main.c](./main.c). Unfortunately this also
requires a dataset which is currently not available for public. (partly because
this is a private repository and partly because github don't allow files larger
than 100MB).
