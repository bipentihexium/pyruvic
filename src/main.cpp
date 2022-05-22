#include <iostream>
#include "cfg.hpp"
#include "formatted_out.hpp"
#include "project_utils.hpp"

bool verbose;

void showHelp() {
	std::cout <<
		"usage: pyruvic [options] [action]\n"
		"\tactions:\n" <<
		"\t\thelp - shows help (this text)\n" <<
		"\t\tnew - generates a new project; next argument is project name\n" <<
		"\t\tbuild - builds the project\n" <<
		"\t\trun - builds and runs the project\n" <<
		"\t\torun - runs the last build of project\n" <<
		"\toptions:\n" <<
		"\t\t-c    --clean - cleans build files and project libraries before building\n" <<
		"\t\t-o    --obfuscate - only with release builds, makes the code harder to decompile (unstable!)\n" <<
		"\t\t-r    --release - enables optimizations, disables debug info\n" <<
		"\t\t-v    --verbose - shows extra info" <<
		"\t\t      --version - shows version\n" <<
		"\t\t      --vscode-ext - updates include paths for the ms-vscode.cpptools extension for vscode" << std::endl;
}
void printVersion() {
	std::cout << "Pyruvic version " << __PYRUVIC_VERSION_MAJOR << "." << __PYRUVIC_VERSION_MINOR << "." <<
		__PYRUVIC_VERSION_PATCH << "." << __PYRUVIC_VERSION_TWEAK << " " << __PYRUVIC_VERSION_NAME << std::endl;
}

int main(int argc, char **argv) {
	if (argc == 1) {
		showHelp();
		return 0;
	}
	bool clean, build, run;
	bool release, obfuscate;
	verbose = clean = build = run = release = obfuscate = false;
	load_cfg();
	for (int i = 1; i < argc; ++i) {
		std::string arg(argv[i]);
		if (arg == "help") {
			showHelp();
			return 0;
		} else if (arg == "new") {
			if (++i >= argc) {
				std::cout << prettyErrorGeneral("Expected project name", severity::ERROR) << std::endl;
				return -1;
			}
			new_project(std::string(argv[i]));
			return 0;
		} else if (arg == "build") {
			build = true;
		} else if (arg == "run") {
			build = run = true;
		} else if (arg == "orun") {
			run = true;
		} else if (arg.starts_with('-')) {
			if (arg.size() > 1 && arg[1] == '-') {
				if (arg == "--release") {
					release = true;
				} else if (arg == "--obfuscate") {
					obfuscate = true;
				} else if (arg == "--clean") {
					clean = true;
				} else if (arg == "--verbose") {
					verbose = true;
				} else if (arg == "--version") {
					printVersion();
				}  else if (arg == "--vscode-ext") {
					std::cout << prettyErrorGeneral("The --vscode-ext option is unimplemented - it'll do nothing.", severity::WARN) << std::endl;
					// TODO: --vscode-ext option
				} else {
					std::cout << prettyErrorGeneral("Unknown option \"" + arg + "\"", severity::ERROR) << std::endl;
				}
			} else {
				for (unsigned int j = 1; j < arg.size(); ++j) {
					switch (arg[j]) {
					case 'r': release = true; break;
					case 'o': obfuscate = true; break;
					case 'c': clean = true; break;
					case 'v': verbose = true; break;
					default:
						std::cout << prettyErrorGeneral(std::string("Unknown switch -") + arg[j], severity::ERROR) << std::endl;
						break;
					}
				}
			}
		} else {
			std::cout << prettyErrorGeneral("Unknown action \"" + arg + "\"", severity::ERROR) << std::endl;
		}
	}
	if (clean) {
		clean_build_files();
	}
	load_project(release, obfuscate);
	pre_build();
	if (build) {
		build_project(release, obfuscate);
	}
	post_build();
	if (run) {
		run_project();
	}
	return 0;
}
