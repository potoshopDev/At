#include "GetDate.h"

#include <chrono>

std::string Date::getDMY(const int days) {
	using namespace std::chrono;

	auto now = system_clock::now();
	auto next = now + hours(days*24);
	auto in_time_t = std::chrono::system_clock::to_time_t(next);

	struct tm buf;
	localtime_s(&buf, &in_time_t);

	return std::format("{:02}.{:02}.{:04}",
		buf.tm_mday, buf.tm_mon + 1, buf.tm_year + 1900);
}

std::string Date::getHM(const int minutes)
{
	using namespace std::chrono;

	auto now = system_clock::now();
	auto next = now + std::chrono::minutes(minutes);
	auto in_time_t = std::chrono::system_clock::to_time_t(next);

	struct tm buf;
	localtime_s(&buf, &in_time_t);

	return std::format("{:02}:{:02}",
		buf.tm_hour, buf.tm_min);
}
