#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include <iostream>
#include <string>
#include <set>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <windows.h>

using json = nlohmann::json;


class agntzabbix {

public:
	void   salvar_config(std::string ip, int porta, int level, std::string user, std::string pass, std::string token);
	//size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
//	json   sendZabbixRequest(const std::string& url, const json& payload, const std::string& token = "");
	void   showWindowsNotification(int warn, int avg, int high, int disaster);

private:


};