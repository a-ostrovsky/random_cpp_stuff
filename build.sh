#!/bin/sh

mkdir -p build

if [ "$#" -eq 0 ]
then
    [ -f build/.last ] || exit 1
    set -- "$(cat build/.last)"
fi

for f in $(find . -name '*.cpp')
do
    b=${f##*/}
    b=${b%.cpp}
    a=$(printf %s "$b" | tr -cd 'A-Z0-9')

    if [ "$b" = "$1" ] || [ "$a" = "$1" ]
    then
        printf '%s\n' "$1" > build/.last
        exec "$(brew --prefix gcc)/bin/g++-16" \
              -std=c++26 \
              -freflection \
              -c "$f" \
              -o "build/$b.o"
    fi
done

exit 1
