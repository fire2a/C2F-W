# C2F-W Code Documentation
## Local
The Doxigen generated files will be saved to the docs directory.
```
sudo apt install doxygen graphviz
cd C2F-W/docs
doxygen Doxyfile
firefox html/index.html
```

### Prerequisites

Ensure you have Doxygen installed on your system. You can find installation instructions in the [official Doxygen documentation](https://www.doxygen.nl/manual/install.html).

### Generating the documentation

Go to the `docs` directory and run the command `doxygen Doxyfile`. The generated HTML files will be stored in that same directory. You can open the main file `html/index.html` using your favorite browser (firefox).

## GitHub Pages
The documentation is hosted on GitHub Pages: [fire2a.github.io/C2F-W](https://fire2a.github.io/C2F-W)
It is automatically updated when a new commit is pushed to the `docs` branch, according to `.github/workflows/doxygen-gh-pages.yml`.

