#!/bin/bash

# install software
sudo apt update -y
sudo apt install -y lcov clangd clang llvm
bash <(curl -fsSL https://xmake.io/shget.text)

# set git
git config --global alias.lg "log --color --graph --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr) %C(bold blue)<%an>%Creset' --abbrev-commit"
git config --global alias.br branch
git config --global alias.bra branch -a
git config --global alias.co checkout
git config --global alias.mg merge
git config --global alias.cp cherry-pick
git config --global alias.rb rebase
git config --global pull.rebase true

# set fish abbr
fish -c "abbr -a -U -- a 'abbr -a'"
fish -c "abbr -a -U -- g git"
fish -c "abbr -a -U -- gpl 'git pull'"
fish -c "abbr -a -U -- gps 'git push'"
fish -c "abbr -a -U -- gs 'git status'"
fish -c "abbr -a -U -- xm xmake"
fish -c "abbr -a -U -- yi 'sudo apt install -y'"
fish -c "abbr -a -U -- yu 'sudo apt update'"
fish -c "abbr -a -U -- ga 'git add -u'"
fish -c "abbr -a -U -- gc 'git commit'"
fish -c "abbr -a -U -- gl 'git lg'"
fish -c "abbr -a -U -- gco 'git checkout'"

