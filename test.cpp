#include "cli.hpp" // ::linea

#include <thread>
#include <chrono>

struct Config {
    int port;
    bool verbose;

    static void bind(Config& c, const linea::Args& args) {
        c.port = args.get<int>("port");
        c.verbose = args.get<bool>("verbose");
    }
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

REGISTER_COMMAND(BuildCommand, "build", "Build the server for production")

int main(int argc, char** argv) {
    linea::App app("AppName", "CLi app ig");
    int port;
    bool verbose = false;

    app.command("serve")
        .option("--port,-p", port)
            .check([](const int p) {
                return p > 1 && p < 65536;
            }, "Port must be between 1 and 65535")
            .description_text("Port to serve on")
            .done()
        .flag("--verbose,-v", verbose).done()
        .run([&]() {
            std::cout << "Port: " << port << std::endl;
            std::cout << "Verbose: " << verbose << std::endl;

            linea::ui::ProgressBar bar(100, 40, linea::ui::ProgressBar::Themes::classic());
            bar.setLabel("Loading");
            bar.enableAutoETA();

            for (int i = 0; i <= 100; ++i) {
                bar.set(i);
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }

            std::cout << "\nDone\n";
        });

    return app.run(argc, argv);
}