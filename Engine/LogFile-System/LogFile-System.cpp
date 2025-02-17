//
// LogFile-System.cpp - Manages the LogFile system and its class.
//
#include "LogFile-System.h"
#include <time.h>
#include "../../Core/ZeeTerminalCore.h"
#include <chrono>
#include <fstream>

// LogFileSystem - Manages ZeeTerminal logging.

// InitialiseTimeStructs
bool LogFileSystem::InitialiseTimeStructs()
{
	// Variables
	time_t currentTime = time(0);

	// Initialise time
	time(&currentTime);

	// Initialise time struct
	if (localtime_s(&tmLocalTimeStart, &currentTime)) {
		VerbosityDisplay("In LogFileSystem::InitialiseTimeStructs(): ERROR - localtime_s failed when getting program start time.\n", nObjectID, false, true);
		return false;
	}

	// Initialise millisecond time
	nStartMillisecondValue = GetCurrentMillisecondTime();

	bTimeStructsInitialised = true;

	return true;
}

// GetCurrentMillisecondTime
time_t LogFileSystem::GetCurrentMillisecondTime()
{
	// Get current system time
	auto now = std::chrono::system_clock::now();

	// Convert system time into seconds
	auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);

	// Subtract all seconds from current system time to leave exact millisecond value
	auto FractionalTime = now - seconds;

	// Cast the time to milliseconds
	auto MillisecondValue = std::chrono::duration_cast<std::chrono::milliseconds>(FractionalTime);

	// Return the count of the final millisecond value variable
	return MillisecondValue.count();
}

// ConvertValueToTimeFormat
std::string LogFileSystem::ConvertValueToTimeFormat(time_t nValue, int nValueWidth)
{
	// 1. Convert value to string immediately
	std::string sStartingValueString = std::to_string(nValue);
	std::string sEndValueString = "";

	// 2. Get number of characters in string. If that is smaller than nValueWidth, add 0s to front of string
	if (sStartingValueString.length() < nValueWidth) {
		long long int nNumOfZeroes = nValueWidth - sStartingValueString.length();
		std::string sZeroBuffer = std::string("0", nNumOfZeroes);
		sEndValueString = sZeroBuffer + sStartingValueString;
	}
	else sEndValueString = sStartingValueString;

	return sEndValueString;
}

// ConvertTimeStructToString
std::string LogFileSystem::ConvertTimeStructToString(struct tm tmStructArg, time_t nMillisecondValue, bool bIncludeTime, char cDateSeparator) {
	std::string sTimeString = "";

	// Get Date Info
	sTimeString = std::to_string(tmStructArg.tm_year + 1900) + cDateSeparator + ConvertValueToTimeFormat(tmStructArg.tm_mon + 1) + cDateSeparator + ConvertValueToTimeFormat(tmStructArg.tm_mday);

	// Get time info if wanted
	if (bIncludeTime) {
		sTimeString += " " + ConvertValueToTimeFormat(tmStructArg.tm_hour) + ":" + ConvertValueToTimeFormat(tmStructArg.tm_min) + ":" + ConvertValueToTimeFormat(tmStructArg.tm_sec) + "." + ConvertValueToTimeFormat(nMillisecondValue, 3);
	}

	return sTimeString;
}

// GetEntryTypesOfLogFileString
std::string LogFileSystem::GetEntryTypesOfLogFileString()
{
	std::string sInfoInLog = "";
	// Get info to be put in log
	if (ConfigObjMain.bVerboseMessageLogging == true) {
		sInfoInLog += "Verbose Messages, ";
	}
	if (ConfigObjMain.bUserSpaceErrorLogging == true) {
		sInfoInLog += "User-Space Error Messages, ";
	}
	if (ConfigObjMain.bCommandInputInfoLogging == true) {
		sInfoInLog += "Command Input Info, ";
	}
	if (ConfigObjMain.bUserInputInfoLogging == true) {
		sInfoInLog += "User Input Info, ";
	}

	// As there will always be 2 trailing, unnecessary characters in string, replace with full stop
	if (sInfoInLog.length() > 0) {
		sInfoInLog.pop_back();
		sInfoInLog.pop_back();
		sInfoInLog.push_back('.');
	}

	return sInfoInLog;
}

// UpdateEntryTypesLine
bool LogFileSystem::UpdateEntryTypesLine() {
	std::ifstream LogFileStreamIn(sLogFileName);
	std::string sFileContents = "";
	std::string sOldEntryLine = "";

	// Check for failure
	if (LogFileStreamIn.fail()) {
		VerbosityDisplay("In LogFileSystem::UpdateEntryTypesLine(): ERROR - Unknown error on std::ifstream init to log file. Attempting to create new log file...\n", nObjectID, false);

		// Attempt to create log file
		if (!CreateLogFile()) {
			VerbosityDisplay("In LogFileSystem::UpdateEntryTypesLine(): ERROR - Failed to create log file on LogFileSystem::CreateLogFile().\n", nObjectID, false);
			return false;
		}

		// Technically, the log file should be updated already with new settings due to CreateLogFile(), so just return TRUE at this point
		return true;
	}

	// Read up to the 16th line
	bool bCheckForEntryTypesLine = true;
	while (!LogFileStreamIn.eof()) {
		std::string sBuffer = "";
		std::getline(LogFileStreamIn, sBuffer, '\n');

		// Skip the 16th line
		if (sBuffer.find("# - Possible entry types in this log:") == std::string::npos || bCheckForEntryTypesLine == false) {
			sFileContents += sBuffer + "\n";
		}
		else if (bCheckForEntryTypesLine == true) {
			sOldEntryLine = sBuffer.substr(sBuffer.find("# - Possible entry types in this log:") + 38, std::string::npos);
			// In place of the 16th line, use the new updated entry types string
			sFileContents += "# - Possible entry types in this log: " + GetEntryTypesOfLogFileString() + "\n";
			// Do not check for another one of these lines, as it may be user input (first one of these entry types lines is always part of the log file context)
			bCheckForEntryTypesLine = false;
		}
	}

	// Remove additional newline character that was added in this line: sFileContents += sBuffer + "\n";
	sFileContents.pop_back();

	// Close input stream
	LogFileStreamIn.close();

	// Overwrite file with updated contents
	std::ofstream LogFileStreamOut(sLogFileName);

	// Check for failure
	if (LogFileStreamOut.fail()) {
		VerbosityDisplay("In LogFileSystem::UpdateEntryTypesLine(): ERROR - Unknown error on std::ofstream init to log file. Attempting to create new log file...\n", nObjectID, false);

		// Attempt to create log file
		if (!CreateLogFile()) {
			VerbosityDisplay("In LogFileSystem::UpdateEntryTypesLine(): ERROR - Failed to create log file on LogFileSystem::CreateLogFile().\n", nObjectID, false);
			return false;
		}

		// Technically, the log file should be updated already with new settings due to CreateLogFile(), so just return TRUE at this point
		return true;
	}

	// Output to file
	LogFileStreamOut << sFileContents;
	LogFileStreamOut.close();

	// Log the change
	VerbosityDisplay("In LogFileSystem::UpdateEntryTypesLine(): INFO - Entry Types Line in this Log File has been updated from [" + sOldEntryLine + "] to [" + GetEntryTypesOfLogFileString() + "].", nObjectID, true);

	// Exit
	return true;
}

// InitialiseLogFileObject
bool LogFileSystem::InitialiseLogFileObject()
{
	// DO NOT initialise if already initialised
	if (bObjectInitialised == true) {
		return true;
	}

	// Tasks //
	// 
	// 1. Initialise Time Structures
	if (!InitialiseTimeStructs()) {
		return false;
	}

	// 2. Update program info string
	UpdateProgramInfo();

	// 3. Update system info string
	UpdateSystemInfo();

	// Object has been initialised
	bObjectInitialised = true;

	// Both config object and log file object have been initialised by this point
	bConfigAndLogSystemsInitialised = true;

	// As logging for both the RGB and config systems were disabled due to systems not initialised, log messages that they are now initialised
	VerbosityDisplay("RGBColourPreset Object Created.", 10001, true);
	VerbosityDisplay("RGBColourPreset Object Created.", 10002, true);
	VerbosityDisplay("RGBColourPreset Object Created.", 10003, true);
	VerbosityDisplay("ConfigFileSystem Object has been created.", 10001, true);

	// Send message that object is being created - everything else has been initialised, it should be safe
	VerbosityDisplay("A LogFileSystem Object has been created.\n", nObjectID);

	return true;
}

// UpdateProgramInfo
void LogFileSystem::UpdateProgramInfo()
{
	// To get info for screen buffer height
	CONSOLE_SCREEN_BUFFER_INFO csbiProgramInfo;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbiProgramInfo);

	// Update Program Info string with necessary functions
	sProgramInfoString = "# __ZeeTerminal Information__\n# - ZeeTerminal Version: " + std::string(ZT_VERSION)
		+ "\n# - ZeeTerminal Start Time/Date: " + GetStartTimeAsString(true)
		+ "\n# - Terminal ANSI Support: " + BoolToString(EnableVTMode())
		+ "\n# - Terminal Screen Buffer Height: " + std::to_string(csbiProgramInfo.dwSize.Y) + " lines"
		+ "\n# - Possible entry types in this log: " + GetEntryTypesOfLogFileString() + "\n";

	return;
}

// CreateLogFile
bool LogFileSystem::CreateLogFile()
{
	// Initialise object if not initialised
	if (!InitialiseLogFileObject()) {
		return false;
	}

	// 1. For loop for checking for good log file title
	for (unsigned long long int i = 0; i < std::numeric_limits<unsigned long long int>::max(); i++) {
		// a. Declare ifstream to title + date + i
		std::string sTestTitle = "ZeeTerminal_Log_" + GetStartTimeAsString(false, '-') + "_" + std::to_string(i) + ".txt";
		std::ifstream TestIn(sTestTitle);

		// b. If good (file found), continue. Else set global log file string to test string and break
		if (!TestIn.fail())
		{
			continue;
		}
		else
		{
			sLogFileName = sTestTitle;
			break;
		}
	}

	// 2. Declare ofstream to global log title
	std::ofstream CreateLogFileOut(sLogFileName);

	// Exit on failure
	if (CreateLogFileOut.fail()) {
		VerbosityDisplay("In LogFileSystem::CreateLogFile(): ERROR - Unknown error on std::ofstream init to log file. Failed to create log file.\n", nObjectID, false, true);

		// Log to Windows Log Database
		std::vector<std::string> vsErrorInfo{
			"In LogFileSystem::CreateLogFile(): ERROR - Unknown error on std::ofstream init to log file. Failed to create log file.",
			"Context: Failed to create and write to new log file, due to out-stream failure.",
			"Object ID: " + std::to_string(nObjectID),
			"Category: Verbose Message (Error)",
			"Windows error code info: Code " + std::to_string(GetLastError())
		};
		LogWindowsEvent(vsErrorInfo, 0, NULL);

		return false;
	}

	// Update system, program strings
	UpdateSystemInfo();
	UpdateProgramInfo();

	// 3. Write global log title surrounded with "___", and system/program info
	CreateLogFileOut << "# ___" + sLogFileName + "___\n\n" << sSystemInfoString << "\n" << sProgramInfoString << "\n\n# __LOG START__\n# FORMAT: <Time> <Time since program execution> <Object ID> <Message Type> <Info/Message>\n# If the Object ID of a line equates to 10000, there is no object associated with that line.\n# Anything higher than that means that the line is associated with an object.\n\n";

	// 4. Exit
	CreateLogFileOut.close();
	return true;
}

// Constructor
LogFileSystem::LogFileSystem()
{
	static int nStaticID = 10000;
	// Wrap-around to prevent overflow
	if (nStaticID >= std::numeric_limits<int>::max() - 1) nStaticID = 10000;
	nObjectID = ++nStaticID;

	// Initialise object
	InitialiseLogFileObject();

	return;
}

// Destructor
LogFileSystem::~LogFileSystem()
{
	// Send message that object is being destroyed
	VerbosityDisplay("A LogFileSystem Object has been destroyed.\n", nObjectID);

	// Send messages that the objects are being destroyed before logging disable
	VerbosityDisplay("ConfigFileSystem Object has been destroyed.", 10001, true);
	VerbosityDisplay("RGBColourPreset Object Destroyed.", 10003, true);
	VerbosityDisplay("RGBColourPreset Object Destroyed.", 10002, true);
	VerbosityDisplay("RGBColourPreset Object Destroyed.", 10001, true);

	// Fully disable logging no matter what, as systems are no longer initialised
	bConfigAndLogSystemsInitialised = false;

	return;
}

// AddLogLine
bool LogFileSystem::AddLogLine(std::string sLogLine, short int nTypeOfLine, int nObjectID)
{
	// Initialise object if not initialised
	if (!InitialiseLogFileObject()) {
		return false;
	}

	// Do not log if logging disabled
	if (!ConfigObjMain.bEnableLogging) return true;

	// Check type of line - if user disabled that line, do not log
	if ((nTypeOfLine == 1 && !ConfigObjMain.bVerboseMessageLogging) ||
		(nTypeOfLine == 2 && !ConfigObjMain.bUserSpaceErrorLogging) ||
		(nTypeOfLine == 3 && !ConfigObjMain.bCommandInputInfoLogging) ||
		(nTypeOfLine == 4 && !ConfigObjMain.bUserInputInfoLogging)
		)
	{
		return true;
	}

	// 1. Check for log file. If not found, create new one. Exit on failure.
	std::ifstream LogFileTestIn(sLogFileName);
	if (LogFileTestIn.fail()) {
		// Attempt to create log file
		if (!CreateLogFile()) {
			VerbosityDisplay("In LogFileSystem::AddLogFile(): ERROR - Failed to create log file on LogFileSystem::CreateLogFile().\n", nObjectID, false, true);

			// Log to Windows Log Database
			std::vector<std::string> vsErrorInfo{
				"In LogFileSystem::AddLogFile(): ERROR - Failed to create log file on LogFileSystem::CreateLogFile().",
				"Context: Failed to create log file after std::ifstream failed to detect a known log file.",
				"Object ID: " + std::to_string(nObjectID),
				"Category: Verbose Message (Error)",
				"Windows error code info: Code " + std::to_string(GetLastError())
			};
			LogWindowsEvent(vsErrorInfo, 0, NULL);

			return false;
		}
	}

	// Close test stream
	LogFileTestIn.close();

	// 2. Open log file with append flag
	std::ofstream LogFileOut(sLogFileName, std::ios::app);

	// Unknown error - just exit
	if (LogFileOut.fail()) {
		VerbosityDisplay("In LogFileSystem::AddLogFile(): ERROR - Unknown error on std::ofstream init to log file. Attempting to create new log file...\n", nObjectID, false, true);

		// Attempt to create log file
		if (!CreateLogFile()) {
			VerbosityDisplay("In LogFileSystem::AddLogFile(): ERROR - Failed to create log file on LogFileSystem::CreateLogFile().\n", nObjectID, false, true);

			// Log to Windows Log Database
			std::vector<std::string> vsErrorInfo{
				"In LogFileSystem::AddLogFile(): ERROR - Failed to create log file on LogFileSystem::CreateLogFile().",
				"Context: Failed to create log file after std::ofstream failed to detect a known log file directory.",
				"Object ID: " + std::to_string(nObjectID),
				"Category: Verbose Message (Error)",
				"Windows error code info: Code " + std::to_string(GetLastError())
			};
			LogWindowsEvent(vsErrorInfo, 0, NULL);

			return false;
		}
	}

	// 3. Output line
	//
	// Get current time
	std::time_t currentTime = time(0);
	struct tm tmCurrentTime {};

	// Initialise time
	time_t nCurrentMilSecTime = GetCurrentMillisecondTime();
	time(&currentTime);

	std::string sLogTime = "";

	// Initialise time struct - exit on fail (error code > 0)
	if (localtime_s(&tmCurrentTime, &currentTime)) {
		VerbosityDisplay("In LogFileSystem::AddLogFile(): ERROR - localtime_s failed when getting current time. This log line will not contain current time information.\n", nObjectID, false, true);
	}
	else {
		// Convert time struct into string with millisecond value
		sLogTime = ConvertTimeStructToString(tmCurrentTime, nCurrentMilSecTime, true);
	}

	// Convert current and start time values into decimal seconds, and put them into a string to calculate difference later
	std::string sCurrentTime = std::to_string(currentTime) + "." + std::to_string(nCurrentMilSecTime);
	std::string sStartTime = std::to_string(std::mktime(&tmLocalTimeStart)) + "." + std::to_string(nStartMillisecondValue);

	// Calculate difference by converting string into double, subtracting, and converting back to string.
	std::string sLogDiffTime = std::to_string(std::stold(sCurrentTime) - std::stold(sStartTime)) + " sec";

	// Get log type
	std::string sLogType = "";
	if (nTypeOfLine == 1)
	{
		sLogType = "Verbose Message";
	}
	if (nTypeOfLine == 2)
	{
		sLogType = "User-space Error";
	}
	if (nTypeOfLine == 3)
	{
		sLogType = "Command Input Info";
	}
	if (nTypeOfLine == 4)
	{
		sLogType = "User Input Info";
	}

	// Replace all newlines with spaces in log line, 
	// because if a newline is in the log line, some of the log line will be on another line
	for (size_t i = 0; i < sLogLine.length(); i++) {
		if (sLogLine[i] == '\n') {
			sLogLine[i] = ' '; // replace newline with space
		}
	}

	// if entries line needs updating after setting change, update now
	if (bEntryTypesLineNeedsUpdate) {
		bEntryTypesLineNeedsUpdate = false;
		UpdateEntryTypesLine();
	}

	// Write line
	LogFileOut << "[" + sLogTime + "]   [" + sLogDiffTime + "]   " + std::to_string(nObjectID) + "   " + sLogType + "   " + sLogLine + "\n";

	// Close and exit
	LogFileOut.close();
	return true;
}