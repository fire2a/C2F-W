# Coding practices

https://clang.llvm.org/docs/ClangFormat.html

Details at `clang-format` [documentation](https://clang.llvm.org/docs/ClangFormat.html)

# setup clang-format

```bash
sudo apt-get install clang-format
clang-format -style=gnu -dump-config > .clang-format
```

## setup clang-format for vim using ALE plugin

Add the following configuration to your `.vimrc`

```bash
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

# GIT practices

Follow [naming conventions](https://github.com/naming-convention/naming-convention-guides/tree/master/git) when creating
branches and commit messages.

## Pull Request Rules

1. **Link to an Issue** – Every pull request must be associated with a corresponding issue that explains the motivation
   for the change and the expected outcome.
2. **Provide a Summary** – Include a comment summarizing the key changes in the pull request description. Highlight any
   major modifications or impacts.
3. **One Feature per Pull Request** – Keep pull requests focused on a single feature, fix, or improvement to ensure
   clarity and easier review.
4. **Cite Scientific References for Calculations** – If your code involves scientific calculations, include proper
   citations. Follow the documentation guidelines in `docs/README.md` to reference sources correctly.
