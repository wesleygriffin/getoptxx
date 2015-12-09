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
    bool exists(key_type const& key) const { return m_parsed.count(key)>0; }

    /*!
     * \brief Get the value of a parsed argument.
     * \param[in] key the option name to use.
     * \return the value of the parsed argument; may be empty if no value is
     * required or was provided and the value is optional.
     */
    auto operator[](key_type const& key) const {
        if (!exists(key)) {
            throw std::runtime_error{ "no value for '"+key.to_string()+"'" };
        }
        return m_parsed.find(key)->second;
    }

    /*!
     * \brief Get the value of a parsed argument.
     * \param[in] key the option name to use.
     * \return the value of the parsed argument; may be empty if no value is
     * required or was provided and the value is optional.
     */
    auto operator[](key_type&& key) const {
        if (!exists(key)) {
            throw std::runtime_error{ "no value for '"+key.to_string()+"'" };
        }
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

private:
    bool m_help{ false };
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
    enum class argument_flags : short {
        none,       /*!< No value can be specified for this argument. */
        optional,   /*!< A value is optional for this argument. */
        required,   /*!< A value is required for this argument. */
    };

    /*!
     * \brief Flags for options, which are what is specified to the parser.
     */
    enum class option_flags : short {
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
    : aflags{ aflags }, oflags{ oflags },
      shortopt{
          (name.size()>1) ? ((name[1]==',') ? &name[0] : "") : &name[0],
          (name.size()>1) ? ((name[1]==',') ? 1u       : 0u) : 1u
      },
      longopt{
          (name.size()==1) ? "" : (name[1]==',') ? &name[2]      : &name[0],
          (name.size()==1) ? 0  : (name[1]==',') ? name.size()-2 : name.size()
      }
    {}

    /*! \brief Copy constructor. */
    constexpr option(option const&) = default;
    /*! \brief Move constructor. */
    constexpr option(option&&) noexcept(true) = default;
    /*! \brief Copy assignment operator. */
    option& operator=(option const&) = default;
    /*! \brief Move assignment operator. */
    option& operator=(option&&) noexcept(true) = default;
    /*! \brief Destructor. */
    ~option() noexcept(true) = default;

    argument_flags const aflags{argument_flags::none};
    option_flags const oflags{option_flags::none};
    arguments::key_type const shortopt{}, longopt{};
};

inline std::string to_string(option const& o) {
    if (o.longopt.empty()) return o.shortopt.to_string();
    else return o.longopt.to_string();
}

} // inline namespace v1
} // namespace getoptxx

inline auto getoptxx::v1::arguments::parse(int argc, char* const argv[],
    std::initializer_list<option> options) -> arguments {
    arguments args;
    auto const argend = std::find_if(argv+1, argv+argc,
                                     [](auto s) { return s=="--"; });

    for (auto&& arg=argv+1; arg!=argend && !args.help(); ++arg) {
        if (!*arg) continue; // ignore empty arguments

        if ((*arg)[0] != '-') { // non-option arguments
            args.m_unparsed.push_back(*arg);
            continue;
        } else if (!(*arg)[1]) { // ignore a single '-'
            continue;
        }

        // can now assume: a[0]=='-' && a.size()>1
        key_type const str{ ((*arg)[1]=='-') ? &(*arg)[2] : &(*arg)[1] };
        args.m_help = (str=="h" || str=="help");
        if (args.m_help) return args;

        auto const opt = std::find_if(std::begin(options), std::end(options),
            [&str](auto opt) { return str==opt.shortopt||str==opt.longopt; });
        if (opt==end(options)) {
            throw std::runtime_error{ "unknown option '"+str.to_string()+"'" };
        }

        auto const val = [&str,aflags=opt->aflags,&arg,&argend]()->value_type {
            bool const present = (arg+1<argend || (*(arg+1))[0]!='-');
            if (aflags==option::argument_flags::none) return {};
            if (aflags==option::argument_flags::required && !present) {
                throw std::runtime_error{
                    "option '"+str.to_string()+"' requires a value" };
            }
            if (present) return *(++arg);
            else return {};
        }();

        if (!opt->shortopt.empty()) args.m_parsed.emplace(opt->shortopt, val);
        if (!opt->longopt.empty()) args.m_parsed.emplace(opt->longopt, val);
    }

    std::for_each(std::begin(options), std::end(options), [&args](auto&& o) {
        if (o.oflags != option::option_flags::required) return;

        if ((!o.shortopt.empty() && args.m_parsed.count(o.shortopt)==0)||
            (!o.longopt.empty() && args.m_parsed.count(o.longopt)==0)) {
            throw std::runtime_error{ "option '"+to_string(o)+"' required" };
        }
    });

    args.m_unparsed.insert(std::end(args.m_unparsed), argend+1, argv+argc);
    return args;
}

#endif // !defined(GUARD_GETOPTXX_H)

