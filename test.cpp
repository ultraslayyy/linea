#include "linea.hpp" // ::linea

#include <thread>
#include <chrono>

struct Config {
    int port;
    bool verbose;

    LINEA_BIND(Config,
        LINEA_FIELD(port)
        LINEA_FIELD(verbose)
    )
};

class BuildCommand : public linea::CommandController {
public:
    void setup(linea::Command& cmd) override {
        cmd.option("--out, -o", outDir)
               .description_text("Output directory").done()
           .flag("--verbose, -v", verbose).done();
    }

    void execute(const linea::Args& args) override {
        std::cout << "Building server into \"" << outDir << (verbose ? "\" (verbose)\n" : "\"\n");
    }

private:
    std::string outDir = "build";
    bool verbose = false;
};

class LoginCommand : public linea::CommandController {
public:
    static std::string command_name() { return "login"; }
    static std::string command_description() { return "Login to the server"; }

    void setup(linea::Command& cmd) override {
        cmd.option("--pass,-p", pass)
            .description_text("Enter password from CLI args").done();
    }

    void execute(const linea::Args& args) override {
        if (pass.empty()) {
            linea::ui::PromptPassword prompt;
            prompt.setLabel("Enter password:");
            prompt.setValidator([&](const std::string& input) -> bool {
                if (input == password) return true;
                return false;
            }, "Invalid password, please try again.");
            pass = prompt.get();
        }

        if (pass != password) {
            std::cout << "Incorrect password\n";
            std::cout << pass << "\n";
            return;
        }

        std::cout << "Logged in successfully!\nLoading data...\n";

        linea::ui::Table table;
        table.setHeaders({"id", "username"});
        table.addRow({"0", "Baby"});
        table.addRow({"1", "John"});
        table.addRow({"2", "Jane"});
        table.render();

        std::cout << "Successfully loaded data!\n";
        return;
    }

private:
    std::string pass = "";

    std::string password = "abc"; // test password
};

REGISTER_COMMAND(BuildCommand, "build", "Build the server for production");
REGISTER_COMMAND(LoginCommand);

int main(int argc, char** argv) {
    linea::App app;
    int port;
    bool verbose = false;

    app.command("serve")
        .option("--port,-p", port)
            .check(linea::validators::range<int>(1, 65535))
            .description_text("Port to serve on")
            .done()
        .flag("--verbose,-v", verbose).done()
        .run([&]() {
            std::cout << "Port: " << port << std::endl;
            std::cout << "Verbose: " << verbose << std::endl;

            linea::ui::ProgressBar bar(100, 40, linea::ui::ProgressBar::Themes::minimal());
            bar.setLabel("Loading");
            bar.enableAutoETA();

            for (int i = 0; i <= 100; ++i) {
                bar.set(i);
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }

            std::cout << "\nDone\n";
        });

    app.command("help")
        .run([&]() {
            app.print_help(argv[0]);
        });

    return app.run(argc, argv);
}