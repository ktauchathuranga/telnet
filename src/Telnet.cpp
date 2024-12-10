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
                flushClient(clients[i]); // Clear buffer after disconnection
            }
        }

        if (telnetServer->hasClient()) {
            for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i] || !clients[i].connected()) {
                    clients[i] = telnetServer->available();
                    clients[i].setTimeout(timeoutMillis);
                    flushClient(clients[i]); // Clear buffer after new client connection
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
                flushClient(clients[i]); // Flush buffers after processing command
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
    client.flush(); // Ensure the banner is sent immediately
}

void Telnet::showPrompt(WiFiClient &client) {
    client.print(prompt);
    client.print(" ");
    client.flush(); // Ensure the prompt is sent immediately
}

void Telnet::handleCommand(WiFiClient &client, const String &inputRaw) {
    String input = inputRaw;
    input.trim();
    input.replace("\r", "");

    Serial.println("Received command: [" + input + "]");

    for (const auto &cmd : commands) {
        if (input == cmd.cmd || aliases[input] == cmd.cmd) {
            cmd.handler(client, "");
            return;
        }
    }

    client.println("Unknown command: " + input);
    client.flush(); // Ensure the error message is sent immediately
}

void Telnet::showHelp(WiFiClient &client, const String &args) {
    client.println("Available commands:");
    for (auto &command : commands) {
        client.println(String(command.cmd) + " - " + String(command.help));
    }
    client.flush(); // Ensure help message is sent immediately
}

void Telnet::disconnectClient(WiFiClient &client, const String &args) {
    client.println("Goodbye!");
    client.flush(); // Ensure goodbye message is sent
    client.stop();
    flushClient(client); // Clear client buffers after disconnection
}

void Telnet::flushClient(WiFiClient &client) {
    while (client.available()) {
        client.read(); // Discard any remaining data in the buffer
    }
    client.flush(); // Ensure all outgoing data is cleared
}
