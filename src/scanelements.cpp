#include "scanelements.h"
#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "Logger.h"
#include "json.h"

#include <sstream>
#include <algorithm>
#include <locale>
#include <codecvt>
#include <regex>

#include <clocale>
#include <cwctype>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
namespace Scan
{
	std::string CleanUTF8(const std::string& input) {
		std::string out;
		size_t i = 0;
		while (i < input.size()) {
			unsigned char c = input[i];
			size_t seq_len = 0;

			if (c <= 0x7F) seq_len = 1;           // ASCII
			else if ((c & 0xE0) == 0xC0) seq_len = 2; // 2 байта
			else if ((c & 0xF0) == 0xE0) seq_len = 3; // 3 байта
			else if ((c & 0xF8) == 0xF0) seq_len = 4; // 4 байта
			else { i++; continue; } // некорректный байт

			if (i + seq_len > input.size()) break; // обрезанный символ
			bool valid = true;
			for (size_t j = 1; j < seq_len; ++j) {
				if ((input[i + j] & 0xC0) != 0x80) { valid = false; break; }
			}
			if (valid) {
				out.append(input, i, seq_len);
			}
			i += seq_len;
		}
		return out;
	}

	// Обновлённая версия GetElementXPath
	std::string GetElementXPath(const std::string& seleniumUrl,
		const std::string& sessionId,
		const std::string& elementId) {
		std::string script = R"(
					const elt = arguments[0];
					if (!elt) return '';
					const tag = elt.tagName.toLowerCase();

					const title = elt.getAttribute('title');
					if (title) return `//${tag}[@title='${title}']`;

					const id = elt.getAttribute('id');
					if (id) return `//${tag}[@id='${id}']`;

					const cls = elt.getAttribute('class');
					if (cls) return `//${tag}[@class='${cls}']`;

					let path = '';
					for (let e = elt; e && e.nodeType === 1; e = e.parentNode) {
						let idx = 1;
						let sib = e.previousSibling;
						while (sib) {
							if (sib.nodeType === 1 && sib.nodeName === e.nodeName) idx++;
							sib = sib.previousSibling;
						}
						path = '/' + e.nodeName.toLowerCase() + `[${idx}]` + path;
					}
					return path;    )";

		auto rawResp = PostRaw(seleniumUrl + "/session/" + sessionId + "/execute/sync",
			{ {"script", script}, {"args", json::array({{{"element-6066-11e4-a52e-4f735466cecf", elementId}}})} });

		// Конвертируем сырые байты в строку
		std::string jsonStr(rawResp.begin(), rawResp.end());

		// Чистим битый UTF-8 перед разбором JSON
		jsonStr = CleanUTF8(jsonStr);

		json resp = json::parse(jsonStr);

		if (resp.contains("value"))
			return CleanUTF8(resp["value"].get<std::string>());

		return "";
	}
	// Основной обходчик: ищем интерактивные элементы и пишем лог
	void LogInteractiveElements(const std::string& seleniumUrl, const std::string& sessionId) {

		// список тегов для поиска
		std::vector<std::string> tags = { "input", "button", "select", "textarea", "a" };

		for (const auto& tag : tags) {
			json resp = PostJson(seleniumUrl + "/session/" + sessionId + "/elements",
				{ {"using", "css selector"}, {"value", tag} });

			if (!resp.contains("value")) continue;

			for (auto& el : resp["value"]) {
				std::string elementId;
				if (el.contains("element-6066-11e4-a52e-4f735466cecf"))
					elementId = el["element-6066-11e4-a52e-4f735466cecf"];
				else continue;

				// Получаем текст/значение
				std::string text;
				try {
					auto valResp = GetJson(seleniumUrl + "/session/" + sessionId + "/element/" + elementId + "/property/value");
					if (valResp.contains("value") && !valResp["value"].is_null())
						text = valResp["value"].get<std::string>();
				}
				catch (...) {}

				if (text.empty()) {
					try {
						auto txtResp = GetJson(seleniumUrl + "/session/" + sessionId + "/element/" + elementId + "/text");
						if (txtResp.contains("value") && !txtResp["value"].is_null())
							text = txtResp["value"].get<std::string>();
					}
					catch (...) {}
				}

				// Получаем id
				std::string idAttr;
				try {
					auto idResp = GetJson(seleniumUrl + "/session/" + sessionId + "/element/" + elementId + "/attribute/id");
					if (idResp.contains("value") && !idResp["value"].is_null())
						idAttr = idResp["value"].get<std::string>();
				}
				catch (...) {}

				//idAttr = CleanText(idAttr);
				//text = CleanText(text);
				// Если ни id, ни текста — пропускаем
				if (idAttr.empty() && text.empty()) continue;

				// Получаем XPath
				std::string xpath = GetElementXPath(seleniumUrl, sessionId, elementId);

				// Формируем имя переменной
				std::string varName = !idAttr.empty() ? idAttr : text;
				if (varName.size() > 30) varName = varName.substr(0, 100);

				// Пишем в лог
				std::string logLine = "save|@@" + varName + "|" + xpath;
				Logger::Log(Logger::Level::Info, logLine);
			}
		}

		Logger::Log(Logger::Level::Info, "Сканирование завершено");
	}

} // namespace Scan
