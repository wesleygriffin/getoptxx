# getoptxx

Basic command line argument parser for C++14 and up.

The goal of this single file header library is to make it slightly easier to
specify options for parsing on the command line. The library attempts to
simplify the boilerplate from `getopt(3)` while using as little overhead as
possible.

Some memory allocation does occur, since the arguments object uses an
`unordered_map` and `vector` for storage. All strings, however, use
`string_view` so there are no string allocations unless an error occurs.
Currently the library throws an exception on error.

An example from the `getopt_long(3)` manpage using getoptxx is below. A more
realistic example is included in the API docs:
https://wesleygriffin.github.io/getoptxx

```
#include "getoptxx.h"

#include <cstdlib>
#include <err.h>

namespace go = getoptxx;

int main(int argc, char* const argv[]) {
    go::arguments args;
    try {
        args = go::arguments::parse(argc, argv, {
            {"e"},
            {"b"},
            {"d", go::option::argument_flags::required},
            {"0"},
            {"1"},
            {"2"},
            {"add", go::option::argument_flags::required},
            {"append"},
            {"c,create", go::option::argument_flags::required},
            {"delete", go::option::argument_flags::required},
            {"verbose"},
            {"file", go::option::argument_flags::required},
        });
    } catch (std::exception const& e) {
        errx(EXIT_FAILURE, "%s", e.what());
    }

    if (args.exists("add")) {
        std::printf("option add with arg %s\n", args["add"].data());
    }

    if (args.exists("append")) std::printf("option append\n");

    if (args.exists("delete")) {
        std::printf("option delete with arg %s\n", args["delete"].data());
    }

    if (args.exists("verbose")) std::printf("option verbose\n");

    if (args.exists("create")) {
        std::printf("option create with arg %s\n", args["create"].data());
    }

    if (args.exists("file")) {
        std::printf("option file with arg %s\n", args["file"].data());
    }

    if (args.exists("a")) std::printf("option a\n");
    if (args.exists("b")) std::printf("option b\n");

    if (args.exists("d")) {
        std::printf("option d with value %s\n", args["d"].data());
    }

    if (args.exists("0")) std::printf("option 0\n");
    if (args.exists("1")) std::printf("option 1\n");
    if (args.exists("2")) std::printf("option 2\n");

    std::printf("non-option ARGV-elements: ");
    for (auto&& arg : args.unparsed()) {
        std::printf("%s ", arg.data());
    }
    std::printf("\n");

    std::exit(EXIT_SUCCESS);
}
```

