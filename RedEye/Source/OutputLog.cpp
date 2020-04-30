#include "OutputLog.h"

#include "Application.h"
#include "RE_HandleErrors.h"

#include <windows.h>
#include <stdio.h>

#define WIN32_MEAN_AND_LEAN
#define LOG_STATEMENT_MAX_LENGTH 512

void _log(const int category, const char file[], int line, const char* format, ...)
{
	static char tmp_string[LOG_STATEMENT_MAX_LENGTH];
	static char tmp_string2[LOG_STATEMENT_MAX_LENGTH];
	static va_list  ap;

	// Construct the string from variable arguments
	va_start(ap, format);
	vsprintf_s(tmp_string, LOG_STATEMENT_MAX_LENGTH, format, ap);
	va_end(ap);
	sprintf_s(tmp_string2, LOG_STATEMENT_MAX_LENGTH, "\n%s(%d) : %s", file, line, tmp_string);
	OutputDebugString(tmp_string2);

	if (App != nullptr)
	{
		switch (LogCategory(category))
		{
		case L_SEPARATOR: sprintf_s(tmp_string2, LOG_STATEMENT_MAX_LENGTH, "\n\n=============\t%s\t=============", tmp_string); break;
		case L_GLOBAL: sprintf_s(tmp_string2, LOG_STATEMENT_MAX_LENGTH, "\n%s", tmp_string); break;
		case L_SECONDARY: sprintf_s(tmp_string2, LOG_STATEMENT_MAX_LENGTH, "\n\t- %s", tmp_string); break;
		case L_TERCIARY: sprintf_s(tmp_string2, LOG_STATEMENT_MAX_LENGTH, "\n\t\t+ %s", tmp_string); break;
		case L_ERROR: sprintf_s(tmp_string2, LOG_STATEMENT_MAX_LENGTH, "\nERROR: %s", tmp_string); break;
		case L_WARNING: sprintf_s(tmp_string2, LOG_STATEMENT_MAX_LENGTH, "\nWARNING: %s", tmp_string); break;
		case L_SOLUTION: sprintf_s(tmp_string2, LOG_STATEMENT_MAX_LENGTH, "\Solution: %s", tmp_string); break;
		case L_SOFTWARE: sprintf_s(tmp_string2, LOG_STATEMENT_MAX_LENGTH, "\n\t* 3rd party software report: %s", tmp_string); break;
		}
		
		App->Log(category, tmp_string2, file);

		if (App->handlerrors != nullptr) {
			App->handlerrors->HandleLog(tmp_string2, LogCategory(category));
		}
	}
}

void _RequestBrowser(const char* link)
{
	ShellExecute(NULL, "open", link, NULL, NULL, SW_SHOWNORMAL);
}

OutputLogHolder::OutputLogHolder(const char * log_dir) : dir(log_dir)
{}

OutputLogHolder::~OutputLogHolder()
{}

void OutputLogHolder::Add(int category, const char * text, const char* file)
{
	eastl::string file_path = file;
	eastl::string file_name = file_path.substr(file_path.find_last_of("\\") + 1);

	eastl::map<eastl::string, unsigned int>::iterator caller_id = callers.find(file_name);

	if (caller_id != callers.end())
	{
		logHistory.push_back(RE_Log(caller_id->second, LogCategory(category), text));
	}
	else
	{
		callers.insert(eastl::pair<eastl::string, unsigned int>(file_name, next_caller_id ));
		logHistory.push_back(RE_Log(next_caller_id, LogCategory(category), text));
		next_caller_id++;
	}
}

void OutputLogHolder::SaveLogs()
{
	// TODO save log to file
}

RE_Log::RE_Log(unsigned int id, LogCategory cat, const char* text) :
	caller_id(id), category(cat)
{
	if (text != nullptr) data = text;
}
