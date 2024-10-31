#include "web_socket_client.h"
#include <websocketpp/common/thread.hpp>
#include <iostream>

WebSocketClient::WebSocketClient()
{
    // Initialize ASIO
    m_client.init_asio();

    // Set event handlers
    m_client.set_open_handler(std::bind(&WebSocketClient::on_open, this, std::placeholders::_1));
    m_client.set_close_handler(std::bind(&WebSocketClient::on_close, this, std::placeholders::_1));
    m_client.set_message_handler(std::bind(&WebSocketClient::on_message, this, std::placeholders::_1, std::placeholders::_2));
}

void WebSocketClient::connect(const std::string &uri)
{
    websocketpp::lib::error_code ec;
    client::connection_ptr con = m_client.get_connection(uri, ec);

    if (ec)
    {
        std::cerr << "Could not create connection: " << ec.message() << std::endl;
        return;
    }

    m_hdl = con->get_handle();
    m_client.connect(con);
    m_client.run();
}

void WebSocketClient::disconnect()
{
    websocketpp::lib::error_code ec;
    m_client.close(m_hdl, websocketpp::close::status::normal, "Client disconnecting", ec);

    if (ec)
    {
        std::cerr << "Disconnect error: " << ec.message() << std::endl;
    }
}

void WebSocketClient::send_message(const std::string &message)
{
    websocketpp::lib::error_code ec;
    
    // Send the message using the client instance
    m_client.send(m_hdl, message, websocketpp::frame::opcode::text, ec);

    if (ec)
    {
        std::cerr << "Send message error: " << ec.message() << std::endl;
    }
}

void WebSocketClient::on_connect(const std::function<void()> &callback) {
    connect_callback = callback;
}

void WebSocketClient::on_disconnect(const std::function<void()> &callback) {
    disconnect_callback = callback;
}

void WebSocketClient::on_message_received(const std::function<void(const std::string &)> &callback)
{
    message_callback = callback;
}

void WebSocketClient::on_open(websocketpp::connection_hdl hdl)
{
    std::cout << "Connection opened" << std::endl;
    if (connect_callback)
    {
        connect_callback();
    }
}

void WebSocketClient::on_close(websocketpp::connection_hdl hdl)
{
    std::cout << "Connection closed" << std::endl;
    if (disconnect_callback)
    {
        disconnect_callback();
    }
}

void WebSocketClient::on_message(websocketpp::connection_hdl hdl, client::message_ptr msg)
{
    if (message_callback)
    {
        message_callback(msg->get_payload());
    }
}