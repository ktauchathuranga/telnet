#include "Telnet.h"

Telnet::Telnet() : telnetServer(nullptr), timeoutMillis(DEFAULT_TIMEOUT) {
    banner = defaultBanner;
    prompt = "root@esp:~$";
    addCommand("help", std::bind(&Telnet::showHelp, this, std::placeholders::_1, std::placeholders::_2), "Shows a list of available commands.");
    addCommand("exit", std::bind(&Telnet::disconnectClient, this, std::placeholders::_1, std::placeholders::_2), "Exits the Telnet session.");
}

void Telnet::beginAP(const char *ssid, const char *password) {
    WiFi.softAP(ssid, password);
    Serial.print("Access Point started. SSID: ");
    Serial.println(ssid);
}

void Telnet::beginClient(const char *ssid, const char *password) {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi connected.");
}

void Telnet::startServer(uint16_t port) {
    telnetServer = new WiFiServer(port);
    telnetServer->begin();
    Serial.println("Telnet server started.");
}

void Telnet::stopServer() {
    if (telnetServer) {
        telnetServer->stop();
        delete telnetServer;
        telnetServer = nullptr;
        Serial.println("Telnet server stopped.");
    }
}

void Telnet::handleClient() {
    if (telnetServer) {
        for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] && !clients[i].connected()) {
                clients[i].stop();
            }
        }

        if (telnetServer->hasClient()) {
            for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i] || !clients[i].connected()) {
                    clients[i] = telnetServer->available();
                    clients[i].setTimeout(timeoutMillis);
                    showBanner(clients[i]);
                    showPrompt(clients[i]);
                    break;
                }
            }
        }

        for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] && clients[i].connected() && clients[i].available()) {
                String input = clients[i].readStringUntil('\n');
                input.trim();
                handleCommand(clients[i], input);
                showPrompt(clients[i]);
            }
        }
    }
}

void Telnet::addCommand(const char *cmd, std::function<void(WiFiClient &client, const String &args)> handler, const char *help) {
    commands.push_back({cmd, handler, help ? help : ""});
}

void Telnet::setAlias(const char *alias, const char *cmd) {
    aliases[alias] = cmd;
}

void Telnet::setPrompt(const char *username, const char *deviceName) {
    prompt = String(username) + "@" + deviceName + ":~$";
}

void Telnet::setTimeout(uint32_t timeoutMillis) {
    this->timeoutMillis = timeoutMillis;
}

void Telnet::showBanner(WiFiClient &client) {
    client.println(banner);
}

void Telnet::showPrompt(WiFiClient &client) {
    client.print(prompt);
    client.print(" ");
}

void Telnet::handleCommand(WiFiClient &client, const String &input) {
    for (const auto &cmd : commands) {
        if (input == cmd.cmd || aliases[input] == cmd.cmd) {
            cmd.handler(client, "");
            return;
        }
    }
    client.println("Unknown command: " + input);
}

void Telnet::showHelp(WiFiClient &client, const String &args) {
    client.println("Available commands:");
    for (auto &command : commands) {
        client.println(String(command.cmd) + " - " + String(command.help));
    }
}

void Telnet::disconnectClient(WiFiClient &client, const String &args) {
    client.println("Goodbye!");
    client.stop();
}