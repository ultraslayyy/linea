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
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <variant>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

/**
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

#define REGISTER_COMMAND_3(ClassName, Name, Desc) \
    static linea::ControllerRegistrar<ClassName> _registrar_##ClassName(Name, Desc)

#define REGISTER_COMMAND_2(ClassName, Name) \
    static linea::ControllerRegistrar<ClassName> _registrar_##ClassName(Name)

#define REGISTER_COMMAND_1(ClassName) \
    static linea::ControllerRegistrar<ClassName> _registrar_##ClassName

#define GET_MACRO(_1,_2,_3,NAME,...) NAME

#define REGISTER_COMMAND(...) \
    GET_MACRO(__VA_ARGS__, REGISTER_COMMAND_3, REGISTER_COMMAND_2, REGISTER_COMMAND_1)(__VA_ARGS__)

namespace linea {

namespace ANSI {

namespace __detail {

inline std::string u8(unsigned v) {
    return std::to_string(v > 255 ? 255 : v);
}

inline std::string rgbConstruct(unsigned r, unsigned g, unsigned b) {
    std::string s;
    s.reserve(12); // 255;255;255
    s += u8(r);
    s += ';';
    s += u8(g);
    s += ';';
    s += u8(b);
    return s;
}

inline std::string oscTerminator(bool st) {
    return st ? "\033\\" : "\a";
}

inline std::string oscCommand(const std::string& body, bool st = false) {
    return "\033]" + body + oscTerminator(st);
}

}

// C Escaped C0 control codes (ASCII control codes, originally ANSI X3.4, so it counts)
inline constexpr char NUL = '\0';
inline constexpr char BEL = '\a';
inline constexpr char BS  = '\b';
inline constexpr char HT  = '\t';
inline constexpr char LF  = '\n';
inline constexpr char BT  = '\v';
inline constexpr char FF  = '\f';
inline constexpr char CR  = '\r';
inline constexpr char ESC = '\033'; // Also \e (GCC only), \033 is octal, \x1b is hexadecimal (\033 is more reliable and supported)
inline constexpr char SP = ' '; // Yes, 'Space'. Not official C0 control character, but close to

inline constexpr char CRLF[] = "\r\n";

inline constexpr char SS2[]  = "\033N";  // Single Shift Two
inline constexpr char LS2[]  = "\033n";  // Lock Shift Two
inline constexpr char SS3[]  = "\033O";  // Single Shift Three
inline constexpr char LS3[]  = "\033o";  // Lock Shift Three
inline constexpr char DCS[]  = "\033P";  // Device Control String
inline constexpr char CSI[]  = "\033[";  // Control Sequence Introducer
inline constexpr char ST[]   = "\033\\"; // String Terminator
inline constexpr char OSC[]  = "\033]";  // Operating System Command
inline constexpr char SOS[]  = "\033X";  // Start of String
inline constexpr char PM[]   = "\033^";  // Privacy Message
inline constexpr char APC[]  = "\033_";  // Application Program Command
inline constexpr char LS1R[] = "\033~";

inline constexpr char COLUMNS_INSERT[]   = "\033['}";
inline constexpr char COLUMNS_DELETE[]   = "\033['~";

inline constexpr char REPEAT_LAST_CHAR[] = "\033[b";

inline constexpr char CURSOR_MOVE_HOME[]       = "\033[H";

inline std::string HORIZONTAL_VERTICAL_POSITION(int l, int c) { return "\033[" + std::to_string(l) + ";" + std::to_string(c) + "f"; }

inline std::string CURSOR_MOVE(int l, int c)           { return "\033[" + std::to_string(l) + ";" + std::to_string(c) + "H"; }
inline std::string CUSOR_MOVE_ROW(int v)               { return "\033[" + std::to_string(v) + "d"; }
inline std::string CURSOR_MOVE_UP(int v)               { return "\033[" + std::to_string(v) + "A"; }
inline std::string CURSOR_MOVE_DOWN(int v)             { return "\033[" + std::to_string(v) + "B"; }
inline std::string CURSOR_MODE_DOWN_ROWS(int v)        { return "\033[" + std::to_string(v) + "e"; }
inline std::string CURSOR_MOVE_RIGHT(int v)            { return "\033[" + std::to_string(v) + "C"; }
inline std::string CURSOR_MOVE_LEFT(int v)             { return "\033[" + std::to_string(v) + "D"; }
inline std::string CURSOR_MOVE_START_LINE_DOWN(int v)  { return "\033[" + std::to_string(v) + "E"; }
inline std::string CURSOR_MOVE_START_LINE_UP(int v)    { return "\033[" + std::to_string(v) + "F"; }
inline std::string CURSOR_MOVE_COLUMN(int v)           { return "\033[" + std::to_string(v) + "G"; }

inline constexpr char CURSOR_REQUEST_POS[]     = "\033[6n";
inline constexpr char CURSOR_REVERSE_INDEX[]   = "\033M";
inline constexpr char CURSOR_SAVE_POS_DEC[]    = "\0337";
inline constexpr char CURSOR_RESTORE_POS_DEC[] = "\0338";
inline constexpr char CURSOR_SAVE_POS_SCO[]    = "\033[s";
inline constexpr char CURSOR_RESTORE_POS_SCO[] = "\033[u";

inline constexpr char TAB_STOP_SET_COL[]       = "\033H";
inline constexpr char TAB_STOP_CLEAR_COL[]     = "\033[g";
inline constexpr char TAB_STOP_CLEAR_ALL[]     = "\033[3g";
inline constexpr char TAB_STOP_MOVE_TO_NEXT[]  = "\t";

inline constexpr char DEVICE_STATUS_REPORT[]        = "\033[6n";
inline constexpr char DEVICE_PRIMARY_ATTRIBUTES[]   = "\033[c";
inline constexpr char DEVICE_SECONDARY_ATTRIBUTES[] = "\033[>c";

inline constexpr char TERMINAL_REQUEST_ID[]         = "\033[0c";
inline constexpr char TERMINAL_RESET_SOFT[]         = "\033[!p";
inline constexpr char TERMINAL_RESET_FULL[]         = "\033c";

inline constexpr char AUX_PORT_ON[]                 = "\033[5i";
inline constexpr char AUX_PORT_OFF[]                = "\033[4i";

inline constexpr char ERASE_IN_DISPLAY[]            = "\033[J";
inline constexpr char ERASE_FROM_CURSOR_TO_SCREEN[] = "\033[0J";
inline constexpr char ERASE_TO_CURSOR_FROM_SCREEN[] = "\033[1J";
inline constexpr char ERASE_SCREEN[]                = "\033[2J";
inline constexpr char ERASE_SAVED_LINES[]           = "\033[3J";
inline constexpr char ERASE_FROM_CURSOR_TO_LINE[]   = "\033[K";
inline constexpr char ERASE_TO_CURSOR_FROM_LINE[]   = "\033[1K";
inline constexpr char ERASE_LINE[]                  = "\033[2K";

inline constexpr char CLEAR_SCREEN_AND_HOME[]       = "\033[2J\033[H"; // Common helper to do both

inline constexpr char SCROLL_UP_ONE[]                        = "\033[S";
inline constexpr char SCROLL_DOWN_ONE[]                      = "\033[T";
inline constexpr char RESET_SCROLL_REGION[]                  = "\033[r";

inline std::string    SCROLL_UP(int n)                { return "\033[" + std::to_string(n) + "S"; }
inline std::string    SCROLL_DOWN(int n)              { return "\033[" + std::to_string(n) + "T"; }
inline std::string    SET_SCROLL_REGION(int t, int b) { return "\033[" + std::to_string(t) + ";" + std::to_string(b) + "r"; }
inline std::string    INSERT_CHARS(int n)             { return "\033[" + std::to_string(n) + "@"; }
inline std::string    DELETE_CHARS(int n)             { return "\033[" + std::to_string(n) + "P"; }
inline std::string    INSERT_LINES(int n)             { return "\033[" + std::to_string(n) + "L"; }
inline std::string    DELETE_LINES(int n)             { return "\033[" + std::to_string(n) + "M"; }

inline constexpr char RESET[]        = "\033[0m"; // 'CSI m' also works, not just 'CSI 0 m'
inline constexpr char BOLD[]         = "\033[1m";
inline constexpr char DIM[]          = "\033[2m";
inline constexpr char ITALIC[]       = "\033[3m";
inline constexpr char UNDERLINE[]    = "\033[4m";
inline constexpr char BLINKING[]     = "\033[5m";
inline constexpr char REVERSE[]      = "\033[7m";
inline constexpr char INVISIBLE[]    = "\033[8m";
inline constexpr char STRIKETHOUGH[] = "\033[9m";

inline constexpr char FONT_PRIMARY[] = "\033[10m";
// Limited support
inline constexpr char FONT_FRAKTUR[] = "\033[20m";

// Disables BOLD on some terminals
inline constexpr char DOUBLE_UNDERLINE[]   = "\033[21m";

inline constexpr char BOLD_RESET[]         = "\033[22m";
inline constexpr char DIM_RESET[]          = "\033[22m";
inline constexpr char ITALIC_RESET[]       = "\033[23m";
inline constexpr char UNDERLINE_RESET[]    = "\033[24m";
inline constexpr char BLINKING_RESET[]     = "\033[25m";
inline constexpr char REVERSE_RESET[]      = "\033[27m";
inline constexpr char INVISIBLE_RESET[]    = "\033[28m";
inline constexpr char STRIKETHOUGH_RESET[] = "\033[29m";

// ITU T.61 and T.416, not known to be used on terminals
inline constexpr char PROPORTIONAL_SPACING[] = "\033[26m";

inline constexpr char FG_BLACK[]   = "\033[30m";
inline constexpr char FG_RED[]     = "\033[31m";
inline constexpr char FG_GREEN[]   = "\033[32m";
inline constexpr char FG_YELLOW[]  = "\033[33m";
inline constexpr char FG_BLUE[]    = "\033[34m";
inline constexpr char FG_MAGENTA[] = "\033[35m";
inline constexpr char FG_CYAN[]    = "\033[36m";
inline constexpr char FG_WHITE[]   = "\033[37m";
inline constexpr char FG_DEFAULT[] = "\033[39m";

inline std::string FG_CUSTOM_256(unsigned v)                         { return "\033[38;5;" + __detail::u8(v) + "m"; }
inline std::string FG_CUSTOM_RGB(unsigned r, unsigned g, unsigned b) { return "\033[38;2;" + __detail::rgbConstruct(r,g,b) + "m"; }

inline constexpr char BG_BLACK[]   = "\033[40m";
inline constexpr char BG_RED[]     = "\033[41m";
inline constexpr char BG_GREEN[]   = "\033[42m";
inline constexpr char BG_YELLOW[]  = "\033[43m";
inline constexpr char BG_BLUE[]    = "\033[44m";
inline constexpr char BG_MAGENTA[] = "\033[45m";
inline constexpr char BG_CYAN[]    = "\033[46m";
inline constexpr char BG_WHITE[]   = "\033[47m";
inline constexpr char BG_DEFAULT[] = "\033[49m";

inline std::string BG_CUSTOM_256(unsigned v)                         { return "\033[48;5;" + __detail::u8(v) + "m"; }
inline std::string BG_CUSTOM_RGB(unsigned r, unsigned g, unsigned b) { return "\033[48;2;" + __detail::rgbConstruct(r,g,b) + "m"; }

inline constexpr char FRAMED[]               = "\033[51m";
inline constexpr char ENCIRCLED[]            = "\033[52m";
inline constexpr char OVERLINE[]             = "\033[53m";
inline constexpr char FRAMED_ENCIRCLED_OFF[] = "\033[54m";
inline constexpr char OVERLINE_OFF[]         = "\033[55m";

// Not in standard; implemented in Kitty, VTE, mintty, and iTerm2
inline std::string UNDERLINE_COLOR_256(unsigned v)                         { return "\033[58;5;" + __detail::u8(v) + "m"; }
inline std::string UNDERLINE_COLOR_RGB(unsigned r, unsigned g, unsigned b) { return "\033[58;2;" + __detail::rgbConstruct(r,g,b) + "m"; }

inline constexpr char UNDERLINE_COLOR_DEFAULT[] = "\033[59m";

// Rarely supported.
// Underline doubles as 'or right side line', overline being left side
// Double of these are double side lines
inline constexpr char IDEOGRAM_UNDERLINE[]        = "\033[60m";
inline constexpr char IDEOGRAM_DOUBLE_UNDERLINE[] = "\033[61m";
inline constexpr char IDEOGRAM_OVERLINE[]         = "\033[62m";
inline constexpr char IDEOGRAM_DOUBLE_OVERLINE[]  = "\033[63m";
inline constexpr char IDEOGRAM_STRESS_MARKING[]   = "\033[64m";
inline constexpr char IDEOGRAM_RESET[]            = "\033[65m";

// Only in mintty
inline constexpr char SUPERSCRIPT[]       = "\033[73m";
inline constexpr char SUBSCRIPT[]         = "\033[74m";
inline constexpr char SUPERSCRIPT_RESET[] = "\033[75m"; // Included twice for ergo
inline constexpr char SUBSCRIPT_RESET[]   = "\033[75m";

// Technically these and bright_ variants are not in standard,
// but often supported (originally by aixterm).
inline constexpr char FG_BLACK_BRIGHT[]   = "\033[90m";
inline constexpr char FG_RED_BRIGHT[]     = "\033[91m";
inline constexpr char FG_GREEN_BRIGHT[]   = "\033[92m";
inline constexpr char FG_YELLOW_BRIGHT[]  = "\033[93m";
inline constexpr char FG_BLUE_BRIGHT[]    = "\033[94m";
inline constexpr char FG_MAGENTA_BRIGHT[] = "\033[95m";
inline constexpr char FG_CYAN_BRIGHT[]    = "\033[96m";
inline constexpr char FG_WHITE_BRIGHT[]   = "\033[97m";

inline constexpr char BG_BLACK_BRIGHT[]   = "\033[100m";
inline constexpr char BG_RED_BRIGHT[]     = "\033[101m";
inline constexpr char BG_GREEN_BRIGHT[]   = "\033[102m";
inline constexpr char BG_YELLOW_BRIGHT[]  = "\033[103m";
inline constexpr char BG_BLUE_BRIGHT[]    = "\033[104m";
inline constexpr char BG_MAGENTA_BRIGHT[] = "\033[105m";
inline constexpr char BG_CYAN_BRIGHT[]    = "\033[106m";
inline constexpr char BG_WHITE_BRIGHT[]   = "\033[107m";

// Legacy modes (mostly unsupported now)
inline constexpr char MODE_40X25_MONO[]       = "\033[=0h";
inline constexpr char MODE_40X25_COLOR[]      = "\033[=1h";
inline constexpr char MODE_80X25_MONO[]       = "\033[=2h";
inline constexpr char MODE_80X25_COLOR[]      = "\033[=3h";
inline constexpr char MODE_320X200_4COLOR[]   = "\033[=4h";
inline constexpr char MODE_320X200_MONO[]     = "\033[=5h";
inline constexpr char MODE_640X200_MONO[]     = "\033[=6h";
inline constexpr char MODE_LINE_WRAPPING[]    = "\033[=7h";
inline constexpr char MODE_320X200_COLOR[]    = "\033[=13h";
inline constexpr char MODE_640X200_16COLOR[]  = "\033[=14h";
inline constexpr char MODE_640X350_2MONO[]    = "\033[=15h";
inline constexpr char MODE_640X350_16COLOR[]  = "\033[=16h";
inline constexpr char MODE_640X480_2MONO[]    = "\033[=17h";
inline constexpr char MODE_640X480_16COLOR[]  = "\033[=18h";
inline constexpr char MODE_320X200_256COLOR[] = "\033[=19h";

inline constexpr char MODE_40X25_MONO_RESET[]       = "\033[=0l";
inline constexpr char MODE_40X25_COLOR_RESET[]      = "\033[=1l";
inline constexpr char MODE_80X25_MONO_RESET[]       = "\033[=2l";
inline constexpr char MODE_80X25_COLOR_RESET[]      = "\033[=3l";
inline constexpr char MODE_320X200_4COLOR_RESET[]   = "\033[=4l";
inline constexpr char MODE_320X200_MONO_RESET[]     = "\033[=5l";
inline constexpr char MODE_640X200_MONO_RESET[]     = "\033[=6l";
inline constexpr char MODE_LINE_WRAPPING_RESET[]    = "\033[=7l";
inline constexpr char MODE_320X200_COLOR_RESET[]    = "\033[=13l";
inline constexpr char MODE_640X200_16COLOR_RESET[]  = "\033[=14l";
inline constexpr char MODE_640X350_2MONO_RESET[]    = "\033[=15l";
inline constexpr char MODE_640X350_16COLOR_RESET[]  = "\033[=16l";
inline constexpr char MODE_640X480_2MONO_RESET[]    = "\033[=17l";
inline constexpr char MODE_640X480_16COLOR_RESET[]  = "\033[=18l";
inline constexpr char MODE_320X200_256COLOR_RESET[] = "\033[=19l";

inline constexpr char CURSOR_STYLE_SET[]                = "\033[ q";
inline constexpr char CURSOR_STYLE_BLINKING_BLOCK[]     = "\033[1 q";
inline constexpr char CURSOR_STYLE_STEADY_BLOCK[]       = "\033[2 q";
inline constexpr char CURSOR_STYLE_BLINKING_UNDERLINE[] = "\033[3 q";
inline constexpr char CURSOR_STYLE_STEADY_UNDERLINE[]   = "\033[4 q";
inline constexpr char CURSOR_STYLE_BLINKING_BAR[]       = "\033[5 q";
inline constexpr char CURSOR_STYLE_STEADY_BAR[]         = "\033[6 q";

/**
 * @brief
 * Line drawing. e.g.,
 * ```
 * lqk
 * x x
 * mqj
 * ```
 * Becomes a box with 'ESC ( 0' active
 */
inline constexpr char CHAR_SET_LINE_DRAWING[] = "\033(0";
inline constexpr char CHAR_SET_ASCII[]        = "\033(B";

// "While these modes may be supported by the most terminals, some may not work in multiplexers like tmux."
// PMODE = Private Mode
inline constexpr char PMODE_CURSOR_KEYS_APPLICATION[]      = "\033[?1h";
inline constexpr char PMODE_CURSOR_KEYS_NORMAL[]           = "\033[?1l";
inline constexpr char PMODE_WRAP_ENABLE[]                  = "\033[?7h";
inline constexpr char PMODE_WRAP_DISABLE[]                 = "\033[?7l";
inline constexpr char PMODE_CURSOR_BLINKING[]              = "\033[?12h";
inline constexpr char PMODE_CURSOR_STEADY[]                = "\033[?12l";
inline constexpr char PMODE_CURSOR_VISIBLE[]               = "\033[?25h";
inline constexpr char PMODE_CURSOR_INVISIBLE[]             = "\033[?25l";
inline constexpr char PMODE_SAVE_SCREEN[]                  = "\033[?47h";
inline constexpr char PMODE_RESTORE_SCREEN[]               = "\033[?47l";
inline constexpr char PMODE_MOUSE_ENABLE_BASIC[]           = "\033[?1000h";
inline constexpr char PMODE_MOUSE_DISABLE[]                = "\033[?1000l";
inline constexpr char PMODE_MOUSE_ENABLE_BUTTON_EVENT[]    = "\033[?1002h";
inline constexpr char PMODE_MOUSE_DISABLE_BUTTON_EVENT[]   = "\033[?1002l";
inline constexpr char PMODE_MOUSE_ENABLE_BUTTON_DRAG[]     = "\033[?1003h";
inline constexpr char PMODE_MOUSE_DISABLE_BUTTON_DRAG[]    = "\033[?1003l";
inline constexpr char PMODE_ENABLE_REPORTING_FOCUS[]       = "\033[?1004h";
inline constexpr char PMODE_DISABLE_REPORTING_FOCUS[]      = "\033[?1004l";
inline constexpr char PMODE_MOUSE_MODE_UTF8[]              = "\033[?1005h";
inline constexpr char PMODE_SGR_EXTENDED_MODE[]            = "\033[?1006h";
inline constexpr char PMODE_MOUSE_ENABLE_URXVT_EXTENDED[]  = "\033[?1015h";
inline constexpr char PMODE_MOUSE_DISABLE_URXVT_EXTENDED[] = "\033[?1015l";
inline constexpr char PMODE_ENABLE_ALT_BUFFER_VAR[]        = "\033[?1047h";
inline constexpr char PMODE_SAVE_CURSOR[]                  = "\033[?1048h";
inline constexpr char PMODE_RESTORE_CURSOR[]               = "\033[?1048l";
inline constexpr char PMODE_ENABLE_ALT_BUFFER[]            = "\033[?1049h";
inline constexpr char PMODE_DISABLE_ALT_BUFFER[]           = "\033[?1049l";
inline constexpr char PMODE_BRACKETED_PASTE_ON[]           = "\033[?2004h";
inline constexpr char PMODE_BRACKETED_PASTE_OFF[]          = "\033[?2004l"; // Disables ESC [200~ and ESC [201~ surrounding paste

// 0Ft Sequences (ESC followed by SP (0x22, space))
// Not common in the slightest, basically dead
inline constexpr char ANNOUNCE_CODE_STRUCTURE_6[]      = "\033 F";
inline constexpr char SEND_7BIT_C1_CONTROL_CHARACTER[] = "\033 F";
inline constexpr char ANNOUNCE_CODE_STRUCTURE_7[]      = "\033 G";
inline constexpr char SEND_8BIT_C1_CONTROL_CHARACTER[] = "\033 G";

// 3Fp (private-use) sequences (common but not guaranteed)
inline constexpr char DOUBLE_HEIGHT_TOP[]     = "\033#3";
inline constexpr char DOUBLE_HEIGHT_BOTTOM[]  = "\033#4";
inline constexpr char SINGLE_WIDTH[]          = "\033#5";
inline constexpr char DOUBLE_WIDTH[]          = "\033#6";
inline constexpr char SCREEN_ALIGNMENT_TEST[] = "\033#8";

// OSC
inline std::string OSC_SET_TITLE(const std::string& title) {
    return "\033]0;" + title + "\007";
}
inline std::string OSC_SET_PALETTE_COLOR(int i, unsigned r, unsigned g, unsigned b, bool st = false) {
    return __detail::oscCommand("4;" + std::to_string(i) + ";rgb:" + __detail::u8(r) + "/" + __detail::u8(g) + "/" + __detail::u8(b), st);
}
inline std::string OSC_SET_WORKING_DIRECTORY(const std::string& host, const std::string& path, bool st = false) {
    return __detail::oscCommand("7;file://" + host + path, st);
}
inline std::string OSC_HYPERLINK(const std::string& url, const std::string& text, const std::string& id = "", bool st = false) {
    std::string params;
    if (!id.empty()) params = "id=" + id;
    
    return __detail::oscCommand("8;" + params + ";" + url + __detail::oscTerminator(st) + text + "\033]8;;", st);
}
inline std::string OSC_NOTIFY(const std::string& message, bool st = false) {
    return __detail::oscCommand("9;" + message, st);
}
inline std::string OSC_SET_DEFAULT_FG_RGB(unsigned r, unsigned g, unsigned b, bool st = false) {
    return __detail::oscCommand("10;rgb:" + __detail::u8(r) + "/" + __detail::u8(g) + "/" + __detail::u8(b), st);
}
inline std::string OSC_QUERY_DEFAULT_FG(bool st = false) {
    return __detail::oscCommand("10;?", st);
}
inline std::string OSC_SET_DEFAULT_BG_RGB(unsigned r, unsigned g, unsigned b, bool st = false) {
    return __detail::oscCommand("11;rgb:" + __detail::u8(r) + "/" + __detail::u8(g) + "/" + __detail::u8(b), st);
}
inline std::string OSC_QUERY_DEFAULT_BG(bool st = false) {
    return __detail::oscCommand("11;?", st);
}
inline std::string OSC_SET_CURSOR_COLOR_RGB(unsigned r, unsigned g, unsigned b, bool st = false) {
    return __detail::oscCommand("12;rgb:" + __detail::u8(r) + "/" + __detail::u8(g) + "/" + __detail::u8(b), st);
}
inline std::string OSC_QUERY_CURSOR_COLOR(bool st = false) {
    return __detail::oscCommand("12;?", st);
}
// Target = 'c' (clipboard), 'p'(primary, X11) or 's' (selection)
inline std::string OSC_SET_CLIPBOARD(const std::string& base64, char target = 'c', bool st = false) {
    return __detail::oscCommand("52;" + std::string(1, target) + ";" + base64, st);
}
inline constexpr char OSC_RESET_PALETTE[]      = "\033]104\033\\";
inline constexpr char OSC_RESET_DEFAULT_FG[]   = "\033]110\033\\";
inline constexpr char OSC_RESET_DEFAULT_BG[]   = "\033]111\033\\";
inline constexpr char OSC_RESET_CURSOR_COLOR[] = "\033]112\033\\";
inline constexpr char OSC_PROMPT_START[]       = "\033]133;A\007";
inline constexpr char OSC_PROMPT_END[]         = "\033]133;B\007";
inline constexpr char OSC_COMMAND_START[]      = "\033]133;C\007";
inline constexpr char OSC_COMMAND_END[]        = "\033]133;D\007";
inline std::string OSC_COMMAND_START_WITH_META(const std::string& meta, bool st = false) {
    return __detail::oscCommand("133;C;" + meta, st);
}
inline std::string OSC_COMMAND_DONE(int exitCode, bool st = false) {
    return __detail::oscCommand("133;D;" + std::to_string(exitCode), st);
}
inline std::string OSC_URXVT_NOTIFY(const std::string& message, bool st = false) {
    return __detail::oscCommand("777;notify;" + message, st);
}
inline std::string OSC_ITERM_SET_CWD(const std::string& path, bool st = false) {
    return __detail::oscCommand("1337;CurrentDir=" + path, st);
}
inline std::string OSC_ITERM_IMAGE(const std::string& base64, int width = 0, int height = 0, bool st = false) {
    std::string params = "inline=1";
    if (width > 0)  params += ";width="  + std::to_string(width);
    if (height > 0) params += ";height=" + std::to_string(height);
    return __detail::oscCommand("1337;File=" + params + ":" + base64, st);
}
inline std::string OSC_ITERM_SET_VAR(const std::string& name, const std::string& base64, bool st = false) {
    return __detail::oscCommand("1337;SetUserVar=" + name + "=" + base64, st);
}

// xterm extensions
inline constexpr char XTERM_WINDOW_GET_SIZE_PX[]    = "\033[14t";
inline constexpr char XTERM_TERMINAL_GET_SIZE_CHR[] = "\033[18t";

// Keyboard strings
namespace KBD {

namespace __detail {

class Item {
public:
    constexpr std::string_view value() const {
        return value_;
    }

private:
    std::string_view value_;
    constexpr Item(std::string_view v) : value_(v) {}

    friend constexpr Item make_item(std::string_view v);
};

inline constexpr Item make_item(std::string_view v) {
    return Item(v);
}

} // namespace __detail

constexpr __detail::Item F1                         = __detail::make_item("0;59");
constexpr __detail::Item F2                         = __detail::make_item("0;60");
constexpr __detail::Item F3                         = __detail::make_item("0;61");
constexpr __detail::Item F4                         = __detail::make_item("0;62");
constexpr __detail::Item F5                         = __detail::make_item("0;63");
constexpr __detail::Item F6                         = __detail::make_item("0;64");
constexpr __detail::Item F7                         = __detail::make_item("0;65");
constexpr __detail::Item F8                         = __detail::make_item("0;66");
constexpr __detail::Item F9                         = __detail::make_item("0;67");
constexpr __detail::Item F10                        = __detail::make_item("0;68");
constexpr __detail::Item F11                        = __detail::make_item("0;133");
constexpr __detail::Item F12                        = __detail::make_item("0;134");
constexpr __detail::Item HOME_NUMPAD                = __detail::make_item("0;71");
constexpr __detail::Item UP_NUMPAD                  = __detail::make_item("0;72");
constexpr __detail::Item PAGE_UP_NUMPAD             = __detail::make_item("0;73");
constexpr __detail::Item LEFT_NUMPAD                = __detail::make_item("0;75");
constexpr __detail::Item RIGHT_NUMPAD               = __detail::make_item("0;77");
constexpr __detail::Item END_NUMPAD                 = __detail::make_item("0;79");
constexpr __detail::Item DOWN_NUMPAD                = __detail::make_item("0;80");
constexpr __detail::Item PAGE_DOWN_NUMPAD           = __detail::make_item("0;81");
constexpr __detail::Item INSERT_NUMPAD              = __detail::make_item("0;82");
constexpr __detail::Item DELETE_NUMPAD              = __detail::make_item("0;83");
constexpr __detail::Item HOME                       = __detail::make_item("224;71");
constexpr __detail::Item UP                         = __detail::make_item("224;72");
constexpr __detail::Item PAGE_UP                    = __detail::make_item("224;73");
constexpr __detail::Item LEFT                       = __detail::make_item("224;75");
constexpr __detail::Item RIGHT                      = __detail::make_item("224;77");
constexpr __detail::Item END                        = __detail::make_item("224;79");
constexpr __detail::Item DOWN                       = __detail::make_item("224;80");
constexpr __detail::Item PAGE_DOWN                  = __detail::make_item("224;81");
constexpr __detail::Item INSERT                     = __detail::make_item("224;82");
constexpr __detail::Item KDELETE                    = __detail::make_item("224;83");
constexpr __detail::Item BACKSPACE                  = __detail::make_item("8");
constexpr __detail::Item ENTER                      = __detail::make_item("13");
constexpr __detail::Item TAB                        = __detail::make_item("9");
constexpr __detail::Item KNULL                      = __detail::make_item("0;3");
constexpr __detail::Item A                          = __detail::make_item("97");
constexpr __detail::Item B                          = __detail::make_item("98");
constexpr __detail::Item C                          = __detail::make_item("99");
constexpr __detail::Item D                          = __detail::make_item("100");
constexpr __detail::Item E                          = __detail::make_item("101");
constexpr __detail::Item F                          = __detail::make_item("102");
constexpr __detail::Item G                          = __detail::make_item("103");
constexpr __detail::Item H                          = __detail::make_item("104");
constexpr __detail::Item I                          = __detail::make_item("105");
constexpr __detail::Item J                          = __detail::make_item("106");
constexpr __detail::Item K                          = __detail::make_item("107");
constexpr __detail::Item L                          = __detail::make_item("108");
constexpr __detail::Item M                          = __detail::make_item("109");
constexpr __detail::Item N                          = __detail::make_item("110");
constexpr __detail::Item O                          = __detail::make_item("111");
constexpr __detail::Item P                          = __detail::make_item("112");
constexpr __detail::Item Q                          = __detail::make_item("113");
constexpr __detail::Item R                          = __detail::make_item("114");
constexpr __detail::Item S                          = __detail::make_item("115");
constexpr __detail::Item T                          = __detail::make_item("116");
constexpr __detail::Item U                          = __detail::make_item("117");
constexpr __detail::Item V                          = __detail::make_item("118");
constexpr __detail::Item W                          = __detail::make_item("119");
constexpr __detail::Item X                          = __detail::make_item("120");
constexpr __detail::Item Y                          = __detail::make_item("121");
constexpr __detail::Item Z                          = __detail::make_item("122");
constexpr __detail::Item K0                         = __detail::make_item("48");
constexpr __detail::Item K1                         = __detail::make_item("49");
constexpr __detail::Item K2                         = __detail::make_item("50");
constexpr __detail::Item K3                         = __detail::make_item("51");
constexpr __detail::Item K4                         = __detail::make_item("52");
constexpr __detail::Item K5                         = __detail::make_item("53");
constexpr __detail::Item K6                         = __detail::make_item("54");
constexpr __detail::Item K7                         = __detail::make_item("55");
constexpr __detail::Item K8                         = __detail::make_item("56");
constexpr __detail::Item K9                         = __detail::make_item("57");
constexpr __detail::Item HYPHEN                     = __detail::make_item("45");
constexpr __detail::Item EQUALS                     = __detail::make_item("61");
constexpr __detail::Item LEFT_SQUARE_BRACKET        = __detail::make_item("91");
constexpr __detail::Item RIGHT_SQUARE_BRACKET       = __detail::make_item("93");
constexpr __detail::Item BACKSLASH                  = __detail::make_item("92");
constexpr __detail::Item SEMICOLON                  = __detail::make_item("59");
constexpr __detail::Item APOSTROPHE                 = __detail::make_item("39");
constexpr __detail::Item COMMA                      = __detail::make_item("44");
constexpr __detail::Item PERIOD                     = __detail::make_item("46");
constexpr __detail::Item SLASH                      = __detail::make_item("47");
constexpr __detail::Item BACKTICK                   = __detail::make_item("96");
constexpr __detail::Item ENTER_NUMPAD               = __detail::make_item("13");
constexpr __detail::Item ASTERISK_NUMPAD            = __detail::make_item("42");
constexpr __detail::Item PLUS_NUMPAD                = __detail::make_item("43");
constexpr __detail::Item HYPHEN_NUMPAD              = __detail::make_item("45");
constexpr __detail::Item SLASH_NUMPAD               = __detail::make_item("47");
constexpr __detail::Item K5_NUMPAD                  = __detail::make_item("0;76");

constexpr __detail::Item SHIFT_F1                   = __detail::make_item("0;84");
constexpr __detail::Item SHIFT_F2                   = __detail::make_item("0;85");
constexpr __detail::Item SHIFT_F3                   = __detail::make_item("0;86");
constexpr __detail::Item SHIFT_F4                   = __detail::make_item("0;87");
constexpr __detail::Item SHIFT_F5                   = __detail::make_item("0;88");
constexpr __detail::Item SHIFT_F6                   = __detail::make_item("0;89");
constexpr __detail::Item SHIFT_F7                   = __detail::make_item("0;90");
constexpr __detail::Item SHIFT_F8                   = __detail::make_item("0;91");
constexpr __detail::Item SHIFT_F9                   = __detail::make_item("0;92");
constexpr __detail::Item SHIFT_F10                  = __detail::make_item("0;93");
constexpr __detail::Item SHIFT_F11                  = __detail::make_item("0;135");
constexpr __detail::Item SHIFT_F12                  = __detail::make_item("0;136");
constexpr __detail::Item SHIFT_HOME_NUMPAD          = __detail::make_item("55");
constexpr __detail::Item SHIFT_UP_NUMPAD            = __detail::make_item("56");
constexpr __detail::Item SHIFT_PAGE_UP_NUMPAD       = __detail::make_item("57");
constexpr __detail::Item SHIFT_LEFT_NUMPAD          = __detail::make_item("52");
constexpr __detail::Item SHIFT_RIGHT_NUMPAD         = __detail::make_item("54");
constexpr __detail::Item SHIFT_END_NUMPAD           = __detail::make_item("49");
constexpr __detail::Item SHIFT_DOWN_NUMPAD          = __detail::make_item("50");
constexpr __detail::Item SHIFT_PAGE_DOWN_NUMPAD     = __detail::make_item("51");
constexpr __detail::Item SHIFT_INSERT_NUMPAD        = __detail::make_item("48");
constexpr __detail::Item SHIFT_DELETE_NUMPAD        = __detail::make_item("46");
constexpr __detail::Item SHIFT_HOME                 = __detail::make_item("224;71");
constexpr __detail::Item SHIFT_UP                   = __detail::make_item("224;72");
constexpr __detail::Item SHIFT_PAGE_UP              = __detail::make_item("224;73");
constexpr __detail::Item SHIFT_LEFT                 = __detail::make_item("224;75");
constexpr __detail::Item SHIFT_RIGHT                = __detail::make_item("224;77");
constexpr __detail::Item SHIFT_END                  = __detail::make_item("224;79");
constexpr __detail::Item SHIFT_DOWN                 = __detail::make_item("224;80");
constexpr __detail::Item SHIFT_PAGE_DOWN            = __detail::make_item("224;81");
constexpr __detail::Item SHIFT_INSERT               = __detail::make_item("224;82");
constexpr __detail::Item SHIFT_KDELETE              = __detail::make_item("224;83");
constexpr __detail::Item SHIFT_BACKSPACE            = __detail::make_item("8");
constexpr __detail::Item SHIFT_TAB                  = __detail::make_item("0;15");
constexpr __detail::Item SHIFT_A                    = __detail::make_item("65");
constexpr __detail::Item SHIFT_B                    = __detail::make_item("66");
constexpr __detail::Item SHIFT_C                    = __detail::make_item("67");
constexpr __detail::Item SHIFT_D                    = __detail::make_item("68");
constexpr __detail::Item SHIFT_E                    = __detail::make_item("69");
constexpr __detail::Item SHIFT_F                    = __detail::make_item("70");
constexpr __detail::Item SHIFT_G                    = __detail::make_item("71");
constexpr __detail::Item SHIFT_H                    = __detail::make_item("72");
constexpr __detail::Item SHIFT_I                    = __detail::make_item("73");
constexpr __detail::Item SHIFT_J                    = __detail::make_item("74");
constexpr __detail::Item SHIFT_K                    = __detail::make_item("75");
constexpr __detail::Item SHIFT_L                    = __detail::make_item("76");
constexpr __detail::Item SHIFT_M                    = __detail::make_item("77");
constexpr __detail::Item SHIFT_N                    = __detail::make_item("78");
constexpr __detail::Item SHIFT_O                    = __detail::make_item("79");
constexpr __detail::Item SHIFT_P                    = __detail::make_item("80");
constexpr __detail::Item SHIFT_Q                    = __detail::make_item("81");
constexpr __detail::Item SHIFT_R                    = __detail::make_item("82");
constexpr __detail::Item SHIFT_S                    = __detail::make_item("83");
constexpr __detail::Item SHIFT_T                    = __detail::make_item("84");
constexpr __detail::Item SHIFT_U                    = __detail::make_item("85");
constexpr __detail::Item SHIFT_V                    = __detail::make_item("86");
constexpr __detail::Item SHIFT_W                    = __detail::make_item("87");
constexpr __detail::Item SHIFT_X                    = __detail::make_item("88");
constexpr __detail::Item SHIFT_Y                    = __detail::make_item("89");
constexpr __detail::Item SHIFT_Z                    = __detail::make_item("90");
constexpr __detail::Item SHIFT_K0                   = __detail::make_item("41");
constexpr __detail::Item SHIFT_K1                   = __detail::make_item("33");
constexpr __detail::Item SHIFT_K2                   = __detail::make_item("64");
constexpr __detail::Item SHIFT_K3                   = __detail::make_item("35");
constexpr __detail::Item SHIFT_K4                   = __detail::make_item("36");
constexpr __detail::Item SHIFT_K5                   = __detail::make_item("37");
constexpr __detail::Item SHIFT_K6                   = __detail::make_item("94");
constexpr __detail::Item SHIFT_K7                   = __detail::make_item("38");
constexpr __detail::Item SHIFT_K8                   = __detail::make_item("42");
constexpr __detail::Item SHIFT_K9                   = __detail::make_item("40");
constexpr __detail::Item SHIFT_HYPHEN               = __detail::make_item("95");
constexpr __detail::Item SHIFT_EQUALS               = __detail::make_item("43");
constexpr __detail::Item SHIFT_LEFT_SQUARE_BRACKET  = __detail::make_item("123");
constexpr __detail::Item SHIFT_RIGHT_SQUARE_BRACKET = __detail::make_item("125");
constexpr __detail::Item SHIFT_BACKSLASH            = __detail::make_item("124");
constexpr __detail::Item SHIFT_SEMICOLON            = __detail::make_item("58");
constexpr __detail::Item SHIFT_APOSTROPHE           = __detail::make_item("34");
constexpr __detail::Item SHIFT_COMMA                = __detail::make_item("60");
constexpr __detail::Item SHIFT_PERIOD               = __detail::make_item("62");
constexpr __detail::Item SHIFT_SLASH                = __detail::make_item("63");
constexpr __detail::Item SHIFT_BACKTICK             = __detail::make_item("126");
constexpr __detail::Item SHIFT_ASTERISK_NUMPAD      = __detail::make_item("0;144");
constexpr __detail::Item SHIFT_PLUS_NUMPAD          = __detail::make_item("43");
constexpr __detail::Item SHIFT_HYPHEN_NUMPAD        = __detail::make_item("45");
constexpr __detail::Item SHIFT_SLASH_NUMPAD         = __detail::make_item("47");
constexpr __detail::Item SHIFT_K5_NUMPAD            = __detail::make_item("53");

constexpr __detail::Item CTRL_F1                    = __detail::make_item("0;94");
constexpr __detail::Item CTRL_F2                    = __detail::make_item("0;95");
constexpr __detail::Item CTRL_F3                    = __detail::make_item("0;96");
constexpr __detail::Item CTRL_F4                    = __detail::make_item("0;97");
constexpr __detail::Item CTRL_F5                    = __detail::make_item("0;98");
constexpr __detail::Item CTRL_F6                    = __detail::make_item("0;99");
constexpr __detail::Item CTRL_F7                    = __detail::make_item("0;100");
constexpr __detail::Item CTRL_F8                    = __detail::make_item("0;101");
constexpr __detail::Item CTRL_F9                    = __detail::make_item("0;102");
constexpr __detail::Item CTRL_F10                   = __detail::make_item("0;103");
constexpr __detail::Item CTRL_F11                   = __detail::make_item("0;137");
constexpr __detail::Item CTRL_F12                   = __detail::make_item("0;138");
constexpr __detail::Item CTRL_HOME_NUMPAD           = __detail::make_item("0;119");
constexpr __detail::Item CTRL_UP_NUMPAD             = __detail::make_item("0;141");
constexpr __detail::Item CTRL_PAGE_UP_NUMPAD        = __detail::make_item("0;132");
constexpr __detail::Item CTRL_LEFT_NUMPAD           = __detail::make_item("0;115");
constexpr __detail::Item CTRL_RIGHT_NUMPAD          = __detail::make_item("0;116");
constexpr __detail::Item CTRL_END_NUMPAD            = __detail::make_item("0;117");
constexpr __detail::Item CTRL_DOWN_NUMPAD           = __detail::make_item("0;145");
constexpr __detail::Item CTRL_PAGE_DOWN_NUMPAD      = __detail::make_item("0;118");
constexpr __detail::Item CTRL_INSERT_NUMPAD         = __detail::make_item("0;146");
constexpr __detail::Item CTRL_DELETE_NUMPAD         = __detail::make_item("0;147");
constexpr __detail::Item CTRL_HOME                  = __detail::make_item("224;119");
constexpr __detail::Item CTRL_UP                    = __detail::make_item("224;141");
constexpr __detail::Item CTRL_PAGE_UP               = __detail::make_item("224;132");
constexpr __detail::Item CTRL_LEFT                  = __detail::make_item("224;115");
constexpr __detail::Item CTRL_RIGHT                 = __detail::make_item("224;116");
constexpr __detail::Item CTRL_END                   = __detail::make_item("224;117");
constexpr __detail::Item CTRL_DOWN                  = __detail::make_item("224;145");
constexpr __detail::Item CTRL_PAGE_DOWN             = __detail::make_item("224;118");
constexpr __detail::Item CTRL_INSERT                = __detail::make_item("224;146");
constexpr __detail::Item CTRL_KDELETE               = __detail::make_item("224;147");
constexpr __detail::Item CTRL_PRINT_SCREEN          = __detail::make_item("0;114");
constexpr __detail::Item CTRL_PAUSE_BREAK           = __detail::make_item("0;0");
constexpr __detail::Item CTRL_BACKSPACE             = __detail::make_item("127");
constexpr __detail::Item CTRL_ENTER                 = __detail::make_item("10");
constexpr __detail::Item CTRL_TAB                   = __detail::make_item("0;148");
constexpr __detail::Item CTRL_A                     = __detail::make_item("1");
constexpr __detail::Item CTRL_B                     = __detail::make_item("2");
constexpr __detail::Item CTRL_C                     = __detail::make_item("3");
constexpr __detail::Item CTRL_D                     = __detail::make_item("4");
constexpr __detail::Item CTRL_E                     = __detail::make_item("5");
constexpr __detail::Item CTRL_F                     = __detail::make_item("6");
constexpr __detail::Item CTRL_G                     = __detail::make_item("7");
constexpr __detail::Item CTRL_H                     = __detail::make_item("8");
constexpr __detail::Item CTRL_I                     = __detail::make_item("9");
constexpr __detail::Item CTRL_J                     = __detail::make_item("10");
constexpr __detail::Item CTRL_K                     = __detail::make_item("11");
constexpr __detail::Item CTRL_L                     = __detail::make_item("12");
constexpr __detail::Item CTRL_M                     = __detail::make_item("13");
constexpr __detail::Item CTRL_N                     = __detail::make_item("14");
constexpr __detail::Item CTRL_O                     = __detail::make_item("15");
constexpr __detail::Item CTRL_P                     = __detail::make_item("16");
constexpr __detail::Item CTRL_Q                     = __detail::make_item("17");
constexpr __detail::Item CTRL_R                     = __detail::make_item("18");
constexpr __detail::Item CTRL_S                     = __detail::make_item("19");
constexpr __detail::Item CTRL_T                     = __detail::make_item("20");
constexpr __detail::Item CTRL_U                     = __detail::make_item("21");
constexpr __detail::Item CTRL_V                     = __detail::make_item("22");
constexpr __detail::Item CTRL_W                     = __detail::make_item("23");
constexpr __detail::Item CTRL_X                     = __detail::make_item("24");
constexpr __detail::Item CTRL_Y                     = __detail::make_item("25");
constexpr __detail::Item CTRL_Z                     = __detail::make_item("26");
constexpr __detail::Item CTRL_K2                    = __detail::make_item("0");
constexpr __detail::Item CTRL_K6                    = __detail::make_item("30");
constexpr __detail::Item CTRL_HYPHEN                = __detail::make_item("31");
constexpr __detail::Item CTRL_LEFT_SQUARE_BRACKET   = __detail::make_item("27");
constexpr __detail::Item CTRL_RIGHT_SQUARE_BRACKET  = __detail::make_item("29");
constexpr __detail::Item CTRL_BACKSLASH             = __detail::make_item("28");
constexpr __detail::Item CTRL_ENTER_NUMPAD          = __detail::make_item("10");
constexpr __detail::Item CTRL_ASTERISK_NUMPAD       = __detail::make_item("0;78");
constexpr __detail::Item CTRL_PLUS_NUMPAD           = __detail::make_item("0;150");
constexpr __detail::Item CTRL_HYPHEN_NUMPAD         = __detail::make_item("0;149");
constexpr __detail::Item CTRL_SLASH_NUMPAD          = __detail::make_item("0;142");
constexpr __detail::Item CTRL_K5_NUMPAD             = __detail::make_item("0;143");

constexpr __detail::Item ALT_F1                     = __detail::make_item("0;104");
constexpr __detail::Item ALT_F2                     = __detail::make_item("0;105");
constexpr __detail::Item ALT_F3                     = __detail::make_item("0;106");
constexpr __detail::Item ALT_F4                     = __detail::make_item("0;107");
constexpr __detail::Item ALT_F5                     = __detail::make_item("0;108");
constexpr __detail::Item ALT_F6                     = __detail::make_item("0;109");
constexpr __detail::Item ALT_F7                     = __detail::make_item("0;110");
constexpr __detail::Item ALT_F8                     = __detail::make_item("0;111");
constexpr __detail::Item ALT_F9                     = __detail::make_item("0;112");
constexpr __detail::Item ALT_F10                    = __detail::make_item("0;113");
constexpr __detail::Item ALT_F11                    = __detail::make_item("0;139");
constexpr __detail::Item ALT_F12                    = __detail::make_item("0;140");
constexpr __detail::Item ALT_HOME                   = __detail::make_item("224;151");
constexpr __detail::Item ALT_UP                     = __detail::make_item("224;152");
constexpr __detail::Item ALT_PAGE_UP                = __detail::make_item("224;153");
constexpr __detail::Item ALT_LEFT                   = __detail::make_item("224;155");
constexpr __detail::Item ALT_RIGHT                  = __detail::make_item("224;157");
constexpr __detail::Item ALT_END                    = __detail::make_item("224;159");
constexpr __detail::Item ALT_DOWN                   = __detail::make_item("224;154");
constexpr __detail::Item ALT_PAGE_DOWN              = __detail::make_item("224;161");
constexpr __detail::Item ALT_INSERT                 = __detail::make_item("224;162");
constexpr __detail::Item ALT_KDELETE                = __detail::make_item("224;163");
constexpr __detail::Item ALT_BACKSPACE              = __detail::make_item("0");
constexpr __detail::Item ALT_ENTER                  = __detail::make_item("0");
constexpr __detail::Item ALT_TAB                    = __detail::make_item("0;165");
constexpr __detail::Item ALT_A                      = __detail::make_item("0;30");
constexpr __detail::Item ALT_B                      = __detail::make_item("0;48");
constexpr __detail::Item ALT_C                      = __detail::make_item("0;46");
constexpr __detail::Item ALT_D                      = __detail::make_item("0;32");
constexpr __detail::Item ALT_E                      = __detail::make_item("0;18");
constexpr __detail::Item ALT_F                      = __detail::make_item("0;33");
constexpr __detail::Item ALT_G                      = __detail::make_item("0;34");
constexpr __detail::Item ALT_H                      = __detail::make_item("0;35");
constexpr __detail::Item ALT_I                      = __detail::make_item("0;23");
constexpr __detail::Item ALT_J                      = __detail::make_item("0;36");
constexpr __detail::Item ALT_K                      = __detail::make_item("0;37");
constexpr __detail::Item ALT_L                      = __detail::make_item("0;38");
constexpr __detail::Item ALT_M                      = __detail::make_item("0;50");
constexpr __detail::Item ALT_N                      = __detail::make_item("0;49");
constexpr __detail::Item ALT_O                      = __detail::make_item("0;24");
constexpr __detail::Item ALT_P                      = __detail::make_item("0;25");
constexpr __detail::Item ALT_Q                      = __detail::make_item("0;16");
constexpr __detail::Item ALT_R                      = __detail::make_item("0;19");
constexpr __detail::Item ALT_S                      = __detail::make_item("0;31");
constexpr __detail::Item ALT_T                      = __detail::make_item("0;20");
constexpr __detail::Item ALT_U                      = __detail::make_item("0;22");
constexpr __detail::Item ALT_V                      = __detail::make_item("0;47");
constexpr __detail::Item ALT_W                      = __detail::make_item("0;17");
constexpr __detail::Item ALT_X                      = __detail::make_item("0;45");
constexpr __detail::Item ALT_Y                      = __detail::make_item("0;21");
constexpr __detail::Item ALT_Z                      = __detail::make_item("0;44");
constexpr __detail::Item ALT_K0                     = __detail::make_item("0;129");
constexpr __detail::Item ALT_K1                     = __detail::make_item("0;120");
constexpr __detail::Item ALT_K2                     = __detail::make_item("0;121");
constexpr __detail::Item ALT_K3                     = __detail::make_item("0;122");
constexpr __detail::Item ALT_K4                     = __detail::make_item("0;123");
constexpr __detail::Item ALT_K5                     = __detail::make_item("0;124");
constexpr __detail::Item ALT_K6                     = __detail::make_item("0;125");
constexpr __detail::Item ALT_K7                     = __detail::make_item("0;126");
constexpr __detail::Item ALT_K8                     = __detail::make_item("0;126");
constexpr __detail::Item ALT_K9                     = __detail::make_item("0;127");
constexpr __detail::Item ALT_HYPHEN                 = __detail::make_item("0;130");
constexpr __detail::Item ALT_EQUALS                 = __detail::make_item("0;131");
constexpr __detail::Item ALT_LEFT_SQUARE_BRACKET    = __detail::make_item("0;26");
constexpr __detail::Item ALT_RIGHT_SQUARE_BRACKET   = __detail::make_item("0;27");
constexpr __detail::Item ALT_BACKSLASH              = __detail::make_item("0;43");
constexpr __detail::Item ALT_SEMICOLON              = __detail::make_item("0;39");
constexpr __detail::Item ALT_APOSTROPHE             = __detail::make_item("0;40");
constexpr __detail::Item ALT_COMMA                  = __detail::make_item("0;51");
constexpr __detail::Item ALT_PERIOD                 = __detail::make_item("0;52");
constexpr __detail::Item ALT_SLASH                  = __detail::make_item("0;53");
constexpr __detail::Item ALT_BACKTICK               = __detail::make_item("0;41");
constexpr __detail::Item ALT_ENTER_NUMPAD           = __detail::make_item("0;166");
constexpr __detail::Item ALT_PLUS_NUMPAD            = __detail::make_item("0;55");
constexpr __detail::Item ALT_HYPHEN_NUMPAD          = __detail::make_item("0;164");
constexpr __detail::Item ALT_SLASH_NUMPAD           = __detail::make_item("0;74");

} // namespace KBD

// Almost no modern terminals support this.
inline std::string_view MAP_KEY(KBD::__detail::Item key, const std::string& str) {
    return "\033[" + std::string(key.value()) + ";" + "\"" + str + "\"" + "p";
}

// Almost no modern terminals support this.
inline std::string_view MAP_KEY(KBD::__detail::Item key, int keycode) {
    return "\033[" + std::string(key.value()) + ";" + std::to_string(keycode) + "p";
}

// Almost no modern terminals support this.
inline std::string_view MAP_KEY(KBD::__detail::Item key, const std::vector<std::variant<int, std::string>>& strs) {
    std::string str;
    for (const auto s : strs) {
        str += ';';

        if (std::holds_alternative<int>(s)) {
            str += std::to_string(std::get<int>(s));
        } else {
            str += '"';
            str += std::get<std::string>(s);
            str += '"';
        }
    }
    str += ';';
    return "\033[" + std::string(key.value()) + str + 'p';
}

} // namespace ANSI

#pragma region

class Args;

/**
 * Internal objects not intended for public use.
 */
namespace __detail {

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

// Async helpers

// Lower overhead than `std::async(std::launch::deferred, []{});`
// Especially in C++17
static inline std::future<void> async_deferred() {
    std::promise<void> p;
    p.set_value();
    return p.get_future();
}

// Command helpers
template <typename T, typename = void>
struct has_bind : std::false_type{};

template <typename T>
struct has_bind<T, std::void_t<decltype(T::bind(std::declval<T&>(), std::declval<const Args&>()))>> : std::true_type {};

template <typename T>
inline constexpr bool has_bind_v = has_bind<T>::value;

// AutoRegisteredCommand helpers
/*
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
*/

template <typename T, typename = void>
struct has_constexpr_name : std::false_type {};

template <typename T>
struct has_constexpr_name<T, std::void_t<decltype(T::command_name)>>
    : std::is_convertible<decltype(T::command_name), const char*> {};

template <typename T>
inline constexpr bool has_constexpr_name_v = has_constexpr_name<T>::value;

template <typename T, typename = void>
struct has_function_name : std::false_type {};

template <typename T>
struct has_function_name<T, std::void_t<decltype(std::declval<T>().command_name())>> 
    : std::is_convertible<decltype(std::declval<T>().command_name()), std::string> {};

template <typename T>
inline constexpr bool has_function_name_v = has_function_name<T>::value;


template <typename T, typename = void>
struct has_constexpr_description : std::false_type {};

template <typename T>
struct has_constexpr_description<T, std::void_t<decltype(T::command_description)>> 
    : std::is_convertible<decltype(T::command_description), const char*> {};

template <typename T>
inline constexpr bool has_constexpr_description_v = has_constexpr_description<T>::value;


template <typename T, typename = void>
struct has_function_description : std::false_type {};

template <typename T>
struct has_function_description<T, std::void_t<decltype(std::declval<T>().command_description())>> 
    : std::is_convertible<decltype(std::declval<T>().command_description()), std::string> {};

template <typename T>
inline constexpr bool has_function_description_v = has_function_description<T>::value;

template <typename T>
std::string get_command_name() {
    if constexpr (has_constexpr_name_v<T>) {
        return T::command_name;
    } else if constexpr (has_function_name_v<T>) {
        return T::command_name();
    } else {
        static_assert(sizeof(T) == 0, "Command must define command_name");
    }
}

template <typename T>
std::string get_command_description() {
    if constexpr (has_constexpr_description_v<T>) {
        return T::command_description;
    } else if constexpr (has_function_description_v<T>) {
        return T::command_description();
    } else {
        return "";
    }
}

// Enable ANSI support for Windows
#ifdef _WIN32
HANDLE enableANSI() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    return hOut;
#endif
}

} // namespace __detail

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

    virtual void execute(const Args& args) {}

    /**
     * Asynchronous usage. For example:
     * @brief
     * ```cpp
     * std::future<void> execute_async(const Args& args) override {
     *     return std::async(std::launch::async, [this]() {
     *         std::cout << "Doing something...\n";
     *         std::this_thread::sleep_for(std::chrono::seconds(1));
     *         std::cout << "Done something!\n";
     *     });
     * }
     * ```
     */
    virtual std::future<void> execute_async(const Args& args) {
        execute(args);
        return __detail::async_deferred();
    }

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

//template <typename T>
template <typename T, typename = typename std::enable_if<std::is_base_of<CommandController, T>::value && !std::is_same<CommandController, T>::value>::type>
struct ControllerRegistrar {
    ControllerRegistrar(const std::string& name, const std::string& desc) {
        register_it(name, desc);
    }

    ControllerRegistrar(const std::string& name) {
        register_it(name, "");
    }

    ControllerRegistrar() {
        register_it(__detail::get_command_name<T>(), __detail::get_command_description<T>());
    }

private:
    void register_it(const std::string& name, const std::string& desc) {
        auto& reg = CommandController::get_registry();

        auto it = std::find_if(reg.begin(), reg.end(),
            [&](const auto& r) { return r.name == name; });

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
        __detail::parse_names(name, long_name, short_name);
    }

    Option(const std::string& name) : target(nullptr) {
        __detail::parse_names(name, long_name, short_name);
    }

    Option(CommandController* ctx, const std::string& name, const std::string& desc = "") {
        __detail::parse_names(name, long_name, short_name);
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
        return __detail::pretty_type<T>();
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
        __detail::parse_names(name, long_name, short_name);
    }

    Flag(const std::string& name) : target(nullptr), value(false) {
        __detail::parse_names(name, long_name, short_name);
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

    std::function<std::future<void>(const Args&)> action;

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

    /**
     * @brief
     * Code that executes when a command runs. Can be synchronous:
     * ```cpp
     * app.command("ping").run([]() {
     *     std::cout << "Pong!";
     * });
     * ```
     * Or asynchronous:
     * ```cpp
     * app.command("download")
     *     .run([](const Args& args) -> std::future<void> {
     *         return std::async(std::launch::async, []() {
     *             std::cout << "Starting download...\n";
     *             std::this_thread::sleep_for(std::chrono::seconds(3));
     *             std::cout << "Download complete!\n";
     *         });
     *     });
     * ```
     */
    template <typename Fn>
    Command& run(Fn fn) {
        action = [this, fn](const Args& args) -> std::future<void> {
            invoke(fn, args);
        };
        return *this;
    }

    /**
     * @deprecated Never use, it does not work at all
     */
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
        } else if constexpr (__detail::has_bind_v<T>) {
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
    std::future<void> invoke(Fn& fn, const Args& args) {
        if constexpr (std::is_invocable_v<Fn, const Args&>) {
            using Ret = std::invoke_result_t<Fn, const Args&>;
            if constexpr (std::is_same_v<Ret, std::future<void>>) {
                return fn(args);
            } else {
                fn(args);
                return __detail::async_deferred();
            }
        } else if constexpr (std::is_invocable_v<Fn>) {
            using Ret = std::invoke_result_t<Fn>;
            if constexpr (std::is_same_v<Ret, std::future<void>>) {
                return fn();
            } else {
                fn();
                return __detail::async_deferred();
            }
        } else {
            using T = __detail::function_arg_t<Fn>;
            fn(resolve<T>(args));
            return __detail::async_deferred();
        }
    }
};

class App {
public:
    std::string name;
    std::string description;

    App(const std::string& n = "", const std::string& desc = "") : name(n), description(desc) {}

    Command& command(const std::string& name) {
        auto cmd = std::make_unique<Command>(name);
        auto* ptr = cmd.get();
        commands.push_back(std::move(cmd));
        command_map[name] = ptr;
        return *ptr;
    }

    int run(int argc, char** argv) {
        try {
#ifdef _WIN32
            __detail::enableANSI();
#endif

            load_controllers();

            if (argc < 2) {
                print_help(argv[0]);
                return 0;
            }

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
                    std::string key = __detail::normalise(arg.substr(0, eq));
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
                    std::string key = __detail::normalise(arg);

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
                Args args{&cmd.option_map, &cmd.positionals};
                std::future<void> async_task = cmd.action(args);
                async_task.get();
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

    void print_help(const char* n) {
        if (!name.empty()) {
            std::cout << name;
        } else {
            std::cout << n;
        }

        if (!description.empty()) {
            std::cout << " - " << description;
        }
        std::cout << "\n\n";
        
        std::cout << "Usage:\n";
        std::cout << "  " << (name.empty() ? n : name) << " <command> [options]\n\n";

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
            cmd.run([shared_controller](const Args& args) -> std::future<void> {
                return shared_controller->execute_async(args);
            });

        }
    }
};

namespace ui {

namespace __detail {

#ifndef _WIN32
class TerminalRawMode {
private:
    struct termios orig_, raw_;

    TerminalRawMode() {
        tcgetattr(STDIN_FILENO, &orig_);
        raw_ = orig_;
        raw_.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw_);
    }

    ~TerminalRawMode() {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_);
    }
};

int getch() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        return c;
    }
    return -1s
}
#endif

}

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

    ProgressBar(size_t total, size_t width = 50, Theme theme = Themes::classic(), Options options = {.progress_in_bar=false,.eta_enabled=true,.items_per_second=false}) : total_(total), value_(0), width_(width), theme_(theme), options_(options) {
#ifdef _WIN32
        linea::__detail::enableANSI();
#endif
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
        std::string s = std::string(count, c);
        return s;
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
        return *this;
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

/**
 * @todo
 * - Live validation (per keystroke)
 * - Autocomplete
 */
class Prompt {
public:
    struct Theme {
        std::string prefix = "> ";
        std::string input_color = "";
        std::string reset_color = "\033[0m";
    };

    struct Options {
        bool inline_mode = false;
        bool show_default = true;
    };

    virtual ~Prompt() = default;

    Prompt& setLabel(const std::string& label) {
        label_ = label;
        return *this;
    }

    Prompt& setTheme(const Theme& theme) {
        theme_ = theme;
        return *this;
    }

    Prompt& setOptions(const Options& opts) {
        options_ = opts;
        return *this;
    }

    Prompt& setDefault(const std::string& value) {
        default_ = value;
        return *this;
    }

    Prompt& setValidator(std::function<bool(const std::string&)> fn, std::string error = "Invalid input") {
        validator_ = fn;
        error_message_ = error;
        return *this;
    }

    virtual std::string get() {
        while (true) {
            render();

            std::string input = readInput();

            if (input.empty() && default_) {
                input = *default_;
            }

            if (validator_ && !validator_(input)) {
                std::cout << error_message_ << "\n";
                continue;
            }

            return input;
        }
    }

protected:
    std::string label_;
    Theme theme_;
    Options options_;

    std::optional<std::string> default_;

    std::function<bool(const std::string&)> validator_;
    std::string error_message_ = "Invalid input";

    std::function<std::string(const Prompt&)> formatter_;

    virtual std::string readInput() = 0;

    void render() const {
        if (formatter_) {
            std::cout << formatter_(*this);
            return;
        }

        if (!label_.empty()) {
            if (options_.inline_mode) {
                std::cout << label_ << " ";
            } else {
                std::cout << label_ << "\n";
            }
        }

        if (!options_.inline_mode) {
            std::cout << theme_.prefix;
        }

        if (default_ && options_.show_default) {
            std::cout << "(" << *default_ << ") ";
        }
    }
};

class PromptText : public Prompt {
protected:
    std::string readInput() override {
        std::string input;
        std::getline(std::cin, input);
        return input;
    }
};

class PromptPassword : public Prompt {
public:
    struct MaskOptions {
        char mask_char = '*';
        bool show_mask = true;
    };

    PromptPassword& setOptions(const MaskOptions& opts) {
        options_ = opts;
        return *this;
    }

    Prompt& setLiveValidator(std::function<bool(const char, const std::string&)> fn) {
        liveValidator_ = fn;
        return *this;
    }

protected:
    std::function<bool(const char, const std::string&)> liveValidator_;

    std::string readInput() override {
        std::string password;

#ifdef _WIN32
        char ch;
        while ((ch = _getch()) != '\r') {
            if (ch == '\b') {
                if (!password.empty()) {
                    password.pop_back();
                    if (options_.show_mask) {
                        std::cout << "\b \b";
                    }
                }
            } else if (ch != '\n') {
                if (liveValidator_ && !liveValidator_(ch, password)) continue;
                password += ch;
                if (options_.show_mask) {
                    std::cout << options_.mask_char;
                }
            }
        }
#else
        temios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, newt);

        std::getline(std::cin, password);

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

        if (options_.show_mask) {
            std::cout << std::string(password.size(), options_.mask_char);
        }
#endif

        std::cout << "\n";
        return password;
    }

private:
    MaskOptions options_;
};

/**
 * @throws
 * - Ctrl+C
 * 
 * - No present options
 */ 
class PromptSelect : public Prompt {
public:
    PromptSelect() {
#ifdef _WIN32
        hConsole_ = linea::__detail::enableANSI(); // Return hConsole
#endif
    }

    PromptSelect& setOptions(const std::vector<std::string>& options) {
        options_ = options;
        return *this;
    }

    PromptSelect& setLabel(const std::string& label) {
        label_ = label;
        return *this;
    }

protected:
    std::string readInput() override {
        if (options_.empty()) {
            throw std::runtime_error("PromptSelect: no options provided");
        }

        int selected = 0;

        auto render = [&](int y) {
            goToXY(0, y);
            std::cout << ANSI::ERASE_LINE << label_ << "\n";
        
            for (size_t i = 0; i < options_.size(); ++i) {
                std::cout << ANSI::ERASE_LINE;
                const std::string prefix = (selected == i) ? theme_.input_color + theme_.prefix + theme_.reset_color : "  ";
                std::cout << prefix << options_[i] << "\n";
            }
            std::cout.flush();
        };
#ifdef _WIN32
        COORD startPos = getCursorPosition();

        render(startPos.Y);

        while (true) {
            int key = _getch();

            if (key == 0 || key == 0xE0) {
                key = _getch();
                if (key == 72) {
                    selected = (selected - 1 + options_.size()) % options_.size();
                    render(startPos.Y);
                } else if (key == 80) {
                    selected = (selected + 1) % options_.size();
                    render(startPos.Y);
                }
            } else if (key == ' ' || key == '\n' || key == '\r') {
                break;
            } else if (key == 3) {
                throw std::runtime_error("Interrupted (Ctrl+C)");
            }
        }

        goToXY(0, startPos.Y);
#else
        TerminalRawMode rawMode;

        int startRow = 0;

        render(startRow);

        while (true) {
            int key = __detail::getch();

            if (key == ANSI::ESC) {
                int next1 = __detail::getch();
                int next2 = __detail::getch();

                if (next1 != '[') continue;

                if (next2 == 'A') { // Up
                    selected = (selected - 1 + options_.size()) % options_.size();
                    render(startRow);
                } else if (next2 == 'B') { // Down
                    selected = (selected + 1) % options_.size();
                    render(startRow);
                }
            } else if (key == ' ' || key == '\n') {
                break;
            } else if (key == 3) {
                throw std::runtime_error("Interrupted (Ctrl+C)");
            }
        }
#endif

        size_t totalLines = options_.size() + 1;
        for (size_t i = 0; i < totalLines; ++i) {
            goToXY(0, startPos.Y + i);
            std::cout << ANSI::ERASE_LINE;
        }

        goToXY(0, startPos.Y);

        return options_[selected];
    }

private:
    std::vector<std::string> options_;
    std::string label_;

#ifdef _WIN32
    HANDLE hConsole_;

    COORD getCursorPosition() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(hConsole_, &csbi)) {
            return csbi.dwCursorPosition;
        }
        return {0,0};
    }

    void goToXY(int x, int y) {
        COORD coord;
        coord.X = x;
        coord.Y = y;
        SetConsoleCursorPosition(hConsole_, coord);
    }
#else
    void goToXY(int x, int y) {
        std::cout << ANSI::CURSOR_MOVE(x + 1, y + 1);
    }
#endif
};

} // namespace ui

#pragma endregion

} // namespace linea
