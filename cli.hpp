/**
 * Basic setup:
 * ```
 * #include "cli.hpp"
 * int main(int argc, char** argv) {
 *     linea::App app("myapp", "CLI tool");
 *     app.run(argc, argv);
 * }
 * ```
 * 
 * Options
 * ```
 * int port = 3000;
 * bool verbose = false;
 * 
 * app.option("--port, -p", port)
 *     .description("Port to run the server on")
 *     .default_value(3000);
 * 
 * app.flag("--verbose, -v", verbose)
 *     .description("Enable verbose logging")
 * ```
 * 
 * Positionals
 * ```
 * std::string filename;
 * 
 * app.argument("file", filename)
 *     .required()
 *     .description("File to process");
 * ```
 * 
 * Subcommands
 * ```
 * app.command("serve")
 *     .description("Start the server")
 *     .option("--port, -p", port)
 *     .run([&]() {
 *         std::cout << "Serving on port " << port << "\n";
 *     });
 * 
 * app.command("build")
 *     .description("Build the project")
 *     .flag("--release")
 *     .run([](auto args) {
 *         # Args could be structured
 *     });
 * ```
 * 
 * Type safe parsing:
 * - Conversion
 * - Validation errors
 * - Error messages
 * For example dealing with array/vector ins/outs, etc.
 * 
 * Validation
 * ```
 * app.option("--port", port)
 *     .check([](int p) {
 *         return p > 0 && p < 65536;
 *     }, "Port must be between 1 and 65535");
 * ```
 * 
 * Default command behaviour
 * ```
 * app.run([&]() {
 *     std::cout << "No command provided\n";
 * });
 * ```
 * 
 * Clean error handling
 * ```
 * Error: invalid value for --port: "abc"
 * ```
 * not
 * ```
 * std::invalid_argment thrown...
 * ```
 *
 * @todo
 * - Improve help
 *   - Coloured ouput (fallback if no TTY)
 *   - Auto-generated examples
 *   - Always clean alignment ofc
 *   - --help=json for machine-readable output
 *   - man-page
 *   - markdown
 * - Built in shell autocompletion
 *   - Generate completion scripts for:
 *     - bash
 *     - zsh
 *     - fish
 *   - Runtime completions (suggest files, enum values, subcommands dynamically, etc.)
 * - Interactive modes (linea::ui::prompt_)
 *   - Prompts:
 *     - Text
 *     - Password
 *     - Confirm
 *     - Select (Arrow keys + space bar/enter)
 *   - Fallback when non-interactive
 *   - Validation ofc
 * - Plugin system
 *   - ABI support as well
 *     - app.load_plugin("libdeploy.so")
 *     - Enables
 *       - Shared library plugins
 *       - Runtime loading
 *       - Version checks
 * - Built-in testing
 *   - e.g. auto result = cli.test("--port 3000"); assert(result.port == 3000);
 *   - Or something like that idk
 * - Async and signal handling
 *   - Ctrl+C
 *   - Async command support
 *   - Cancellation tokens
 * - Internationalisation (i18n)
 * - Built-in schema reflection
 *   - cmd.schema()
 *   - Returns
 *     - Option definitions
 *     - Positional definitions
 *     - Types
 *     - Defaults
 *     - Constraints
 *   - Enables
 *     - JSON help
 *     - IDE tooling
 *     - Completion
 * - Middleware
 */

#pragma once

#include <algorithm>
#include <any>
#include <chrono>
#include <concepts>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

/*
#if defined (_MSVC_LANG)
#define LINEA_CPLUSPLUS _MSVC_LANG
#else
#define LINEA_CPLUSPLUS __cplusplus
#endif

#define LINEA_CPP17 (LINEA_CPLUSPLUS >= 201703L)
#define LINEA_CPP20 (LINEA_CPLUSPLUS >= 202002L)
#define LINEA_CPP26 (LINEA_CPLUSPLUS >= 202603L)

#if LINEA_CPP17
#include <any>
#include <optional>
namespace linea {
    using any = std::any;
    template <typename T>
    inline T any_cast(const any& a) {
        return std::any_cast<T>(a);
    }

    template <typename T>
    using optional = std::optional<T>;
    constexpr auto nullopt = std::nullopt;
}
#else
namespace linea {
    class any {
    public:
        any() {}
        template <typename T>
        any(const T& v) : ptr(new derived<T>(v)) {}
        any(const any& other) : ptr(other.ptr ? other.ptr->clone() : nullptr) {}

        template <typename T>
        T cast() const {
            return static_cast<derived<T>*>(ptr.get())->value;
        }

        bool has_value() const {
            return ptr != nullptr;
        }

    private:
        struct base {
            virtual ~base() {}
            virtual std::unique_ptr<base> clone() const = 0;
        };
        template <typename T>
        struct derived : base {
            T value;
            derived(const T& v) : value(v) {}
            std::unique_ptr<base> clone() const override {
                return std::unique_ptr<base>(new derived(value));
            }
        };
        std::unique_ptr<base> ptr;
    };

    template <typename T>
    T any_cast(const any& a) {
        return a.cast<T>();
    }

    template <typename T>
    class optional {
    public:
        optional() : has(false) {}
        optional(const T& v) : has(true), value(v) {}
        bool has_value() const {
            return has;
        }
        T& operator*() {
            return value;
        }
        const T& operator*() const {
            return value;
        }
    
    private:
        bool has;
        T value;
    };

    struct nullopt_t{};
    static const nullopt_t nullopt{};
}
#endif
*/

#define REGISTER_COMMAND(ClassName, Name, Desc) \
    static linea::ControllerRegistrar<ClassName> _registrar_##ClassName(Name, Desc);

namespace linea {

class Args;

/**
 * Internal objects not intended for public use.
 */
namespace detail {

template <typename T>
struct function_traits;

template <typename T>
struct function_traits : function_traits<decltype(&T::operator())> {};

template <typename ClassType, typename ReturnType, typename Arg>
struct function_traits<ReturnType(ClassType::*)(Arg) const> {
    using arg_type = std::decay_t<Arg>;
};

template <typename ReturnType, typename Arg>
struct function_traits<ReturnType(*)(Arg)> {
    using arg_type = Arg;
};

template <typename Fn>
using function_arg_t = typename function_traits<Fn>::arg_type;

template <typename T>
std::string pretty_type() {
    return typeid(T).name();
}
template <> std::string pretty_type<int>() { return "int"; }
template <> std::string pretty_type<std::string>() { return "string"; }
template <> std::string pretty_type<double>() { return "double"; }

static inline std::string normalise(const std::string& name) {
    if (name.rfind("--", 0) == 0) return name.substr(2);
    if (name.rfind("-", 0) == 0) return name.substr(1);
    return name;
}

static inline void parse_names(const std::string& input, std::string& long_name, std::string& short_name) {
    auto comma = input.find(',');
    if (comma == std::string::npos) {
        long_name = normalise(input);
    } else {
        long_name = normalise(input.substr(0, comma));
        short_name = normalise(input.substr(comma + 1));
    }
}

// Command helpers
template <typename T, typename = void>
struct has_bind : std::false_type{};

template <typename T>
struct has_bind<T, std::void_t<decltype(T::bind(std::declval<T&>(), std::declval<const Args&>()))>> : std::true_type {};

template <typename T>
inline constexpr bool has_bind_v = has_bind<T>::value;

// AutoRegisteredCommand helpers
template <typename T>
concept HasConstexprName = requires {
    { T::command_name } -> std::convertible_to<const char*>;
};

template <typename T>
concept HasConstexprDescription = requires {
    { T::command_description } -> std::convertible_to<const char*>;
};

template <typename T>
concept HasFunctionName = requires {
    { T::command_name() } -> std::convertible_to<std::string>;
};

template <typename T>
concept HasFunctionDescription = requires {
    { T::command_description() } -> std::convertible_to<std::string>;
};

template <typename T>
std::string get_command_name() {
    if constexpr (HasConstexprName<T>) {
        return T::command_name;
    } else if constexpr (HasFunctionName<T>) {
        return T::command_name();
    } else {
        static_assert(sizeof(T) == 0, "Command must define command_name");
    }
}

template <typename T>
std::string get_command_description() {
    if constexpr (HasConstexprDescription<T>) {
        return T::command_description;
    } else if constexpr (HasFunctionDescription<T>) {
        return T::command_description();
    } else {
        return "";
    }
}

// Enable ANSI support for Windows
void enableANSI() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

} // namespace detail

enum class ErrorKind {
    UnknownCommand,
    UnknownOption,
    MissingValue,
    InvalidValue,
    MissingPositional
};

struct CLIError : std::runtime_error {
    ErrorKind kind;

    CLIError(ErrorKind k, std::string msg) : std::runtime_error(std::move(msg)), kind(k) {}

    std::string_view message() const {
        return "\033[31m[Error]:\033[0m " + std::string(std::runtime_error::what());
    }
};

class Command;

class OptionBase {
public:
    std::string long_name;
    std::string short_name;
    std::string description;
    bool has_default = false;
    Command* parent = nullptr;

    virtual void set(const std::string& value) = 0;
    virtual std::string type_name() const = 0;
    virtual bool is_flag() const = 0;

    virtual std::any get_value() const = 0;

    virtual void print_default(std::ostream& os) const {
        if (has_default) os << "(Default: <unknown type>)";
    }

    template <typename T>
    T get_value_typed() const {
        auto val = get_value();
        try {
            return std::any_cast<T>(val);
        } catch (...) {
            throw std::runtime_error("Type mismatch for argument: " + long_name);
        }
    }

    Command& done() const {
        return *parent;
    }

    virtual ~OptionBase() = default;
};

template <typename T>
class Positional;

struct PositionalInfo {
    std::string name;
    OptionBase* opt;
    bool required = false;
};

class Args {
public:
    const std::unordered_map<std::string, OptionBase*>* options;
    const std::vector<PositionalInfo>* positionals;

    template <typename T>
    T get(const std::string& name) const {
        auto it = options->find(name);
        if (it == options->end()) {
            throw std::runtime_error("Unknown argument: " + name);
        }

        std::any val = it->second->get_value();
        try {
            return std::any_cast<T>(val);
        } catch (...) {
            throw std::runtime_error("Type mismatch for argument: " + name);
        }
    }

    template <typename T>
    std::optional<T> try_get(const std::string& name) const {
        auto it = options->find(name);
        if (it == options->end()) return std::nullopt;

        try {
            return std::any_cast<T>(it->second->get_value());
        } catch (...) {
            return std::nullopt;
        }
    }

    template <typename T>
    T positional(size_t index) const {
        if (index >= positionals->size()) {
            throw std::runtime_error("Positional index out of range");
        }

        return positionals->at(index).opt->template get_value_typed<T>();
    }

    template <typename T>
    T positional(const std::string& name) const {
        for (auto& pos : *positionals) {
            if (pos.name == name) {
                return pos.opt->template get_value_typed<T>();
            }
        }
        throw std::runtime_error("Unknown positional: " + name);
    }
};

/**
 * Allows for the following example usage:
 * @brief
 * ```
 * class ServeCommand : public linea::CommandController {
 * public:
 *   void setup(linea::Command& cmd) override {
 *     cmd.option("--port, -p", port)
 *          .description_text("Port to serve on").done()
 *        .flag("--verbose, -v", verbose).done();
 *   }
 * 
 *   void execute(const linea::Args& args) override {
 *     std::cout << "Starting server on " << port
 *               << (verbose ? "(verbose)" : "") << std::endl;
 *   }
 * 
 * private:
 *   int port = 8080;
 *   bool verbose = false;
 * };
 * 
 * REGISTER_COMAND(ServeCommand, "serve", "Start the web server")
 * ```
 * 
 * You can also declare fields like:
 * @brief
 * ```
 * linea::Flag verbose{this, "--verbose, -v", "Enable logging"};
 * linea::Option<int> port = linea::Option<int>(int, "--port, -p", "Port to serve on")
 *                             .default_value(8080)
 *                             .check([](int p) { return p > 0 && < 65535 },
 *                                    "Port must be between 1 and 65536");
 * ```
 */
class CommandController {
public:
    virtual ~CommandController() = default;
    virtual void setup(Command& cmd) = 0;
    virtual void execute(const Args& args) = 0;

    struct CommandSchema {
        std::unordered_map<std::string, OptionBase*> options;
        std::vector<PositionalInfo> positionals;
    };

    CommandSchema schema;

    struct Registration {
        std::string name;
        std::string description;
        std::function<std::unique_ptr<CommandController>()> creator;
    };

    static std::vector<Registration>& get_registry() {
        static std::vector<Registration> registry;
        return registry;
    }

protected:
    void register_schema_option(const std::string& option, OptionBase* opt) {
        schema.options[option] = opt;
    }

    void register_schema_positional(const std::string& name, OptionBase* opt, bool required = false) {
        schema.positionals.push_back({name, opt, required});
    }
};

template <typename T>
struct ControllerRegistrar {
    ControllerRegistrar(const std::string& name, const std::string& desc) {
        auto& reg = CommandController::get_registry();

        auto it = std::find_if(reg.begin(), reg.end(), [&](const auto& r) { return r.name == name; });

        if (it == reg.end()) {
            reg.push_back({
                name,
                desc,
                []() -> std::unique_ptr<CommandController> {
                    return std::make_unique<T>();
                }
            });
        }
    }
};

template <typename T>
class Option : public OptionBase {
public:
    T value;
    T default_val{};

    Option(const std::string& name, T& ref) : target(&ref) {
        detail::parse_names(name, long_name, short_name);
    }

    Option(const std::string& name) : target(nullptr) {
        detail::parse_names(name, long_name, short_name);
    }

    Option(CommandController* ctx, const std::string& name, const std::string& desc = "") {
        detail::parse_names(name, long_name, short_name);
        description = desc;

        if (ctx) {
            ctx->register_schema_option(long_name, this);
        }
    }

    Option& description_text(const std::string& desc) {
        description = desc;
        return *this;
    }

    Option& default_value(const T& val) {
        default_val = val;
        has_default = true;
        set_value(val);
        return *this;
    }

    void print_default(std::ostream& os) const override {
        if (has_default) os << "(Default: " << default_val << ")";
    }

    Option& check(std::function<bool(const T&)> fn, const std::string& msg) {
        validators.emplace_back(fn, msg);
        return *this;
    }

    void set(const std::string& str) override {
        std::istringstream iss(str);
        T tmp;
        iss >> tmp;
        if (!iss || !iss.eof()) {
            throw CLIError(ErrorKind::InvalidValue, "Invalid value for --" + long_name + ": \"" + str + "\"");
        }

        for (auto& [fn, msg] : validators) {
            if (!fn(tmp)) throw CLIError(ErrorKind::InvalidValue, msg);
        }

        set_value(tmp);
    }

    std::string type_name() const override {
        return detail::pretty_type<T>();
    }

    bool is_flag() const override {
        return false;
    }

    std::any get_value() const override {
        return target ? *target : value;
    }

private:
    T* target;

    std::vector<std::pair<std::function<bool(const T&)>, std::string>> validators;

    void set_value(const T& val) {
        value = val;
        if (target) *target = val;
    }
};

template <typename T>
class Positional : public Option<T> {
public:
    bool required = false;

    Positional(CommandController* ctx, const std::string& name, const std::string& desc = "") : Option<T>(nullptr, name, desc) {
        this->long_name = name;

        if (ctx) {
            ctx->register_schema_positional(name, this, false);
        }
    }

    Positional(CommandController* ctx, const std::string& name, const std::string& desc, bool required) : Option<T>(nullptr, name, desc) {
        this->long_name = name;

        if (ctx) {
            ctx->register_schema_positional(name, this, required);
        }
    }

    Positional(CommandController* ctx, const std::string& name, bool required) : Positional(ctx, name, "", required) {}

    /*Positional(CommandController* ctx, const std::string& name, const std::string& desc, bool required) : Option<T>(nullptr, name, desc) {
        this->long_name = name;

        if (ctx) {
            ctx->schema.positionals.push_back({name, this, required});
        }
    }*/

    Positional& set_required(bool is_req = true) {
        required = is_req;
        return *this;
    }
};

class Flag : public OptionBase {
public:
    Flag(const std::string& name, bool& ref) : target(&ref), value(false) {
        detail::parse_names(name, long_name, short_name);
    }

    Flag(const std::string& name) : target(nullptr), value(false) {
        detail::parse_names(name, long_name, short_name);
    }

    void set(const std::string&) override {
        value = true;
        if (target) *target = true;
    }

    void set(const bool val) {
        value = val;
        if (target) *target = val;
    }

    std::string type_name() const override {
        return "bool";
    }

    bool is_flag() const override {
        return true;
    }

    std::any get_value() const override {
        return target ? *target : value;
    }

private:
    bool value = false;
    bool* target = nullptr;
};

class Command {
public:
    std::string name;
    std::string description;

    std::function<void(const Args&)> action;

    std::vector<std::unique_ptr<OptionBase>> options;
    std::unordered_map<std::string, OptionBase*> option_map;

    std::vector<PositionalInfo> positionals;

    Command(const std::string& n) : name(n) {}

    template <typename T>
    Option<T>& option(const std::string& name, T& var) {
        std::unique_ptr<Option<T>> opt = std::make_unique<Option<T>>(name, var);
        Option<T>* ptr = opt.get();

        ptr->parent = this;

        option_map[ptr->long_name] = ptr;
        if (!ptr->short_name.empty()) {
            option_map[ptr->short_name] = ptr;
        }

        options.push_back(std::move(opt));
        return *ptr;
    }

    template <typename T>
    Option<T>& argument(const std::string& name, T& var) {
        std::unique_ptr<Option<T>> opt = std::make_unique<Option<T>>(name, var);
        Option<T>* ptr = opt.get();
        positionals.push_back({name, ptr, false});
        options.push_back(std::move(opt));
        return *ptr;
    }

    Flag& flag(const std::string& name, bool& var) {
        auto opt = std::make_unique<Flag>(name, var);
        auto* ptr = opt.get();

        ptr->parent = this;

        option_map[ptr->long_name] = ptr;
        if (!ptr->short_name.empty()) {
            option_map[ptr->short_name] = ptr;
        }
        
        options.push_back(std::move(opt));
        return *ptr;
    }

    Command& required() {
        positionals.back().required = true;
        return *this;
    }

    template <typename Fn>
    Command& run(Fn fn) {
        action = [this, fn](const Args& args) {
            invoke(fn, args);
        };
        return *this;
    }

    template <typename T>
    Command& bind(std::function<T(const Args&)> fn) {
        binder = [fn](const Args& args) -> std::any {
            return fn(args);
        };
        return *this;
    }

    void print_help(const std::string& app_name) {
        std::cout << app_name << " " << name;
        if (!description.empty()) {
            std::cout << " - " << description;
        }
        std::cout << "\n\n";

        std::cout << "Usage:\n";
        std::cout << "  " << app_name << " " << name << " [options]\n\n";

        size_t max_width = 0;
        std::vector<std::string> option_names;

        for (auto& opt : options) {
            std::string names = "";

            if (!opt->short_name.empty()) {
                names += "-" + opt->short_name;
                if (!opt->long_name.empty()) {
                    names += ", ";
                }
            }

            if (!opt->long_name.empty()) {
                names += "--" + opt->long_name;
            }

            if (!opt->is_flag()) {
                names += " <" + opt->type_name() + ">";
            }

            option_names.push_back(names);
            if (names.size() > max_width) {
                max_width = names.size();
            }
        }

        if (!options.empty()) {
            std::cout << "Options\n";

            for (size_t i = 0; i < options.size(); ++i) {
                auto& opt = options[i];
                auto& names = option_names[i];

                std::string desc = opt->description;

                std::cout << "  " << std::left << std::setw(max_width + 4) << names << desc;

                if (opt->has_default) {
                    opt->print_default(std::cout);
                }

                std::cout << "\n";
            }

            std::cout << "\n";
        }
    }

private:
    std::function<std::any(const Args&)> binder;

    template <typename T>
    T resolve(const Args& args) {
        if (binder) {
            auto val = binder(args);
            if (val.type() != typeid(T)) {
                throw std::runtime_error("Binder returned wrong type");
            }
            return std::any_cast<T>(val);
        }

        if constexpr (std::is_same_v<T, Args>) {
            return args;
        } else if constexpr (detail::has_bind_v<T>) {
            T obj{};
            T::bind(obj, args);
            return obj;
        } else if constexpr (std::is_constructible_v<T, const Args&>) {
            return T(args);
        } else {
            static_assert(!std::is_same_v<T, T>,
                "linea::Command::run(): Cannot construct handler argument.\n"
                "Provide one of:\n"
                " - []()\n"
                " - [](linea::Args)\n"
                " - [](T) where T has:\n"
                "     * T::bind(T& Args)\n"
                "     * or constructor T(Args)\n"
                "     * or .bind<T>() on command"
            );
        }
    }

    template <typename Fn>
    void invoke(Fn& fn, const Args& args) {
        if constexpr (std::is_invocable_v<Fn, const Args&>) {
            fn(args);
        } else if constexpr (std::is_invocable_v<Fn>) {
            fn();
        } else {
            using T = detail::function_arg_t<Fn>;
            fn(resolve<T>(args));
        }
    }
};

/**
 * Allows:
 * @brief
 * ```cpp
 * class ServeCommand : public linea::AutoRegisteredCommand<ServeCommand> {
 * public:
 *     static std::string command_name() { return "serve"; }
 *     static std::string command_description() { return "Start the web server"; }
 * 
 *     void setup(linea::Command& cmd) override {
 *     cmd.option("--port, -p", port)
 *          .description_text("Port to serve on").done()
 *        .flag("--verbose, -v", verbose).done();
 *   }
 * 
 *   void execute(const linea::Args& args) override {
 *     std::cout << "Starting server on " << port
 *               << (verbose ? "(verbose)" : "") << std::endl;
 *   }
 * 
 * private:
 *     int port = 8080;
 *     bool verbose = false;
 * };
 * ```
 */
template <typename Derived>
class AutoRegisteredCommand : public CommandController {
protected:
    static inline ControllerRegistrar<Derived> registrar{
        detail::get_command_name<Derived>(),
        detail::get_command_description<Derived>()
    };
};

class App {
public:
    std::string name;
    std::string description;

    App(const std::string& n, const std::string& desc = "") : name(n), description(desc) {}

    Command& command(const std::string& name) {
        auto cmd = std::make_unique<Command>(name);
        auto* ptr = cmd.get();
        commands.push_back(std::move(cmd));
        command_map[name] = ptr;
        return *ptr;
    }

    int run(int argc, char** argv) {
        try {
            detail::enableANSI();

            if (argc < 2) {
                print_help();
                return 0;
            }

            load_controllers();

            std::string cmd_name = argv[1];

            auto it = command_map.find(cmd_name);
            if (it == command_map.end()) {
                throw CLIError(ErrorKind::UnknownCommand, "Unknown command: " + cmd_name + "\n");
            }
            Command& cmd = *it->second;

            size_t pos_index = 0;
            for (int i = 2; i < argc; ++i) {
                std::string arg = argv[i];

                if (arg == "--help" || arg == "-h") {
                    cmd.print_help(name);
                    return 0;
                }

                if (arg.rfind("--", 0) == 0 && arg.find('=') != std::string::npos) {
                    auto eq = arg.find('=');
                    std::string key = detail::normalise(arg.substr(0, eq));
                    std::string value = arg.substr(eq + 1);

                    auto it = cmd.option_map.find(key);
                    if (it == cmd.option_map.end()) {
                        throw CLIError(ErrorKind::UnknownOption, "Unknown option: --" + key + "\n");
                    }

                    try {
                        it->second->set(value);
                    } catch (...) {
                        throw CLIError(ErrorKind::InvalidValue, "Error: invalid value for --" + key + ": \"" + value + "\"\n");
                    }

                    continue;
                }

                if (arg.rfind("--", 0) == 0) {
                    std::string key = detail::normalise(arg);

                    auto it = cmd.option_map.find(key);
                    if (it == cmd.option_map.end()) {
                        throw CLIError(ErrorKind::UnknownOption, "Unknown option: " + arg + "\n");
                    }

                    auto* opt = it->second;

                    if (opt->is_flag()) {
                        opt->set("");
                    } else {
                        if (i + 1 >= argc) {
                            throw CLIError(ErrorKind::MissingValue, "Missing value for " + arg + "\n");
                        }

                        std::string value = argv[++i];
                        
                        try {
                            opt->set(value);
                        } catch (...) {
                            throw CLIError(ErrorKind::InvalidValue, "Error: invalid value for " + arg + ": \"" + value + "\"\n");
                        }
                    }

                    continue;
                }

                if (arg.rfind("-", 0) == 0) {
                    for (size_t j = 1; j < arg.size(); ++j) {
                        std::string key(1, arg[j]);
                        
                        auto it = cmd.option_map.find(key);
                        if (it == cmd.option_map.end()) {
                            throw CLIError(ErrorKind::UnknownOption, "Unknown option: -" + key + "\n");
                        }

                        auto* opt = it->second;

                        if (opt->is_flag()) {
                            opt->set("");
                        } else {
                            if (j != arg.size() - 1) {
                                throw CLIError(ErrorKind::InvalidValue, "Option -" + key + " must be last in group to take a value\n");
                            }

                            if (i + 1 >= argc) {
                                throw CLIError(ErrorKind::MissingValue, "Missing value for -" + key + "\n");
                            }

                            std::string value = argv[++i];
                            opt->set(value);
                        }

                    }

                    continue;
                }

                if (pos_index < cmd.positionals.size()) {
                    auto& pos = cmd.positionals[pos_index++];
                    try {
                        pos.opt->set(arg);
                    } catch (...) {
                        throw CLIError(ErrorKind::InvalidValue, "Error: invalid value for positional argument " + pos.name + ": \"" + arg + "\"\n");
                    }
                    continue;
                }

                throw CLIError(ErrorKind::UnknownOption, "Unexpected argument: " + arg + "\n");
            }

            for (size_t i = 0; i < cmd.positionals.size(); ++i) {
                auto& pos = cmd.positionals[i];
                if (pos.required && i >= pos_index) {
                    throw CLIError(ErrorKind::MissingPositional, "Missing required positional argument: " + pos.name + "\n");
                }
            }

            if (cmd.action) {
                Args args{ &cmd.option_map, &cmd.positionals };
                cmd.action(args);
            }  else {
                throw CLIError(ErrorKind::UnknownCommand, "Unknown command: " + cmd_name + "\n");
            }

            return 0;
        } catch (const CLIError& e) {
            std::cerr << e.message() << "\n";
            return static_cast<int>(e.kind);
        } catch (const std::exception& e) {
            std::cerr << "Internal error: " << e.what() << "\n";
            return 1;
        }
    }

    void print_help() {
        std::cout << name;
        if (!description.empty()) {
            std::cout << " - " << description;
        }
        std::cout << "\n\n";
        
        std::cout << "Usage:\n";
        std::cout << "  " << name << " <command> [options]\n\n";

        size_t max_width = 0;
        for (auto& cmd : commands) {
            if (cmd->name.size() > max_width) {
                max_width = cmd->name.size();
            }
        }

        std::cout << "Commands:\n";
        for (auto& cmd : commands) {
            std::cout << "  " << std::left << std::setw(max_width + 4) << cmd->name << cmd->description << "\n";
        }

        std::cout << "\n";
    }

private:
    std::vector<std::unique_ptr<Command>> commands;
    std::unordered_map<std::string, Command*> command_map;

    void load_controllers() {
        for (auto& reg : CommandController::get_registry()) {
            auto& cmd = this->command(reg.name);
            cmd.description = reg.description;

            auto controller_instance = reg.creator();
            controller_instance->setup(cmd);

            std::shared_ptr<CommandController> shared_controller = std::move(controller_instance);
            cmd.run([shared_controller](const Args& args) {
                shared_controller->execute(args);
            });

        }
    }
};

namespace ui {

class ProgressBar {
public:
    struct Theme {
        char fill = '#';
        char empty = '-';
        char left = '[';
        char right = ']';

        std::string fill_color = "";
        std::string empty_color = "";
        std::string reset_color = "\033[0m";
    };

    struct Options {
        bool progress_in_bar = false;
        bool eta_enabled = true;
        bool items_per_second = false;
    };

    struct Themes {
        static Theme classic() {
            return {'#', '-', '[', ']'};
        }

        static Theme minimal() {
            return {
                .fill = '=',
                .empty = ' ',
                .left =  '|',
                .right = '|'
            };
        }
    };

    ProgressBar(size_t total, size_t width = 50, Theme theme = Themes::classic(), Options options = {}) : total_(total), value_(0), width_(width), theme_(theme), options_(options) {
        detail::enableANSI();
    }

    ProgressBar& set(size_t value) {
        value_ = std::min(value, total_);
        render();
        return *this;
    }

    ProgressBar& advance(size_t step = 1) {
        set(value_ + step);
        return *this;
    }

    // bar += n (advances by n)
    ProgressBar& operator+=(int v) {
        return advance(v);
    }

    ProgressBar& setLabel(const std::string& label) {
        label_ = label;
        return *this;
    }

    ProgressBar& setTheme(Theme theme) {
        theme_ = theme;
        return *this;
    }

    ProgressBar& setOptions(Options options) {
        options_ = options;
        return *this;
    }

    ProgressBar& enableAutoETA(bool enabled = true) {
        options_.eta_enabled = enabled;
        return *this;
    }

    ProgressBar& setETA(double seconds) {
        manual_eta_seconds_ = seconds;
        return *this;
    }

    ProgressBar& clearETA() {
        manual_eta_seconds_.reset();
        return *this;
    }

    ProgressBar& customFormatter(std::function<std::string(const ProgressBar&)> formatter) {
        formatter_ = formatter;
        render();
        return *this;
    }

    void render() const {
        if (formatter_) {
            std::cout << "\r" << formatter_(*this) << std::flush;
            return;
        }

        float ratio = total_ == 0 ? 0.0f : (float)value_ / total_;
        size_t filled = static_cast<size_t>(ratio * width_);

        std::string raw = buildRawBar(filled);
        int percent = std::clamp(static_cast<int>(ratio * 100), 0, 100);
        overlayPercent(raw, percent);
        std::string bar = applyColor(raw);

        /* if (theme_.progress_in_bar) {
            std::string percent_str = std::to_string(percent) + '%';

            size_t start = (width_ > percent_str.size()) ? (width_ - percent_str.size()) / 2 : 0;

            for (size_t i = 0; i < percent_str.size() && start + i < bar.size(); ++i) {
                bar[start + i] = percent_str[i];
            }
        } */

        std::cout << "\r";

        if (!label_.empty()) {
            std::cout << label_ << " ";
        }

        std::cout << theme_.left << bar << theme_.right;

        if (!options_.progress_in_bar) {
            std::cout << " " << percent << "%";
        }

        if (options_.eta_enabled || manual_eta_seconds_.has_value()) {
            double eta = manual_eta_seconds_.has_value() ? manual_eta_seconds_.value() : computeETASeconds();

            std::string eta_str = formatETA(eta);

            if (!eta_str.empty()) {
                std::cout << " | ETA: " << eta_str;
            }
        }

        if (options_.items_per_second) {
            std::cout << " | " << std::fixed << std::setprecision(2) << computeRate() << " it/s";
        }

        std::cout << std::flush;
    }

private:
    size_t total_;
    size_t value_;
    size_t width_;
    Theme theme_;
    Options options_;
    std::string label_;

    std::chrono::steady_clock::time_point start_time_ = std::chrono::steady_clock::now();

    std::optional<double> manual_eta_seconds_;
    mutable double smoothed_eta_ = -1.0;

    std::function<std::string(const ProgressBar&)> formatter_;

    std::string repeat(char c, size_t count) const {
        std::string(count, c);
    }

    std::string buildRawBar(size_t filled) const {
        return repeat(theme_.fill, filled) + repeat(theme_.empty, width_ - filled);
    }

    void overlayPercent(std::string& bar, int percent) const {
        if (!options_.progress_in_bar) return;

        std::string percent_str = std::to_string(percent) + '%';

        if (percent_str.size() > bar.size()) return;

        size_t start = (bar.size() - percent_str.size()) / 2;

        for (size_t i = 0; i < percent_str.size(); ++i) {
            bar[start + i] = percent_str[i];
        }
    }

    std::string applyColor(const std::string& raw) const {
        std::string out;

        size_t filled = std::count(raw.begin(), raw.end(), theme_.fill);

        if (!theme_.fill_color.empty())
            out += theme_.fill_color;

        for (size_t i = 0; i < filled; ++i)
            out += theme_.fill;

        if (!theme_.fill_color.empty())
            out += theme_.reset_color;

        if (!theme_.empty_color.empty())
            out += theme_.empty_color;

        for (size_t i = filled; i < raw.size(); ++i)
            out += theme_.empty;

        if (!theme_.reset_color.empty())
            out += theme_.reset_color;

        return out;
    }

    /* std::string buildBar(size_t filled) const {
        std::string bar =
            theme_.fill_color + std::string(filled, theme_.fill) + theme_.reset_color +
            theme_.empty_color + std::string(width_ - filled, theme_.empty) + theme_.reset_color;

        return bar;
    } */

    double computeETASeconds() const {
        if (value_ == 0) return -1.0;

        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start_time_).count();

        double ratio = static_cast<double>(value_) / total_;
        if (ratio <= 0.0) return -1.0;

        double total_estimated = elapsed / ratio;
        double eta = total_estimated - elapsed;

        if (smoothed_eta_ < 0) {
            smoothed_eta_ = eta;
        } else {
            smoothed_eta_ = 0.8 * smoothed_eta_ + 0.2 * eta;
        }

        return smoothed_eta_;
    }

    std::string formatETA(double seconds) const {
        if (seconds < 0) return "";

        int h = static_cast<int>(seconds) / 3600;
        int m = (static_cast<int>(seconds) % 3600) / 60;
        int s = static_cast<int>(seconds) % 60;

        std::ostringstream oss;

        if (h > 0) {
            oss << h << "h ";
        }

        if (m > 0 || h > 0) {
            oss << m << "m ";
        }

        oss << s << "s";

        return oss.str();
    }

    double computeRate() const {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start_time_).count();
        return elapsed > 0 ? value_ / elapsed : 0.0;
    }
};

class Table {
public:
    enum class Align {
        Left,
        Center,
        Right
    };

    struct Theme {
        std::string horizontal = "-";
        std::string vertical = "|";
        std::string corner = "+";
        std::string padding = " ";

        bool header_separator = true;
    };

    struct Themes {
        static Theme classic() {
            return {};
        }

        static Theme minimal() {
            return {
                .horizontal = "",
                .vertical = " ",
                .corner = "",
                .padding = " "
            };
        }
    };

    enum class Overflow {
        Wrap,
        Truncate,
        Ellipsis
    };

    struct ColumnConfig {
        size_t min_width = 0;
        size_t max_width = SIZE_MAX;
        Align align = Align::Left;
        Overflow overflow = Overflow::Wrap;
    };

    Table(Theme theme = Themes::classic()) : theme_(theme) {}

    Table& setHeaders(const std::vector<std::string>& headers) {
        headers_ = headers;
        aligns_.resize(headers.size(), Align::Left);
        columns_.resize(headers.size());
        return *this;
    }

    Table& addRow(const std::vector<std::string>& row) {
        rows_.push_back(row);
        return *this;
    }

    Table& setAlign(size_t col, Align align) {
        if (col < aligns_.size()) aligns_[col] = align;
        return *this;
    }

    Table& setColumn(size_t col, ColumnConfig config) {
        if (col < aligns_.size()) columns_[col] = config;
    }

    void render() const {
        auto widths = computeWidths();

        printSeparator(widths);
        printRow(headers_, widths);

        if (theme_.header_separator) {
            printSeparator(widths);
        }

        for (const auto& row : rows_) {
            printRow(row, widths);
        }

        printSeparator(widths);
    }

private:
    Theme theme_;
    std::vector<std::string> headers_;
    std::vector<std::vector<std::string>> rows_;
    std::vector<Align> aligns_;
    std::vector<ColumnConfig> columns_;

    size_t visibleLength(const std::string& s) const {
        size_t len = 0;
        bool in_escape = false;

        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\033') {
                in_escape = true;
            } else if (in_escape && s[i] == 'm') {
                in_escape = false;
            } else if (!in_escape) {
                ++len;
            }
        }

        return len;
    }

    std::vector<size_t> computeWidths() const {
        std::vector<size_t> widths(headers_.size(), 0);

        for (size_t i = 0; i < headers_.size(); ++i) {
            widths[i] = visibleLength(headers_[i]);
        }

        for (const auto& row : rows_) {
            for (size_t i = 0; i < row.size(); ++i) {
                widths[i] = std::max(widths[i], visibleLength(row[i]));
            }
        }

        for (size_t i = 0; i < widths.size(); ++i) {
            widths[i] = std::max(widths[i], columns_[i].min_width);
            widths[i] = std::min(widths[i], columns_[i].max_width);
        }

        return widths;
    }

    std::string pad(const std::string& text, size_t width, Align align) const {
        if (text.size() >= width) return text;

        size_t space = width - text.size();

        switch (align) {
            case Align::Left:
                return text + std::string(space, ' ');
            case Align::Right:
                return std::string(space, ' ') + text;
            case Align::Center: {
                size_t left = space / 2;
                size_t right = space - left;
                return std::string(left, ' ') + text + std::string(right, ' ');
            }
        }

        return text;
    }

    void printSeparator(const std::vector<size_t>& widths) const {
        std::cout << theme_.corner;

        for (size_t w : widths) {
            std::cout << std::string(w + 2, theme_.horizontal.empty() ? ' ' : theme_.horizontal[0]) << theme_.corner;
        }

        std::cout << "\n";
    }

    void printRow(const std::vector<std::string>& row, const std::vector<size_t>& widths) const {
        std::vector<std::vector<std::string>> wrapped(widths.size());

        size_t max_lines = 0;

        for (size_t i = 0; i < widths.size(); ++i) {
            std::string cell = (i < row.size()) ? row[i] : "";

            if (columns_[i].overflow == Overflow::Wrap) {
                wrapped[i] = wrapCell(cell, widths[i]);
            } else {
                wrapped[i] = {truncate(cell, widths[i], columns_[i].overflow)};
            }

            max_lines = std::max(max_lines, wrapped[i].size());
        }

        for (size_t line = 0; line < max_lines; ++line) {
            std::cout << theme_.vertical;

            for (size_t col = 0; col < widths.size(); ++col) {
                std::string content = (line < wrapped[col].size()) ? wrapped[col][line] : "";
                std::string padded = pad(content, widths[col], columns_[col].align);

                std::cout << theme_.padding
                          << padded
                          << theme_.padding
                          << theme_.vertical;
            }

            std::cout << "\n";
        }
    }

    std::string truncate(const std::string& text, size_t width, Overflow mode) const {
        if (visibleLength(text) <= width) return text;

        if (mode == Overflow::Truncate) {
            return text.substr(0, width);
        }

        if (mode == Overflow::Ellipsis && width >= 3) {
            return text.substr(0, width - 3) + "...";
        }

        return text.substr(0, width);
    }

    std::vector<std::string> wrapCell(const std::string& text, size_t width) const {
        std::vector<std::string> lines;

        if (width == 0) {
            lines.push_back("");
            return lines;
        }

        std::istringstream iss(text);
        std::string word;
        std::string line;

        while (iss >> word) {
            if (visibleLength(line + " " + word) > width) {
                if (!line.empty()) lines.push_back(line);
                line = word;
            } else {
                if (!line.empty()) line += " ";
                line += word;
            }
        }

        if (!line.empty()) lines.push_back(line);
        if (lines.empty()) lines.push_back("");

        return lines;
    }
};

} // namespace ui

} // namespace linea
