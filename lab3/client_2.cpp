#include <boost/asio.hpp>
#include <iostream>
#include <windows.h>

using boost::asio::ip::tcp;

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);
        tcp::endpoint server(boost::asio::ip::make_address("127.0.0.1"), 12345);
        socket.connect(server);

        std::cout << "Enter a number: ";
        std::string number;
        std::getline(std::cin, number);
        number += "\n";

        boost::asio::write(socket, boost::asio::buffer(number));

        boost::asio::streambuf response_buffer;
        boost::asio::read_until(socket, response_buffer, '\n');

        std::istream stream(&response_buffer);
        std::string response;
        std::getline(stream, response);
        std::cout << "Factorial: " << response << "\n";

    } catch (std::exception& e) {
        std::cerr << "Client error: " << e.what() << "\n";
    }

    return 0;
}