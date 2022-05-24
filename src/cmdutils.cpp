#include "cmdutils.hpp"

#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <vector>
#include "formatted_out.hpp"

constexpr unsigned int progressbar_width = 40;

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#define WEXITSTATUS
#endif
std::string run_and_capture_out(const std::string &cmd, int &return_val) {
	std::string result;
	{
		std::array<char, 1024> buffer;
		auto pclose_capture = [&return_val](FILE *stream) { return_val = WEXITSTATUS(pclose(stream)); };
		std::unique_ptr<FILE, decltype(pclose_capture)> pipe(popen(cmd.c_str(), "r"), pclose_capture);
		if (!pipe) {
			std::cout << prettyErrorGeneral("Failed to pipe " + cmd, severity::ERROR) << std::endl;
			return_val = 1;
			return "";
		}
		while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
			result += buffer.data();
		}
	}
	return result;
}
std::string repeat(const std::string &str, int n) {
    std::ostringstream os;
    for(int i = 0; i < n; i++)
        os << str;
    return os.str();
}

class multi_command {
public:
	multi_command(const std::vector<std::string> &cmds, const std::string &final_cmd, const std::string &note, unsigned int threads) :
		final(final_cmd), note(note), threadc(threads) {
		for (const auto &cmd : cmds)
			cmdqueue.emplace(cmd);
		totalcmds = cmds.size() + (!final_cmd.empty());
	}
	bool run() {
		failed = false;
		donecmds = 0;
		print_status();
		for (unsigned int i = 1; i < threadc; ++i) {
			threads.emplace_back(new std::thread(&multi_command::work, this));
		}
		work();
		for (const auto &t : threads) {
			t->join();
		}
		threads.clear();
		if (!failed && !final.empty()) {
			int retval;
			std::string out = run_and_capture_out(final, retval);
			failed |= retval;
			std::cout << out;
		}
		if (failed) {
			std::cout << "\x1b[91m" << repeat("\u2588", progressbar_width) << " failed - \x1b[0m" << note << std::endl;
			return true;
		} else {
			std::cout << "\x1b[92m" << repeat("\u2588", progressbar_width) << " done - \x1b[0m" << note << std::endl;
		}
		return false;
	}
	void work() {
		cmdqueue_mutex.lock();
		while (!cmdqueue.empty()) {
			std::string cmd = cmdqueue.front();
			cmdqueue.pop();
			cmdqueue_mutex.unlock();
			int retval;
			std::string out = run_and_capture_out(cmd, retval);
			failed |= retval;
			cmdqueue_mutex.lock();
			++donecmds;
			stdout_mutex.lock();
			std::cout << out;
			print_status();
			stdout_mutex.unlock();
		}
		cmdqueue_mutex.unlock();
	}
	void print_status() {
		unsigned int part_done = (donecmds * progressbar_width) / totalcmds;
		unsigned int part_todo = progressbar_width - part_done;
		unsigned int percent_done = (donecmds * 100) / totalcmds;
		std::cout << "\x1b[92m" << repeat("\u2588", part_done) << repeat("\u2592", part_todo) <<
			percent_done << "% " << "\x1b[0m" << note << "        \r" << std::flush;
		std::cout << std::string(8 + note.size() + progressbar_width, ' ') << '\r';
	}
private:
	std::vector<std::unique_ptr<std::thread>> threads;
	std::mutex stdout_mutex;
	std::mutex cmdqueue_mutex;
	std::queue<std::string> cmdqueue;
	std::string final;
	std::string note;
	unsigned int threadc;
	unsigned int totalcmds;
	unsigned int donecmds;
	std::atomic<int> failed;
};

bool run_commands(const std::vector<std::string> &cmds, const std::string &note) {
	multi_command mc(cmds, "", note, 1);
	return mc.run();
}
bool run_commands_parallel(const std::vector<std::string> &cmds, const std::string &note) {
	unsigned int threads = std::thread::hardware_concurrency();
	if (threads < 2) {
		threads = 1;
	} else {
		--threads;
	}
	multi_command mc(cmds, "", note, threads);
	return mc.run();
}
bool build_using(const std::vector<std::string> &compile_cmds, const std::string &link_cmd, const std::string &note) {
	unsigned int threads = std::thread::hardware_concurrency();
	if (threads < 2) {
		threads = 1;
	} else {
		--threads;
	}
	multi_command mc(compile_cmds, link_cmd, note, threads);
	return mc.run();
}
