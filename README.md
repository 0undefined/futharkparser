# Futhark binary datafile reader

Futhark reader is a c-implementation (ports well to cuda) of a parser which is
able to parse a compiled futhark-dataset. ie. the output of
`futhar-dataset --binary`.

Example usage is in [main.c](./main.c). Unfortunately to run the example
requires a dataset which is currently not available. (due to size limitations
on github)

The current version does not support other datatypes that `i32` and `f32`, but
do support multiple dimensional datatypes and scalars.


```C
print_topology(const char *filename);
```

Prints all the variable types and dimensions in the file.


```C
print_data(DataObj **in);
```

Same as `print_topology(..)`, except it takes an data-object as argument.
(saving your disk from constant r/w.

```C
parse(const char *filename, DataObj **in);
```

Actually parses the file into a dataobject. the dataobject is allocated from
within parse, so no need to do that.


## Roadmap for v. 1.1

* [ ] 'deconstructor' for the `DataObj` struct.
* [ ] Support for other datatypes.
* [ ] Better docs?
* [ ] Better availability in the api. (UX)
* [ ] Fix all the bad coding habits in the code.
* [ ] Improve maintainability/readability.
