# Coding guidelines

## Linter & Fixer
### TL;DR
Install the clang-format linter and use it in your IDE or editor, it should automatically use the `.clang-format` file in the project root.

### Overview
For C++ code, `clang-format` is used to ensure consistent code formatting according to the rules specified in the [.clang-format](.clang-format) file. Formatting details can be found in the [documentation](https://clang.llvm.org/docs/ClangFormat.html).

    # basic example
    # install
    sudo apt install clang-format
    # use
    clang-format -i -style=file my-code.cpp

Some IDE or editor integrations are listed below. As a last resort, use a pre-commit hook!

### Integrations

#### Codium / Visual Studio Code
- Install the `Clang-Format` extension
- Add the following to your `settings.json`:
```json
{
    "clang-format.executable": "/usr/bin/clang-format",
    "clang-format.style": "file",
    "editor.formatOnSave": true
}
```

#### C-Lion
Requirements: Already installed CLion and have the appropiate `.clang-format` file in the project root.

CLion will automatically enable ClangFormat and apply the settings found in the file if it's under the project root.

To manually run the formatting, use **ctrl + alt + shift + L**.

You can also enable automatic formatting on save, go to **Settings | Tools | Actions on save** and enable "Reformat code".

You can find more information in the [official CLion documentation](https://www.jetbrains.com/help/clion/clangformat-as-alternative-formatter.html).

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

### Further read: how to setup clang-format from scratch
_Notice: This is not needed for this project, as we already have a `.clang-format` file in the project root._

    sudo apt install clang-format                           # install clang-format
    clang-format -style=gnu -dump-config > .clang-format    # create .clang-format settings file
    vim .clang-format                                       # modify .clang-format to your liking
    clang-format -i -style=file my-code.cpp                 # format a file

