#include "json.h"
#include <curl/curl.h>

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

// ------------------- Callback libcurl -------------------
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}


// Callback для записи ответа в std::vector<unsigned char>
static size_t WriteCallbackA(void* contents, size_t size, size_t nmemb, void* userp) {
    auto totalSize = size * nmemb;
    auto* buffer = static_cast<std::vector<unsigned char>*>(userp);
    buffer->insert(buffer->end(), (unsigned char*)contents, (unsigned char*)contents + totalSize);
    return totalSize;
}

// Функция для POST запроса, возвращает сырые байты ответа
std::vector<unsigned char> PostRaw(const std::string& url, const nlohmann::json& jsonData) {
    std::vector<unsigned char> response;

    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Curl init failed");

    std::string postData = jsonData.dump();

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postData.size());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackA);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // если https и нет сертификатов
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("Curl perform failed: ") + curl_easy_strerror(res));
    }

    return response;
}
