# ⚓ AnchorZBX: Zabbix Push Notifier

O **AnchorZBX** é uma ferramenta de monitoramento projetada para atuar em conjunto com o Zabbix, auxiliando profissionais de infraestrutura e NOC. Ele funciona como um "X9" silencioso: sempre que o Zabbix gerar um novo alerta, o agente dispara uma notificação *push* nativa diretamente no seu Windows.

Com isso, você pode focar em outras tarefas críticas sem a necessidade de manter o dashboard do Zabbix aberto o tempo todo, garantindo que nenhum incidente passe despercebido.

### Observação: é necessário incluir as seguintes bibliotecas manualmente:

CLI11   = ela é responsável por gerenciar as passagens de parâmetro via cmd para o executável da ferramenta.
libcurl = Para consumir api do zabbix.
json    = para tratamento dos dados.

## 🚀 Funcionalidades e Arquitetura

O projeto foi construído com foco em performance e segurança:

* **C++ Moderno e POO:** Totalmente desenvolvido em C++ utilizando princípios de Orientação a Objetos para isolamento de responsabilidades.
* **Consumo de API:** Comunicação com a API JSON-RPC do Zabbix utilizando a biblioteca Libcurl para ser possível consumir api do zabbix.
* **Parsing Seguro:** Utilização da biblioteca **nlohmann/json** para montagem e tratamento eficiente das respostas do servidor.
* **Background:** Utiliza a API do Win32 (`FreeConsole()`) para rodar silenciosamente em segundo plano, sem prender o terminal do usuário e também para gerar as notificações do windows.
* **Segurança de Memória:** Gerenciamento rigoroso de memória e callbacks protegidos para evitar *buffer overflows*.

## 🛠️ Como Usar

A ferramenta é inicializada via linha de comando (CMD ou PowerShell). Ao ser executada com os parâmetros corretos, ela salva a configuração e entra em background automaticamente.

### Exemplo de Inicialização com Usuário/Senha:
Abra o cmd e vá até a pasta do executável anchorZBX
anchorZBX.exe -a 192.168.55.203 -p 80 -u user_01 -w Senha@10

### Exemplo de Inicialização com Token (Recomendado):
anchorZBX.exe -a 192.168.55.203 -p 80 -t "SEU_TOKEN_DE_API_AQUI" -l 1

### Parâmetros Disponíveis:

-a, --address = Endereço IP ou Host do servidor Zabbix.
-p, --port	  = Porta de comunicação do Zabbix (padrão: 80).
-u, --user	  = Usuário do Zabbix (Ignorado se usar Token).
-w, --pass	  = Senha do Zabbix (Ignorado se usar Token).
-t, --token	  = Autenticação via Token de API do Zabbix.
-l, --level	  = Define o nível de severidade dos alertas.

### Níveis de Severidade (-l):
Level 1: Apenas alertas Disaster e High. Ideal para gestores que só precisam ver incidentes críticos.
Level 2: Adiciona alertas Average e Warning. Ideal para a equipe de infraestrutura atuar na prevenção.
Level 3: Todos os alertas. Ideal para telas de NOC. (Padrão caso a flag não seja informada).

### Subcomandos Úteis:
Uma vez que o agente esteja rodando de forma invisível em segundo plano, você pode utilizar o próprio executável para gerenciá-lo.

anchorZBX.exe show  = Exibe a configuração atual que o agente está utilizando na memória (mascarando a senha/token por segurança).
anchorZBX.exe clear = Apaga as configurações locais e encerra imediatamente o processo do agente em background

Imagem ilustrando a ferramenta em operação:
<img width="1655" height="329" alt="notf2" src="https://github.com/user-attachments/assets/a33da184-b0bc-4953-8b98-3aca49babfee" />











