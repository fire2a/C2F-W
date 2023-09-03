# Compiling

```bash
make clean
make [-f custom_makefile]
```

## macos (M1)
### setup
1. app store > install xcode
2. https://github.com/Homebrew/brew/releases/latest > install homebrew
3. install xcode essentials?
4. run brew shellenv:
```
fdo@MacBook-Air ~ % cat .zprofile 
export HOMEBREW_PREFIX="/opt/homebrew";
export HOMEBREW_CELLAR="/opt/homebrew/Cellar";
export HOMEBREW_REPOSITORY="/opt/homebrew";
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin${PATH+:$PATH}";
export MANPATH="/opt/homebrew/share/man${MANPATH+:$MANPATH}:";
export INFOPATH="/opt/homebrew/share/info:${INFOPATH:-}";
```
4. open new terminal
### compile
Depending if homebrew was installed for one or all users, adjust include directory in `makefile` accordingly:
```
# user (default)
/opt/homebrew/include
# all (github runner)
/usr/local/include
```

```
# 1. install dependencies (maybe missing boost-mpi boost-build)
$ brew install boost eigen gcc libopenmpt
$ cd Cell2FireC
$ make clean
$ make -f makefile.macos
```

# DAVID
Resulta que usar variables globalaes como fls en Kitral.cpp estaba colisionando con el system `strings.h`

```
# from /Library/Developer/CommandLineTools/SDKs/MacOSX12.sdk/usr/include/strings.h:78
/* Darwin extensions */
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
__BEGIN_DECLS
int	 ffsl(long) __OSX_AVAILABLE_STARTING(__MAC_10_5, __IPHONE_2_0);
int	 ffsll(long long) __OSX_AVAILABLE_STARTING(__MAC_10_9, __IPHONE_7_0);
int	 fls(int) __OSX_AVAILABLE_STARTING(__MAC_10_5, __IPHONE_2_0);
```

# Nota al instalar LVM
```
==> llvm
To use the bundled libc++ please add the following LDFLAGS:
  LDFLAGS="-L/opt/homebrew/opt/llvm/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++"

llvm is keg-only, which means it was not symlinked into /opt/homebrew,
because macOS already provides this software and installing another version in
parallel can cause all kinds of trouble.

If you need to have llvm first in your PATH, run:
  echo 'export PATH="/opt/homebrew/opt/llvm/bin:$PATH"' >> ~/.zshrc

For compilers to find llvm you may need to set:
  export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
  export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"
