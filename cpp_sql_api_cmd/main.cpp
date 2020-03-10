#include "main.h"

#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <functional>
#include <INIReader.h>

#include "cpp_sql_api.h"

using namespace std;
typedef std::function<void(int& idx, int& argc, char** argv)> cmd_func;
typedef std::map<std::string, cmd_func> cmd_map;

cpp_sql_api * api = nullptr;

cmd_map cmd_map_;

list<string> query_list;

string fetch_cmd_ = "all";
string db_ = "vntu_iq";
string action_str_delimmer_ = "";

int argument_id = 1;

bool bIsQueryFileSpecified = false;

void init_cmd_map()
{
	cmd_map_["-f"] = [](int& idx, int& argc, char** argv)
	{
		idx++;

		if (idx >= argc)
		{
			throw std::runtime_error("To few arguments for -f");
		}

		bIsQueryFileSpecified = true;

		ifstream q_file;
		q_file.open(string(argv[idx]));

		if (!q_file.is_open())
		{
			throw std::runtime_error("error opening file: " + string(argv[2]));
		}

		string query;

		while (!q_file.eof())
		{
			std::getline(q_file, query);
			if (query.size() == 0)continue;
			query_list.push_back(query);
		}
	};

	cmd_map_["-db"] = [](int& idx, int& argc, char** argv)
	{
		idx++;

		if (idx >= argc)
		{
			throw std::runtime_error("To few arguments for -db");
		}

		db_ = argv[idx];
	};

	cmd_map_["-fm"] = [](int& idx, int& argc, char** argv)
	{
		idx++;

		if (idx >= argc)
		{
			throw std::runtime_error("To few arguments for -db");
		}

		fetch_cmd_ = argv[idx];
	};

	cmd_map_["-h"] = [](int& idx, int& argc, char** argv)
	{
		display_help_file(get_exec_dir(argv[0]));
		exit(0);
	};
}

int main(int argc, char** argv)
{
	try
	{
		setlocale(LC_ALL, "ru_RU.UTF-8");

		string exec_dir = get_exec_dir(argv[0]);

		if (argc == 1)
		{
			display_help_file(exec_dir);
			return 1;
		}

		api = new cpp_sql_api(parce_url(exec_dir));

		cpp_sql_api::init();

		init_cmd_map();

		parce_ini(exec_dir);

		string result;

		for (int idx = 1; idx < argc; idx++)
		{
			if (argv[idx][0] == '-')
			{
				cmd_map_[argv[idx]](idx, argc, argv);
			}
			else
			{
				argument_id = idx;
			}
		}

		if (bIsQueryFileSpecified)
		{
			cout << action_str_delimmer_ << endl;
			cout << "-------------EXECUTING_SQL_QUERIES_FROM_FILE_--------------" << endl;
			for (auto& query : query_list)
			{
				cout << action_str_delimmer_ << endl;
				cout << query + ":" << endl;
				cout << action_str_delimmer_ << endl;
				result = api->sql_query(query, db_, fetch_cmd_);
				try
				{
					cout << nlohmann::json::parse(result) << endl;
				}
				catch (const nlohmann::json::parse_error&)
				{
					cout << result << endl;
				}
			}

			cout << "----------------------SCENARIO_ENDED-----------------------" << endl;
		}
		else
		{
			result = api->sql_query(argv[argument_id], db_, fetch_cmd_);
			try
			{
				cout << action_str_delimmer_ << endl;
				cout << nlohmann::json::parse(result) << endl;
				cout << action_str_delimmer_ << endl;
			}
			catch (const nlohmann::json::parse_error&)
			{
				cout << result << endl;
			}
		}

		cpp_sql_api::cleanup();

		delete api;
	}
	catch (const std::exception& e)
	{
		cerr << e.what() << endl;
		delete api;
	}

	return 0;
}

void display_help_file(std::string path)
{
	ifstream help_file;
	help_file.open(string(path + "help.txt"));

	if (!help_file.is_open())
	{
		throw std::runtime_error("error opening: help.txt");
	}

	string help_line_;

	while (!help_file.eof())
	{
		std::getline(help_file, help_line_);
		cout << help_line_ << endl;
	}
}

std::string parce_url(std::string path)
{
	ifstream url_file;
	url_file.open(string(path + ".url"));

	if (!url_file.is_open())
	{
		throw std::runtime_error("error opening: .url");
	}

	string script_url_;

	std::getline(url_file, script_url_);
	if (script_url_.size() == 0) throw std::runtime_error("error parcing .url");

	return script_url_;
}

std::string get_exec_dir(std::string path)
{
	auto tmp = path.find_last_of('\\');
	auto tmp2 = path.size();
	path.erase(path.end() - (path.size()- path.find_last_of('\\') - 1), path.end());
	return path;
}

void parce_ini(std::string path)
{
	INIReader DefaultValues(path + "DefaultValues.ini");

	db_ = DefaultValues.Get("sql", "db", "UNKNOWN");

	fetch_cmd_ = DefaultValues.Get("sql", "fetch_cmd", "UNKNOWN");

	action_str_delimmer_ = DefaultValues.Get("sql", "action_str_delimmer", "UNKNOWN");

	if (DefaultValues.ParseError() != 0)
	{
		throw std::runtime_error("parce error ini " + DefaultValues.ParseError());
	}
}