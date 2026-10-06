#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include <iostream>
#include <string>
#include <set>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <windows.h>
#include "../x64/Release/CLI11/include/CLI/CLI.hpp" //biblioteca para auxiliar na passagem de parametros via cmd.
#include "agnt-zabbix.cpp"

using json = nlohmann::json;

int main(int argc, char** argv) {
    CLI::App app{ "Agente Zabbix Monitor" };

    ConfigManager::ConfigData input_data;

    app.add_option("-a,--address", input_data.ip, "IP do Servidor Zabbix");
    app.add_option("-p,--port", input_data.port, "Porta do Zabbix")->default_val(80);
    app.add_option("-l,--level", input_data.level, "Nivel (1 a 3)")->default_val(3)->check(CLI::Range(1, 3));
    app.add_option("-u,--user", input_data.user, "Usuario do Zabbix");
    app.add_option("-w,--pass", input_data.pass, "Senha do Zabbix");
    app.add_option("-t,--token", input_data.token, "Token de API");

    CLI::App* show_cmd = app.add_subcommand("show", "Exibe a configuracao atual");
    CLI::App* clear_cmd = app.add_subcommand("clear", "Limpa configuracoes");

    CLI11_PARSE(app, argc, argv);

    // --- COMANDOS CLI ---
    if (show_cmd->parsed()) {
        auto config = ConfigManager::carregar();
        if (config.is_valid) {
            std::cout << "[STATUS] IP: " << config.ip << " | Porta: " << config.port << "\n";
            std::cout << "Autenticacao: " << (config.token.empty() ? "Usuario/Senha" : "Token") << "\n";
        }
        else {
            std::cout << "O agente nao esta configurado.\n";
        }
        return 0;
    }

    if (clear_cmd->parsed()) {
        ConfigManager::limpar();
        system("taskkill /F /IM Instalador_AgenteZabbix_v1.exe >nul 2>&1");
        std::cout << "[CLEAR] Agente parado e configuracoes limpas.\n";
        return 0;
    }

    // --- INICIALIZAÇÃO DO AGENTE ---
    ConfigManager::ConfigData config_ativa;

    if (!input_data.ip.empty()) {
        if (input_data.token.empty() && (input_data.user.empty() || input_data.pass.empty())) {
            std::cout << "ERRO: Forneca um Token (-t) OU Usuario (-u) e Senha (-w)!\n";
            return 1;
        }
        ConfigManager::salvar(input_data);
        config_ativa = input_data;
        std::cout << "Configuracao salva! Iniciando...\n";
    }
    else {
        config_ativa = ConfigManager::carregar();
        if (!config_ativa.is_valid) {
            std::cout << "Erro: Nenhuma configuracao encontrada. Use -a <IP>\n";
            return 1;
        }
    }

    // Instancia o cliente da API
    ZabbixClient zabbix(config_ativa.ip, config_ativa.port);

    if (!zabbix.autenticar(config_ativa.user, config_ativa.pass, config_ativa.token)) {
        return 1; // Falhou no login, encerra.
    }

    // Entra em modo invisível
    FreeConsole();

    // Loop Principal
    std::set<std::string> triggers_conhecidos;
    std::vector<int> severities = (config_ativa.level == 1) ? std::vector<int>{4, 5} :
        (config_ativa.level == 2) ? std::vector<int>{2, 3} :
        std::vector<int>{ 0, 1, 2, 3, 4, 5 };

    while (true) {
        json response = zabbix.buscarTriggers(severities);

        if (response.contains("result")) {
            int w = 0, a = 0, h = 0, d = 0;
            std::set<std::string> triggers_atuais;
            bool tem_novo = false;

            for (const auto& trigger : response["result"]) {
                std::string id = trigger["triggerid"];
                int sev = std::stoi(trigger["priority"].get<std::string>());
                triggers_atuais.insert(id);

                if (sev == 2) w++; else if (sev == 3) a++; else if (sev == 4) h++; else if (sev == 5) d++;
                if (triggers_conhecidos.find(id) == triggers_conhecidos.end()) tem_novo = true;
            }

            if (tem_novo || triggers_atuais.size() != triggers_conhecidos.size()) {
                if (w > 0 || a > 0 || h > 0 || d > 0) {
                    WindowsNotifier::mostrarAlerta(w, a, h, d);
                }
                triggers_conhecidos = triggers_atuais;
            }
        }
        Sleep(10000);
    }

    return 0;
}

//====================================
//FUNÇÃO OLD SOMENTE PARA WINDOWS 10.
//====================================
/*
void showWindowsNotification(int warn, int avg, int high, int disaster) {
    NOTIFYICONDATAA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = GetConsoleWindow();
    nid.uID = 1001;
    nid.uFlags = NIF_ICON | NIF_INFO;
    nid.hIcon = LoadIcon(NULL, IDI_WARNING);
    nid.dwInfoFlags = NIIF_WARNING;

    std::string title = "Zabbix: Alertas Ativos";
    std::string msg = "Desastre: " + std::to_string(disaster) +
        " | High: " + std::to_string(high) +
        "\nAverage: " + std::to_string(avg) +
        " | Warning: " + std::to_string(warn);

    strncpy_s(nid.szInfoTitle, sizeof(nid.szInfoTitle), title.c_str(), _TRUNCATE);
    strncpy_s(nid.szInfo, sizeof(nid.szInfo), msg.c_str(), _TRUNCATE);

    Shell_NotifyIconA(NIM_ADD, &nid);
    Sleep(3000);
    Shell_NotifyIconA(NIM_DELETE, &nid);
}
*/


