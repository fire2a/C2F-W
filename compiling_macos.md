1. app store > install xcode
2. https://github.com/Homebrew/brew/releases/latest > install homebrew
3. install xcode essentials?
4.
fdo@MacBook-Air-de-Jacqueline ~ % cat .zprofile 
export HOMEBREW_PREFIX="/opt/homebrew";
export HOMEBREW_CELLAR="/opt/homebrew/Cellar";
export HOMEBREW_REPOSITORY="/opt/homebrew";
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin${PATH+:$PATH}";
export MANPATH="/opt/homebrew/share/man${MANPATH+:$MANPATH}:";
export INFOPATH="/opt/homebrew/share/info:${INFOPATH:-}";

abrir otro terminal

$ brew install boost eigen gcc
boost-mpi boost-build

