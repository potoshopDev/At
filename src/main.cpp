#include <iostream>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <vector>
#include <sstream>

#include <map>
#include <unordered_map>
#include <functional>
#include <ranges>
#include <print>

#include "screenshot.h"
#include "Logger.h"

using json = nlohmann::json;

// ------------------- Callback libcurl -------------------
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

// ------------------- HTTP POST JSON -------------------
json PostJson(const std::string& url, const json& data) {
	CURL* curl = curl_easy_init();
	std::string readBuffer;

	if (!curl) throw std::runtime_error("Curl init failed");

	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");

	std::string jsonStr = data.dump();

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	CURLcode res = curl_easy_perform(curl);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) throw std::runtime_error("Curl POST failed: " + std::string(curl_easy_strerror(res)));

	return json::parse(readBuffer);
}

// ------------------- HTTP GET JSON -------------------
json GetJson(const std::string& url) {
	CURL* curl = curl_easy_init();
	std::string readBuffer;

	if (!curl) throw std::runtime_error("Curl init failed");

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) throw std::runtime_error("Curl GET failed: " + std::string(curl_easy_strerror(res)));

	return json::parse(readBuffer);
}

// ------------------- Wait for page load -------------------
void WaitForPageLoad(const std::string& baseUrl, const std::string& sessionId, int timeout_ms = 10000) {
	int elapsed = 0;
	const int step = 200;

	while (elapsed < timeout_ms) {
		try {
			json resp = PostJson(baseUrl + "/session/" + sessionId + "/execute/sync",
				{ {"script", "return document.readyState;"},
				 {"args", json::array()} });

			if (resp.contains("value") && resp["value"].get<std::string>() == "complete") {
				return;
			}
			else Logger::Log(Logger::Level::Info, "Ожидаю загрузку страницы.");
		}
		catch (...) {}
		std::this_thread::sleep_for(std::chrono::milliseconds(step));
		elapsed += step;
	}

	throw std::runtime_error("Страница не успела загрузиться за timeout");
}

// ------------------- Check element visibility via JS -------------------
bool IsElementVisible(const std::string& baseUrl, const std::string& sessionId, const std::string& xpath) {
	json resp = PostJson(baseUrl + "/session/" + sessionId + "/execute/sync",
		{ {"script",
		  "const el = document.evaluate(\"" + xpath + "\", document, null, XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;"
		  "return el != null && el.offsetParent !== null;"},
		 {"args", json::array()} });
	return resp.contains("value") && resp["value"].get<bool>();
}

// ------------------- Wait until element visible -------------------
std::string WaitForElementVisible(const std::string& baseUrl, const std::string& sessionId, const std::string& xpath, int timeout_ms = 10000) {
	int elapsed = 0;
	const int step = 200;

	Logger::Log(Logger::Level::Info, "Поиск элемента: " + xpath);

	while (elapsed < timeout_ms) {
		try {
			if (IsElementVisible(baseUrl, sessionId, xpath)) {
				json resp = PostJson(baseUrl + "/session/" + sessionId + "/element",
					{ {"using", "xpath"}, {"value", xpath} });
				return resp["value"]["element-6066-11e4-a52e-4f735466cecf"];
			}
		}
		catch (...) {}
		std::this_thread::sleep_for(std::chrono::milliseconds(step));
		elapsed += step;
	}

	throw std::runtime_error("Элемент не найден или не видим: " + xpath);
}

struct Command {
	std::string action;
	std::string target;
	std::string value;
};

std::vector<Command> LoadScript(const std::string& filename) {
	std::vector<Command> commands;
	std::ifstream file(filename);
	std::string line;
	while (std::getline(file, line)) {
		std::istringstream ss(line);
		Command cmd;
		std::getline(ss, cmd.action, '|');
		std::getline(ss, cmd.target, '|');
		std::getline(ss, cmd.value);
		commands.push_back(cmd);
	}
	return commands;
}

void Navigate(const std::string& seleniumUrl, const std::string& sessionId, const std::string& targetUrl, const int timeout_ms = 10000)
{
	Logger::Log(Logger::Level::Info, "Перехожу по URL " + targetUrl);

	PostJson(seleniumUrl + "/session/" + sessionId + "/url",
		{ {"url", targetUrl} });

	WaitForPageLoad(seleniumUrl, sessionId, timeout_ms);
}

std::string FindElementByXPath(const std::string& seleniumUrl, const std::string& sessionId, const std::string& target, const int timeout_ms = 2000)
{
	try {
		Logger::Log(Logger::Level::Info, "Ищу элемент на странице: " + target);

		const auto result{ WaitForElementVisible(
			seleniumUrl, sessionId,
			target,
			timeout_ms
		)
		};

		return result;
	}

	catch (const std::exception& e) {
		Logger::Log(Logger::Level::Error, "Не найден: " + target);
		std::cerr << "Ошибка: " << e.what() << std::endl;
	}
	catch (...) {
		Logger::Log(Logger::Level::Error, "Не найден: " + target);
		std::cerr << "Ошибка: " << target << std::endl;
	}

	throw std::runtime_error("Критическая ошибка поиска: " + target);
}

// ------------------- Storage -------------------
std::unordered_map<std::string, std::string> storage;

std::string GetElementText(const std::string& seleniumUrl, const std::string& sessionId, const std::string& target)
{
	json resp = GetJson(seleniumUrl + "/session/" + sessionId + "/element/" + target + "/property/value");
	return resp.contains("value") ? resp["value"].get<std::string>() : "";
}
std::string LoadKey(const std::string& key)
{
	if (key.rfind("@@", 0) == 0) {
		auto it = storage.find(key);
		if (it != storage.end()) return it->second;

		Logger::Log(Logger::Level::Error, "Не найден ключ: " + key);
		throw std::runtime_error("Ключ " + key + " не найден в storage");
	}
	return key;
}
// ------------------- Check element value -------------------
void CheckKey(const std::string& seleniumUrl, const std::string& sessionId, const std::string& target, const Command& cmd) {

	const auto actual{ GetElementText(seleniumUrl, sessionId, target) };
	const auto current{ LoadKey(cmd.value) };

	if (actual == current)
	{
		Logger::Log(Logger::Level::Info, "Поде " + cmd.target + "имеет ожидаемое значение: " + current);
	}
	else {
		Logger::Log(Logger::Level::Warn, "Проверка не прошла: у элемента " + cmd.target + "ожидалось: " + current + " -  текущие: " + actual);
	}
}

void ClickElement(const std::string seleniumUrl, const std::string sessionId, const std::string target)
{
	Logger::Log(Logger::Level::Info, "Кликаю на " + target);
	PostJson(seleniumUrl + "/session/" + sessionId + "/element/" + target + "/click", json::object());
}

void SendKeys(const std::string seleniumUrl, const std::string sessionId, const std::string target, const std::string text)
{
	Logger::Log(Logger::Level::Info, "Пишу в поле: " + target + " значение: " + text);
	PostJson(seleniumUrl + "/session/" + sessionId + "/element/" + target + "/value",
		{ {"text", text} });
}



int GetTimeout(const Command& cmd)
{
	auto timeout{ 10000 };
	try {
		if (!cmd.value.empty())
			timeout = std::stoi(cmd.value);

	}
	catch (...) {
		std::cerr << "Некорректное значение таймаута: " << cmd.value << std::endl;
	}
	return timeout;
}

// ------------------- Main -------------------
int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "ru_RU.UTF-8");

	Logger::Init();

	const std::string seleniumUrl = "http://localhost:4444";

	// 1. Создаём сессию Chrome
	json caps = {
		{"capabilities", {{"alwaysMatch", {{"browserName", "chrome"}}}}}
	};
	json sessionResp = PostJson(seleniumUrl + "/session", caps);
	std::string sessionId = sessionResp["value"]["sessionId"];

	Logger::Log(Logger::Level::Info, "Запускаю selenium");
	Logger::Log(Logger::Level::Info, "Session ID: " + sessionId);

	std::map<std::string, std::function<void(const Command& cmd)>> actions;
	actions["goto"] = [&](const Command& cmd)
		{ Navigate(seleniumUrl, sessionId, cmd.target); };
	actions["click"] = [&](const Command& cmd)
		{
			const auto element{ FindElementByXPath(seleniumUrl, sessionId, cmd.target) };
			ClickElement(seleniumUrl, sessionId, element);
		};
	actions["input"] = [&](const Command& cmd)
		{
			const auto element{ FindElementByXPath(seleniumUrl, sessionId, cmd.target) };
			SendKeys(seleniumUrl, sessionId, element, LoadKey(cmd.value));
		};
	actions["check"] = [&](const Command& cmd)
		{
			const auto element{ FindElementByXPath(seleniumUrl, sessionId, cmd.target) };
			CheckKey(seleniumUrl, sessionId, element, cmd);
		};
	actions["waitp"] = [&](const Command& cmd)
		{
			const auto timeout{ GetTimeout(cmd) };
			WaitForPageLoad(seleniumUrl, sessionId, timeout);

			const auto msg{ std::format("Ожидание загрузки страницы завершено (таймаут {}мс", timeout) };
			Logger::Log(Logger::Level::Info, msg);
		};
	actions["wait"] = [&](const Command& cmd)
		{
			const auto timeout{ GetTimeout(cmd) };
			std::this_thread::sleep_for(std::chrono::milliseconds(timeout));

			const auto msg{ std::format("Ожидание завершено (таймаут {}мс", timeout) };
			Logger::Log(Logger::Level::Info, msg);
		};
	actions["save"] = [&](const Command& cmd)
		{
			const auto element{ FindElementByXPath(seleniumUrl, sessionId, LoadKey(cmd.target)) };
			const auto text = GetElementText(seleniumUrl, sessionId, element);
			storage[cmd.value] = text;

			const auto msg{ std::format("Сохранено значение: {} = {}", cmd.value, storage.at(cmd.value)) };
			Logger::Log(Logger::Level::Info, msg);
		};

	if (argc < 2) {
		const auto msg{ std::format("Внимание: не указан путь к файлу со сценарием.\nПример: ./app script.txt\nБудет использовать стандартный сценарий!") };
		Logger::Log(Logger::Level::Info, msg);
	}

	const auto firstScript{ 1 };
	const auto lastScript{ argc < 2 ? 2 : argc };

	for (const auto& i : std::views::iota(firstScript, lastScript))
	{
		const auto scriptPath = argc < 2 ? "script.txt" : argv[i];
		const auto script = LoadScript(scriptPath);

		for (const auto& cmd : script)
		{
			if (actions.find(cmd.action) != actions.end())
			{
				try
				{
					actions[cmd.action](cmd);
				}
				catch (const std::exception& e)
				{
					const auto msg{ std::string(e.what()) };
					Logger::Log(Logger::Level::Error, msg);

					std::this_thread::sleep_for(std::chrono::milliseconds(1000));

					Logger::Log(Logger::Level::Warn, "Делаю скриншот главного окна");
					CaptureFullScreenAndSave(L"D:\\Repos\\tmp2\\AT\\build\\bin\\Debug\\log\\screenshots");
					Logger::Log(Logger::Level::Info, "Session ID: " + sessionId);
					Logger::Log(Logger::Level::Info, "------------------------------------------------");
					return 1;
				}
			}
			else
			{
				Logger::Log(Logger::Level::Error, "Неизвестная команда: " + cmd.action);
			}
		}
		Logger::Log(Logger::Level::Info, "Сценарий завершен: " + std::string(scriptPath));

	}

	Logger::Log(Logger::Level::Info, "Session ID: " + sessionId);
	Logger::Log(Logger::Level::Info, "\n\n\t!!!!!!!!!Автотесты завершены!!!!!!!!!!!!");
}
