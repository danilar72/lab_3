#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <windows.h>

using boost::asio::ip::tcp;

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "Server started at 127.0.0.1:12345\nWaiting for connections...\n";

        for (;;) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::cout << "Client connected: " << socket.remote_endpoint() << std::endl;

            // Read message
            boost::asio::streambuf buffer;
            boost::asio::read_until(socket, buffer, '\n');
            
            std::istream stream(&buffer);
            std::string message;
            std::getline(stream, message);
            std::cout << "Received: " << message << std::endl;

            // Ответ
            std::string response = "Message received: " + message + "\n";
            boost::asio::write(socket, boost::asio::buffer(response));
            std::cout << "Response sent: " << response;
        }
    } catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
    return 0;
}