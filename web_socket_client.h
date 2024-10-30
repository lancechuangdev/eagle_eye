#ifndef EAGLE_EYE_WEBSOCKET_CLIENT_H
#define EAGLE_EYE_WEBSOCKET_CLIENT_H

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <string>
#include <functional>

class WebSocketClient {
public:
    WebSocketClient();
    void connect(const std::string &uri);
    void disconnect();
    
    // Register callback for messages
    void on_connect(const std::function<void()> &callback);
    void on_disconnect(const std::function<void()> &callback);
    void on_message_received(const std::function<void(const std::string &)> &callback);

private:
    // Alias for websocket client type
    using client = websocketpp::client<websocketpp::config::asio_client>;

    client m_client;
    websocketpp::connection_hdl m_hdl;

    // Callback functions
    std::function<void()> connect_callback;
    std::function<void()> disconnect_callback;
    std::function<void(const std::string &)> message_callback;

    // Event Handlers
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg);
};

#endif // EAGLE_EYE_WEBSOCKET_CLIENT_H