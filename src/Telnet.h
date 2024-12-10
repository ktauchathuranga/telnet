#ifndef TELNET_H
#define TELNET_H

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <vector>
#include <map>
#include <functional>

#define MAX_CLIENTS 5
#define DEFAULT_TIMEOUT 300000 // 5 minutes

class Telnet {
public:
    Telnet();

    // Mode setup
    void beginAP(const char *ssid = "ESP8266_Telnet", const char *password = "12345678");
    void beginClient(const char *ssid, const char *password);

    // Server management
    void startServer(uint16_t port = 23);
    void stopServer();
    void handleClient();

    // Command customization
    void addCommand(const char *cmd, std::function<void(WiFiClient &client, const String &args)> handler, const char *help = nullptr);
    void setAlias(const char *alias, const char *cmd);

    // Banner and Prompt
    void setPrompt(const char *username, const char *deviceName);

    // Timeout management
    void setTimeout(uint32_t timeoutMillis);
    void prioritizeClient(uint8_t index);

    // Help
    void showHelp(WiFiClient &client, const String &args);


private:
    struct Command {
        String cmd;
        std::function<void(WiFiClient &client, const String &args)> handler;
        String help;
    };

    WiFiServer *telnetServer;
    WiFiClient clients[MAX_CLIENTS];
    uint32_t lastActivity[MAX_CLIENTS];
    std::vector<Command> commands;
    std::map<String, String> aliases;

    const String defaultBanner =
"  ______     ______     ______  \r\n"
" /\\  ___\\   /\\  ___\\   /\\  == \\ \r\n"
" \\ \\  __\\   \\ \\___  \\  \\ \\  _-/ \r\n"
"  \\ \\_____\\  \\/\\_____\\  \\ \\_\\   \r\n"
"   \\/_____/   \\/_____/   \\/_/   \r\n"
"                                \r\n"
"Welcome to the ESP8266 Telnet Server!\r\n"
"Type 'help' to see the list of commands.\r\n";

    String banner;
    String prompt;
    uint32_t timeoutMillis;

    void showBanner(WiFiClient &client);
    void showPrompt(WiFiClient &client);
    void handleCommand(WiFiClient &client, const String &input);
    void removeInactiveClients();
    void disconnectClient(WiFiClient &client, const String &args);
    void flushClient(WiFiClient &client);
};

#endif
