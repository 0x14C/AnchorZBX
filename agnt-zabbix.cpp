#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include <iostream>
#include <string>
#include <set>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <windows.h>
#include "../x64/Release/CLI11/include/CLI/CLI.hpp"
//#include "agnt-zabbix.hpp"

using json = nlohmann::json;

using json = nlohmann::json;

// =====================================================================
// CLASSE 1: GERENCIADOR DE CONFIGURAÇÕES
// =====================================================================
class ConfigManager {
public:
    struct ConfigData {
        std::string ip;
        int port;
        int level;
        std::string user;
        std::string pass;
        std::string token;
        bool is_valid = false;
    };

    static void salvar(const ConfigData& data) {
        json config;
        config["ip"] = data.ip;
        config["port"] = data.port;
        config["level"] = data.level;
        config["user"] = data.user;
        config["pass"] = data.pass;
        config["token"] = data.token;
        std::ofstream file("config.json");
        file << config.dump(4);
    }

    static ConfigData carregar() {
        ConfigData data;
        std::ifstream file("config.json");
        if (file.is_open()) {
            json config;
            file >> config;
            data.ip = config.value("ip", "");
            data.port = config.value("port", 80);
            data.level = config.value("level", 3);
            data.user = config.value("user", "");
            data.pass = config.value("pass", "");
            data.token = config.value("token", "");
            data.is_valid = true;
        }
        return data;
    }

    static void limpar() {
        remove("config.json");
    }
};

// =====================================================================
// CLASSE 2: NOTIFICADOR DO WINDOWS
// =====================================================================
class WindowsNotifier {
public:
    static void mostrarAlerta(int w, int a, int h, int d) {
        HWND dummy_hwnd = CreateWindowA("STATIC", "ZabbixDummy", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

        NOTIFYICONDATAA nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATAA);
        nid.hWnd = dummy_hwnd;
        nid.uID = 1001;
        nid.uFlags = NIF_ICON | NIF_INFO;
        nid.hIcon = LoadIcon(NULL, IDI_WARNING);
        nid.dwInfoFlags = NIIF_WARNING;

        std::string title = "Zabbix: Alertas Ativos";
        std::string msg = "Desastre: " + std::to_string(d) + " | High: " + std::to_string(h) +
            "\nAverage: " + std::to_string(a) + " | Warning: " + std::to_string(w);

        strncpy_s(nid.szInfoTitle, sizeof(nid.szInfoTitle), title.c_str(), _TRUNCATE);
        strncpy_s(nid.szInfo, sizeof(nid.szInfo), msg.c_str(), _TRUNCATE);

        Shell_NotifyIconA(NIM_ADD, &nid);
        Sleep(8000); // Tempo para o Windows 11 processar a animação
        Shell_NotifyIconA(NIM_DELETE, &nid);
        DestroyWindow(dummy_hwnd);
    }
};

// =====================================================================
// CLASSE 3: CLIENTE ZABBIX (Rede e API)
// =====================================================================
class ZabbixClient {
private:
    std::string base_url;
    std::string session_token;

    // Em POO, callbacks de bibliotecas em C puro precisam ser estáticos (static)
    // para não se confundirem com as instâncias do objeto na memória.
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    json enviarRequisicao(const json& payload) {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();
        if (curl) {
            std::string payloadStr = payload.dump();
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json-rpc");

            if (!session_token.empty()) {
                std::string auth_header = "Authorization: Bearer " + session_token;
                headers = curl_slist_append(headers, auth_header.c_str());
            }

            curl_easy_setopt(curl, CURLOPT_URL, base_url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (res == CURLE_OK) {
                try { return json::parse(readBuffer); }
                catch (...) { return json({}); }
            }
        }
        return json({});
    }

public:
    ZabbixClient(const std::string& ip, int port) {
        base_url = "http://" + ip + ":" + std::to_string(port) + "/zabbix/api_jsonrpc.php";
    }

    bool autenticar(const std::string& user, const std::string& pass, const std::string& token) {
        if (!token.empty()) {
            session_token = token;
            return true;
        }

        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "user.login"},
            {"params", {{"username", user}, {"password", pass}}},
            {"id", 1}
        };

        json response = enviarRequisicao(payload);

        if (response.contains("result")) {
            session_token = response["result"];
            return true;
        }

        std::string erro_msg = "Falha ao conectar no Zabbix!\nURL: " + base_url;
        if (response.contains("error")) erro_msg += "\nMotivo: " + response["error"]["data"].get<std::string>();
        MessageBoxA(NULL, erro_msg.c_str(), "Erro Fatal", MB_ICONERROR | MB_OK);
        return false;
    }

    json buscarTriggers(const std::vector<int>& severities) {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "trigger.get"},
            {"params", {
                {"output", {"triggerid", "priority"}},
                {"filter", {{"value", "1"}}},
                {"severities", severities},
                {"active", true},
                {"only_true", true}
            }},
            {"id", 2}
        };
        return enviarRequisicao(payload);
    }
};

