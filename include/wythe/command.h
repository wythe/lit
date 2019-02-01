#pragma once
#include "exception.h"

#include <algorithm>
#include <functional>
#include <vector>
#include <string>
#include <map>
#include <iostream>

/*
 * Command lines are of the form:
 * [global options...] command [command options...] [targets...]
 */

namespace wythe
{
namespace cli
{

struct option
{
      public:
	option(const std::string name, char short_opt, const std::string desc,
	       std::function<void()> action = []() {})
	    : name(name), short_opt(short_opt), desc(desc),
	      binary_action(action), flag(true), present(false)
	{
	}

	option(const std::string name, char short_opt, const std::string desc,
	       const std::string def,
	       std::function<void(std::string val)> action = [](std::string) {})
	    : name(name), short_opt(short_opt), desc(desc), default_value(def),
	      action(action), flag(false), present(false)
	{
	}

	std::string name;
	char short_opt;
	std::string desc;
	std::string default_value;

	std::function<void()> binary_action;
	std::function<void(std::string)> action;

	bool flag;
	bool present;
};

template <typename Opts> struct command {
	command() = default;
	command(const std::string &name, const std::string &desc,
		void (*action)(Opts &))
	    : name(name), desc(desc), action(action)
	{
	}
	std::string name;
	std::string desc;
	void (*action)(Opts &);
	std::vector<option> opts;
};

template <typename Opts> struct line
{
	line(const std::string &version, const std::string &name,
	     const std::string &desc, const std::string &usage)
	    : version_no(version), name(name), desc(desc), usage(usage)
	{
	}

	template <typename T> void disp_opts(T &opts) const
	{
		for (auto const &opt : opts) {
			std::cout << "  ";
			if (opt.short_opt != '~')
				std::cout << "-" << opt.short_opt << ", ";
			std::cout << "--" << opt.name;
			if (opt.flag)
				std::cout << " : " << opt.desc << "\n";
			else {
				std::cout << "= value : " << opt.desc;
				if (!opt.default_value.empty())
					std::cout << " (" << opt.default_value
						  << ")";
				std::cout << std::endl;
			}
		}
	}

	template <typename T> void disp_cmds(T &cmds) const
	{
		for (auto const &cmd : cmds) {
			std::cout << "\n  " << cmd.name << " : " << cmd.desc
				  << '\n';
			disp_opts(cmd.opts);
		}
	}

	void help() const
	{
		std::cout << name << " -- " << desc << "\n\n" << usage << "\n";
		disp_opts(global_opts);
		disp_cmds(commands);
		if (!notes.empty()) {
			std::cout << "\n";
			for (auto const &n : notes)
				std::cout << n << "\n";
		}
		std::cout << "\n";
	}

	void version() const
	{
		std::cout << name << " version " << version_no << '\n';
	}

	void set_defaults(auto opts)
	{
		for (auto &opt : opts) {
			if (!opt.default_value.empty())
				opt.action(opt.default_value);
		}
	}

	void exec()
	{
		if (!cmd.name.empty())
			cmd.action();
	}

	template <class... Args> void go(Args &&... args)
	{
		if (!cmd.name.empty())
			cmd.action(std::forward<Args>(args)...);
	}

	std::vector<std::string> targets;

	std::vector<option> global_opts;
	std::vector<std::string> notes;
	std::vector<command<Opts>> commands;

	std::string version_no;
	std::string name;
	std::string desc;
	std::string usage;
	command<Opts> cmd;
};

template <typename T>
inline void help(T &line) { line.help(); }

template <typename T>
inline void version(T &line) { line.version(); }

template <typename T>
inline void parse(T &line, int argc, char **argv)
{
	enum State {
		Idle,
		ShortStart,
		ShortContinue,
		ValueStart,
		Value,
		QuotedValue,
		LongOptionStart,
		LongOption,
		LookingForEqual,
		CommandOrTarget,
		LookingForTarget,
		Target

	};
	State state = Idle;
	std::string cl;
	std::string target;
	std::string long_opt;
	std::string value;
	std::vector<option>::iterator it;

	std::vector<option> &opts = line.global_opts;
	// let's set all the global opts to their defaults
	line.set_defaults(line.global_opts);
	// let's add help and version options.
	it = std::find_if(opts.begin(), opts.end(),
			  [&](option &o) { return o.short_opt == 'h'; });
	opts.emplace_back("help", (it == opts.end() ? 'h' : '~'),
			  "Show this help usage", [&] {
				  help(line);
				  exit(0);
			  });

	it = std::find_if(opts.begin(), opts.end(),
			  [&](option &o) { return o.short_opt == 'v'; });
	opts.emplace_back("version", (it == opts.end() ? 'v' : '~'),
			  "Show version", [&] {
				  version(line);
				  exit(0);
			  });

	for (int i = 1; i < argc; ++i) {
		cl += argv[i];
		if (i != argc - 1)
			cl += ' ';
	}
	cl += ' ';
	// PRINT("parse: " << cl);

	for (auto ch : cl) {
		// WARN("ch = " << ch);
		switch (state) {
		case Idle: // looking for the first character
			switch (ch) {
			case '-':
				state = ShortStart;
				break;
			case ' ':
				break;
			default: // isgraph character, starting target
				target += ch;
				if (line.cmd.name.empty())
					state = CommandOrTarget;
				else
					state = Target;
				break;
			}
			break;
		case ShortStart: // first '-'
			switch (ch) {
			case '-':
				state = LongOptionStart;
				break;
			case ' ':
				PANIC("illegal \"- \" sequence");
			default:
				if (isgraph(ch)) // found short option
				{
					it = std::find_if(
					    opts.begin(), opts.end(),
					    [&](option &o) {
						    return o.short_opt == ch;
					    });
					if (it == opts.end())
						PANIC("invalid option: " << ch);

					if (it->present)
						PANIC("option " << ch
								<< " already "
								   "present");
					it->present = true;
					// do it!  We may want to save
					// the
					// actions until parsing is
					// complete, in
					// case
					// there are errors later in
					// command
					// line
					if (it->flag) {
						it->binary_action();
						state = ShortContinue;
					} else
						state = ValueStart;
				}
			}
			break;
		case ShortContinue: // possibly more short opts
			switch (ch) {
			case '-':
				PANIC("illegal \"-\" character");
			case ' ':
				state = Idle;
			default:
				if (isgraph(ch)) // found short option
				{
					it = std::find_if(
					    opts.begin(), opts.end(),
					    [&](option &o) {
						    return o.short_opt == ch;
					    });
					if (it == opts.end())
						PANIC("invalid option: " << ch);
					if (it->present)
						PANIC("option " << ch
								<< " already "
								   "present");
					it->present = true;
					if (it->flag)
						it->binary_action();
					else
						state = ValueStart;
				}
			}
			break;
		case ValueStart:
			switch (ch) {
			case ' ':
				break;
			case '\"':
				// WARN("setting to quoted value");
				value.clear();
				state = QuotedValue;
				break;

			default:
				value.clear();
				value += ch;
				state = Value;
			}
			break;
		case Value:
			switch (ch) {
			case ' ':
				it->action(value);
				state = Idle;
				break;
			default:
				value += ch;
			}

			break;
		case QuotedValue:
			switch (ch) {
			case '\"':
				it->action(value);
				state = Idle;
				break;
			default:
				value += ch;
			}

			break;
		case LongOptionStart: // "--"
			// PRINT("LongOptionStart");
			if (isgraph(ch)) {
				long_opt.clear();
				long_opt += ch;
				state = LongOption;
			} else {
				PANIC("illegal \"--" << ch << "\" sequence");
			}
			break;

		case LongOption:
			// PRINT("LongOption");
			switch (ch) {
			case ' ':
				it = std::find_if(
				    opts.begin(), opts.end(), [&](option &o) {
					    return o.name == long_opt;
				    });
				if (it == opts.end())
					PANIC("invalid option: " << long_opt);
				it->present = true;
				if (it->flag) {
					it->binary_action();
					state = Idle;
				} else
					state = LookingForEqual;
				break;
			case '=':
				it = std::find_if(
				    opts.begin(), opts.end(), [&](option &o) {
					    return o.name == long_opt;
				    });
				if (it == opts.end())
					PANIC("invalid option: " << long_opt);
				it->present = true;
				if (it->flag)
					PANIC("value not expected for "
					      "option: "
					      << long_opt);
				state = ValueStart;
			default:
				long_opt += ch;
				break;
			}
			break;

		case LookingForEqual:
			switch (ch) {
			case ' ':
				break;
			case '=':
				state = ValueStart;
				break;
			default:
				PANIC("expected '=' after \"" << long_opt
							      << "\" option");
			}
			break;
		case CommandOrTarget:
			switch (ch) {
			case ' ': {
				auto it = std::find_if(
				    line.commands.begin(), line.commands.end(),
				    [&](auto &c) { return c.name == target; });
				if (it ==
				    line.commands.end()) { // not in command list
					line.targets.push_back(target);
					target.clear();
					state = LookingForTarget;
				} else {
					line.cmd = *it; // yes, its a command,
					// let's get some options
					opts = line.cmd.opts;
					line.set_defaults(opts);
					target.clear();
					state = Idle;
				}

				break;
			}
			default:
				target += ch;
			}
			break;
		case LookingForTarget:
			switch (ch) {
			case ' ':
				break;

			default:
				target += ch;
				state = Target;
			}
			break;

		case Target:
			switch (ch) {
			case ' ':
				line.targets.push_back(target);
				target.clear();
				state = LookingForTarget;
				break;
			default:
				target += ch;
			}
			break;
		default:
			PANIC("panic!");
		}
	}
}


template <typename T, typename... Args>
void add_opt(line<T> &line, Args &&... args)
{
	line.global_opts.emplace_back(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
void add_opt(command<T> &cmd, Args &&... args)
{
	cmd.opts.emplace_back(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
void add_cmd(line<T> &line, Args &&... args)
{
	line.commands.emplace_back(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
typename std::vector<command<T>>::iterator emp_cmd(line<T> &line, Args &&... args)
{
	return line.commands.emplace(line.commands.end(), std::forward<Args>(args)...);
}
}
}
