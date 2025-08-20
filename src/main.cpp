#include <iostream>
#include <thread>
#include <chrono>
#include <curl/curl.h>

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
#include "GetDate.h"
#include "winsystem.h"
#include "scanelements.h"
#include "json.h"



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

	namespace fs = std::filesystem;
	const auto fullPathToFile{ win::getFullPath(filename) };

	std::ifstream file(fullPathToFile);
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

void Navigate(const std::string& seleniumUrl, const std::string& sessionId, const std::string& targetUrl, const std::string& path, const int timeout_ms = 10000)
{
	Logger::Log(Logger::Level::Info, "Перехожу по URL " + targetUrl);

	PostJson(seleniumUrl + path + "/session/" + sessionId + "/url",
		{ {"url", targetUrl} });

	WaitForPageLoad(seleniumUrl, sessionId, timeout_ms);
}

std::string FindElementByXPath(const std::string& seleniumUrl, const std::string& sessionId, const std::string& target, const int timeout_ms = 10000)
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
	//json resp = GetJson(seleniumUrl + "/session/" + sessionId + "/element/" + target + "/property/value");
	//return resp.contains("value") ? resp["value"].get<std::string>() : "";

	auto tryGet = [&](const std::string& endpoint, const std::string& label) -> std::string {
		try {
			json resp = GetJson(seleniumUrl + "/session/" + sessionId + "/element/" + target + endpoint);
			if (resp.contains("value") && !resp["value"].is_null()) {
				auto txt = resp["value"].get<std::string>();
				if (!txt.empty()) {
					Logger::Log(Logger::Level::Info, "Удалось найти текст: " + label + " : " + txt);
					return txt;
				}
			}
		}
		catch (const std::exception& e) {
			const std::string msg(e.what());
			Logger::Log(Logger::Level::Warn, "Не удалось найти текст: " + label + " : " + msg);
		}
		return "";
		};

	// Сначала пробуем value (для input/textarea)
	auto text = tryGet("/property/value", "value");
	if (!text.empty())
		return text;

	// Потом пробуем inner text (для div/a/span/etc.)
	text = tryGet("/text", "text");
	if (!text.empty())
		return text;

	throw std::runtime_error("Не удалось найти и сохранить текст из: " + target);

	return "";
}
bool CheckXPath(const std::string& key)
{
	return key.starts_with("//");
}
bool CheckKey(const std::string& key)
{
	return key.starts_with("@@");
}
std::string LoadKey(const std::string& key)
{
	if (CheckKey(key)) {
		auto it = storage.find(key);
		if (it != storage.end()) {
			Logger::Log(Logger::Level::Info, "Раскрыл псевдоним: '" + key + "' на '" + it->second + "'");
			return it->second;
		}

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

void SelectOptionJava(const std::string& seleniumUrl, const std::string& sessionId, const std::string& comboXPath, const std::string& optionValue)
{
	const auto comboElement{ FindElementByXPath(seleniumUrl, sessionId, comboXPath) };
	ClickElement(seleniumUrl, sessionId, comboElement);

	Logger::Log(Logger::Level::Info, "Выбран элемент: " + optionValue);

}
void SelectComboBoxOptionJava(const std::string& seleniumUrl, const std::string& sessionId, const std::string& comboXPath, const std::string& optionValue)
{
	Logger::Log(Logger::Level::Info, "Выбираю элемент в комбо-боксе: " + comboXPath + " значение: " + optionValue);

	// Находим сам комбо-бокс
	const auto comboElement{ FindElementByXPath(seleniumUrl, sessionId, comboXPath) };
	ClickElement(seleniumUrl, sessionId, comboElement);

	// JS-скрипт для выбора значения в <select>
	std::string script =
		"const select = document.evaluate(\"" + comboXPath + "\", document, null, XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;"
		"if (select) {"
		"  for (let i = 0; i < select.options.length; i++) {"
		"    if (select.options[i].text === '" + optionValue + "') {"
		"      select.selectedIndex = i;"
		"      select.dispatchEvent(new Event('change', { bubbles: true }));"
		"      return true;"
		"    }"
		"  }"
		"}"
		"return false;";

	json resp = PostJson(seleniumUrl + "/session/" + sessionId + "/execute/sync",
		{ {"script", script}, {"args", json::array()} });

	if (!resp.contains("value") || !resp["value"].get<bool>())
	{
		throw std::runtime_error("Не удалось выбрать значение '" + optionValue + "' в комбо-боксе: " + comboXPath);
	}

	Logger::Log(Logger::Level::Info, "Выбран элемент: " + optionValue);
}

// ------------------- Main -------------------
int main(int argc, char* argv[]) {
	Logger::Init();
	setlocale(LC_ALL, "ru_RU.UTF-8");


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
		{ Navigate(seleniumUrl, sessionId, LoadKey(cmd.target), LoadKey(cmd.value)); };
	actions["click"] = [&](const Command& cmd)
		{
			const auto element{ FindElementByXPath(seleniumUrl, sessionId, LoadKey(cmd.target)) };
			ClickElement(seleniumUrl, sessionId, element);
			Logger::Log(Logger::Level::Info, "Кликнул на: " + LoadKey(cmd.target));
		};
	actions["input"] = [&](const Command& cmd)
		{
			const auto element{ FindElementByXPath(seleniumUrl, sessionId, LoadKey(cmd.target)) };
			SendKeys(seleniumUrl, sessionId, element, LoadKey(cmd.value));
		};
	actions["check"] = [&](const Command& cmd)
		{
			const auto element{ FindElementByXPath(seleniumUrl, sessionId, LoadKey(cmd.target)) };
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
	auto SaveKey = [&](const std::string& value, const std::string& target, bool isLogging = true)
		{
			std::string text{ value };
			std::string key{ target };

			auto getTextToKey = [&]()
				{
					const auto element{ FindElementByXPath(seleniumUrl, sessionId, LoadKey(key)) };
					text = GetElementText(seleniumUrl, sessionId, element);
				};

			if (CheckKey(text))
			{
				std::swap(key, text);
				if (CheckXPath(LoadKey(key))) 
					getTextToKey();
				else
					text = storage.at(key);

				key = target;
			}
			else if (!CheckKey(key))
			{
				getTextToKey();
				key = value;
			}



			storage[key] = text;

			if (isLogging)
			{
				const auto msg{ std::format("Сохранено значение: {} = {}", key, storage.at(key)) };
				Logger::Log(Logger::Level::Info, msg);
			}
		};
	actions["save"] = [&](const Command& cmd)
		{
			SaveKey(cmd.value, cmd.target);
		};
	actions["cache"] = [&](const Command& cmd)
		{
			const auto isLogging{ false };
			SaveKey(cmd.value, cmd.target, isLogging);
		};
	actions["#"] = [&](const Command& cmd)
		{
		};
	actions["print"] = [&](const Command& cmd)
		{
			Logger::Log(Logger::Level::Info, LoadKey(cmd.target) + " " + LoadKey(cmd.value));
		};
	actions["printw"] = [&](const Command& cmd)
		{
			Logger::Log(Logger::Level::Warn, LoadKey(cmd.target) + " " + LoadKey(cmd.value));
		};
	actions["printe"] = [&](const Command& cmd)
		{
			Logger::Log(Logger::Level::Error, LoadKey(cmd.target) + " " + LoadKey(cmd.value));
		};
	actions["getd"] = [&](const Command& cmd)
		{
			const auto dateAdjustment{ cmd.value.empty() ? 0 : GetTimeout(cmd) };
			const auto rdate{ Date::getDMY(dateAdjustment) };
			SaveKey(rdate, cmd.target);
		};
	actions["geth"] = [&](const Command& cmd)
		{
			const auto dateAdjustment{ cmd.value.empty() ? 0 : GetTimeout(cmd) };
			const auto rdate{ Date::getHM(dateAdjustment) };
			SaveKey(rdate, cmd.target);
		};
	actions["selectjava"] = [&](const Command& cmd)
		{
			SelectComboBoxOptionJava(seleniumUrl, sessionId, LoadKey(cmd.target), LoadKey(cmd.value));
		};
	actions["clicktext"] = [&](const Command& cmd)
		{
			SelectOptionJava(seleniumUrl, sessionId, LoadKey(cmd.target), LoadKey(cmd.value));
		};
	actions["scan"] = [&](const Command& cmd)
		{
			LOG_INFO(seleniumUrl);
			Scan::LogInteractiveElements(seleniumUrl, sessionId);
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
					CaptureFullScreenAndSave();
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
