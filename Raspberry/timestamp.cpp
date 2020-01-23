#include <ctime>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <errno.h>
#include <unistd.h>

#include "timestamp.h"

// use strftime to format time_t into a "date time"
std::string date_time(std::time_t posix)
{
	char buf[20+1]; // big enough for 2015-07-08 10:06:51\0
	std::tm tp = *std::localtime(&posix);
	std::strftime(buf, sizeof(buf), "%F %T", &tp);
	return buf;
}

std::string stamp()
{
	using namespace std;
	using namespace std::chrono;

	auto now = system_clock::now();

	// use microseconds % 1000000 now
	auto us = duration_cast<microseconds>(now.time_since_epoch()) % 1000000;

	std::ostringstream oss;
	oss.fill('0');

	oss << date_time(system_clock::to_time_t(now));
	oss << '.' << setw(6) << us.count();

	return oss.str();
}
