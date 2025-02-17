## TL;DR

```
sudo apt-get install doxygen
cd C2F-W
doxygen docs/Doxyfile
```

The generated files will be saved to the docs directory.

## Prerequisites

Ensure you have Doxygen installed on your system. You can find installation instructions in
the [official Doxygen documentation](https://www.doxygen.nl/manual/install.html).

## Writing documentation

Function docstrings should include:

- A brief description of the function.
- A more detailed explanation if necessary.
- A list of parameters with their types and descriptions.
- A description of the return value.
- A scientific reference if applicable.

To add a scientific reference in the code:

1. Add the citation in `docs/bibliography.bib` using `BibTeX` format.
2. Use the `@cite` tag in the function's docstring to reference the citation ID.

## Generating the documentation

Go to the `docs` directory and run the command `doxygen Doxyfile`. The generated HTML files will be stored in that
same
directory. You can open the main file `docs/html/index.html` using your favorite browser.