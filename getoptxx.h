#if !defined(GUARD_GETOPTXX_H)
#define GUARD_GETOPTXX_H
#pragma once

#include <experimental/string_view>
#include <string>
#include <unordered_map>
#include <vector>

/*!
 * \mainpage
Basic command line argument parser for C++14 and up.

The goal of this single file header library is to make it slightly easier to
specify options for parsing on the command line. The library attempts to
simplify the boilerplate from `getopt(3)` while using as little overhead as
possible.

Some memory allocation does occur, since the arguments object uses an
`unordered_map` and `vector` for storage. All strings, however, use
`string_view` so there are no string allocations unless an error occurs.
Currently the library throws an exception on error.

The main entry point is getoptxx::v1::arguments::parse which takes a list
of getoptxx::v1::option values and parses the argc/argv command line
arguments. A basic example looks like the following:

\code
#include "getoptxx.h"

#include <cstdlib>
#include <err.h>

namespace {

namespace go = getoptxx;

struct params {
    bool debug;
    std::string directory;
    unsigned short port;
    unsigned int verbose;
    int warning;
    bool zed;
};

void usage(char const* argv0) {
    std::fprintf(stderr,
        "usage: %s [options] --port <P>\n"
        "    -h,--help             Display this help message.\n"
        "    --debug               Turn on debug checks.\n"
        "    -p,--port PORT        Listen on PORT for connections.\n"
        "    -v,--verbose LEVEL    Be verbose up to LEVEL.\n"
        "    -W LEVEL              Set warning level to LEVEL.\n"
        "    -z                    Do something.\n",
        argv0
    );
    std::exit(EXIT_SUCCESS);
}

params parse_args(int argc, char* const argv[]) {
    using aflags = go::option::argument_flags;
    using oflags = go::option::option_flags;

    go::arguments args;
    try {
        args = go::arguments::parse(argc, argv, {
            { "debug" },
            { "directory", aflags::required },
            { "p,port", aflags::required, oflags::required },
            { "v,verbose", aflags::optional },
            { "W", aflags::optional },
            { "z" }
        });
    } catch (std::exception const& e) {
        errx(EXIT_FAILURE, "%s", e.what());
    }

    if (args.help()) usage(argv[0]);

    return {
        static_cast<bool>(args.exists("debug")),
        args.exists("directory") ? args["directory"].data() : ".",
        [ps=args["port"].data(),&argv]{
            auto const p = std::stoi(ps);
            if (p<0) errx(EXIT_FAILURE, "port must be positive");
            return static_cast<unsigned short>(p);
        }(),
        [&args,&argv]{
            if (!args.exists("verbose")) return 0u;
            auto const v =
                args["verbose"].empty()?1 : std::stoi(args["verbose"].data());
            if (v<0) errx(EXIT_FAILURE, "verbose must be positive");
            return static_cast<unsigned int>(v);
        }(),
        args.exists("W")?args["W"].empty()?0:std::stoi(args["W"].data()):-1,
        static_cast<bool>(args.exists("z"))
    };
}

}

int main(int argc, char* argv[]) {
    auto const params = parse_args(argc, argv);
    std::printf("debug: %s\ndirectory: %s\nport: %u\nverbose: %u\n"
        "warning: %d\nzed: %s\n", (params.debug?"true":"false"),
        params.directory.c_str(), params.port, params.verbose, params.warning,
        (params.zed?"true":"false"));
}

\endcode
*/

/*! \brief Basic command line argument parser for C++14 and up. */
namespace getoptxx {
/*! \brief Inline versioning namespace */
inline namespace v1 {

struct option;

/*! \brief Holds the parsed and unparsed arguments. */
class arguments final {
public:
    /*!
     * \brief Parse the argc/argv command line arguments given a list of
     * getoptxx::option values.
     *
     * \param[in] argc Argument from main
     * \param[in] argv Argument from main
     * \param[in] options A list of getoptxx::v1::option values.
     * \return The parsed getoptxx::v1::arguments
     * \throws std::runtime_error if there is a parsing error.
     */
    static auto parse(int argc, char* const argv[],
                      std::initializer_list<option> options) -> arguments;

    /*! \brief The type of the key used to look up parsed arguments. */
    using key_type = std::experimental::string_view;

    /*! \brief The type of the value of the parsed option argument. */
    using value_type = std::experimental::string_view;

    /*!
     * \brief Indicates if the user requested help with -h,--help.
     * \return true if the user requested help or false if not.
     */
    bool help() const noexcept(true) { return m_help; }

    /*!
     * \brief Indicates if an option was parsed.
     * \param[in] key the option name to check.
     * \return true if \a key was parsed on the command line of false if not.
     */
    bool exists(key_type const& key) const { return m_parsed.count(key); }

    /*!
     * \brief Get the value of a parsed argument.
     * \param[in] key the option name to use.
     * \return the value of the parsed argument; may be empty if no value is
     * required or was provided and the value is optional.
     */
    auto operator[](key_type const& key) const {
        return m_parsed.find(key)->second;
    }

    /*!
     * \brief Get the value of a parsed argument.
     * \param[in] key the option name to use.
     * \return the value of the parsed argument; may be empty if no value is
     * required or was provided and the value is optional.
     */
    auto operator[](key_type&& key) const {
        return m_parsed.find(key)->second;
    }

    /*!
     * \brief Get the list of unparsed arguments.
     * \return the list of unparsed arguments.
     */
    auto const& unparsed() const noexcept(true) { return m_unparsed; }

    /*! \brief Default constructor. */
    constexpr arguments() = default;
    /*! \brief Copy constructor. */
    arguments(arguments const&) = default;
    /*! \brief Move constructor. */
    arguments(arguments&&) noexcept(true) = default;
    /*! \brief Copy assignment operator. */
    arguments& operator=(arguments const&) = default;
    /*! \brief Move assignment operator. */
    arguments& operator=(arguments&&) noexcept(true) = default;
    /*! \brief Destructor. */
    ~arguments() noexcept(true) = default;

    /*! \brief Swap this arguments object with another. */
    void swap(arguments&) noexcept(true);

private:
    bool m_help{false};
    std::unordered_map<key_type, value_type> m_parsed{};
    std::vector<value_type> m_unparsed{};
};

/*!
 * \brief A single command line option.
 */
struct option final {
    /*!
     * \brief Flags for arguments, which are what is parsed on the command
     * line.
     */
    enum class argument_flags {
        none,       /*!< No special behavior for this argument. */
        optional,   /*!< A value is required for this argument. */
        required,   /*!< A value is optional for this argument. */
    };

    /*!
     * \brief Flags for options, which are what is specified to the parser.
     */
    enum class option_flags {
        none     = 0x0, /*!< No special behavior for this option. */
        required = 0x1, /*!< This option is required on the command line. */
    };

    /*!
     * \brief Create a new option.
     * \param[in] name The short and long option names.
     * \param[in] aflags The \ref argument_flags of the parsed argument.
     * \param[in] oflags The \ref option_flags of the option.
     */
    constexpr option(arguments::key_type const& name,
        argument_flags aflags = argument_flags::none,
        option_flags oflags = option_flags::none) noexcept(true)
    : m_aflags{aflags}, m_oflags{oflags} {
        if (name.size()==1) {
            m_short = name;
        } else if (name[1]!=',') {
            m_long = name;
        } else {
            m_short = {&name[0], 1};
            m_long = {&name[2], name.size()-2};
        }
    }

    /*! \brief Copy constructor. */
    constexpr option(option const&) = default;
    /*! \brief Copy assignment operator. */
    option& operator=(option const&) = default;
    /*! \brief Destructor. */
    ~option() noexcept(true) = default;

    /*! \brief Swap this option object with another. */
    void swap(option&) noexcept(true);

private:
    arguments::key_type m_short{};
    arguments::key_type m_long{};
    argument_flags m_aflags{argument_flags::none};
    option_flags m_oflags{option_flags::none};

    friend class arguments;
};

inline void arguments::swap(arguments& o) noexcept(true) {
    using std::swap;
    swap(m_help, o.m_help);
    swap(m_parsed, o.m_parsed);
    swap(m_unparsed, o.m_unparsed);
}

/*!
 * \brief Swap two arguments objects.
 * \param[in] a
 * \param[in] b
 */
inline void swap(arguments& a, arguments& b) noexcept(true) { a.swap(b); }

inline void option::swap(option& o) noexcept(true) {
    using std::swap;
    swap(m_short, o.m_short);
    swap(m_long, o.m_long);
    swap(m_aflags, o.m_aflags);
    swap(m_oflags, o.m_oflags);
}

/*!
 * \brief Swap two option objects.
 * \param[in] a
 * \param[in] b
 */
inline void swap(option& a, option& b) noexcept(true) { a.swap(b); }

} // inline namespace v1
} // namespace getoptxx

inline auto getoptxx::v1::arguments::parse(int argc, char* const argv[],
    std::initializer_list<option> options) -> arguments {
    arguments args;
    auto const ae = std::find_if(argv+1, argv+argc,
                                 [] (key_type const& s) { return s == "--"; });

    for (auto&& a = argv+1; a != ae && !args.help(); ++a) {
        if (!*a) continue; // ignore empty arguments

        if ((*a)[0] != '-') { // non-option arguments
            args.m_unparsed.push_back(*a);
            continue;
        } else if (!(*a)[1]) { // ignore a single '-'
            continue;
        }

        auto const s = key_type{((*a)[1] == '-') ? &(*a)[2] : &(*a)[1]};
        args.m_help = (s == "h" || s == "help");
        if (args.m_help) return args;

        auto const o = std::find_if(std::begin(options), std::end(options),
            [&s] (auto o) { return s == o.m_short || s == o.m_long; });
        if (o == std::end(options)) {
            auto const what = "unknown option '"+s.to_string()+"'";
            throw std::runtime_error{std::move(what)};
        }

        auto const v = [&s,&o,&a,&ae] () -> value_type {
            if (o->m_aflags == option::argument_flags::required ||
                o->m_aflags == option::argument_flags::optional) {
                if (a+1 < ae && (a+1 && (*(a+1))[0] != '-')) {
                    return *(++a);
                } else if (o->m_aflags == option::argument_flags::required) {
                    auto const what =
                        "option '"+s.to_string()+"' requires a value";
                    throw std::runtime_error{std::move(what)};
                }
            }
            return {};
        }();

        if (!o->m_short.empty()) {
            args.m_parsed.insert(std::make_pair(o->m_short, v));
        }
        if (!o->m_long.empty()) {
            args.m_parsed.insert(std::make_pair(o->m_long, v));
        }
    }

    std::for_each(std::begin(options), std::end(options), [&args] (auto&& o) {
        if (o.m_oflags != option::option_flags::required) return;

        if ((!o.m_short.empty() && args.m_parsed.count(o.m_short) == 0)||
            (!o.m_long.empty() && args.m_parsed.count(o.m_long) == 0)) {
            auto const what = "option '"+
                (!o.m_long.empty()?o.m_long.to_string():o.m_short.to_string())+
                "' required";
            throw std::runtime_error{std::move(what)};
        }
    });

    args.m_unparsed.insert(std::end(args.m_unparsed), ae+1, argv+argc);
    return args;
}

#endif // !defined(GUARD_GETOPTXX_H)

