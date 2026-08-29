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
        compiler="$(brew --prefix gcc)/bin/g++-16"
        "$compiler" \
              -std=c++26 \
              -freflection \
              -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror \
              -g \
              -c "$f" \
              -o "build/$b.o"
        rc=$?
        [ "$rc" -eq 0 ] || exit "$rc"

        if grep -Eq '(^|[^[:alnum:]_])main[[:space:]]*\(' "$f"
        then
            "$compiler" \
                -std=c++26 \
                -freflection \
                -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror \
                "build/$b.o" \
                -o "build/$b"
            rc=$?
            [ "$rc" -eq 0 ] || exit "$rc"
            if [ "$2" != "--build-only" ]
            then
                "build/$b"
            fi
        fi

        exit 0
    fi
done

exit 1
