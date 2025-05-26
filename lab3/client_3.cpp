#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <windows.h>

using boost::asio::ip::tcp;

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        boost::asio::io_context io;
        tcp::socket socket(io);
        tcp::endpoint server(boost::asio::ip::make_address("127.0.0.1"), 12345);
        socket.connect(server);

        std::cout << "Enter command (timer N or message): ";
        std::string input;
        std::getline(std::cin, input);
        input += "\n";

        boost::asio::write(socket, boost::asio::buffer(input));

        boost::asio::streambuf buffer;
        boost::asio::read_until(socket, buffer, '\n');

        std::istream stream(&buffer);
        std::string response;
        std::getline(stream, response);
        std::cout << "Response: " << response << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
    return 0;
}