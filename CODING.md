# Coding guidelines

## Linter & Fixer
For c++ code, `clang-format` is used, according to [.clang-format](.clang-format) file.
Details at [documentation](https://clang.llvm.org/docs/ClangFormat.html)

    # basic example usage
    sudo apt install clang-format
    clang-format -i -style=file my-code.cpp

Some IDE integrations below, last resort use a pre-commit hook!

### Integrations
#### Codium / Visual Studio Code

#### C-Lion

#### vim using ALE plugin
Requirements: Already installed vim and ALE plugin
Add the following configuration to your `.vimrc`
```vimscript
" Enable ALE
let g:ale_linters = {
\   'cpp': ['clang-format'],
\}

" Automatically fix files on save
let g:ale_fix_on_save = 1

" Specify the fixer to use for C++
let g:ale_fixers = {
\   'cpp': ['clang-format'],
\}
```

## setup clang-format yourself
Only the first step is needed, we already defined a `.clang-format` file in the project.

    sudo apt install clang-format                           # install clang-format
    clang-format -style=gnu -dump-config > .clang-format    # create .clang-format settings file
    vim .clang-format                                       # modify .clang-format to your liking
    clang-format -i -style=file my-code.cpp                 # format a file

