#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <windows.h>

using namespace boost::asio;
using namespace boost::asio::ip;

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (argc != 3) {
        std::cerr << "Usage: client <server_ip> <port>\n";
        return 1;
    }

    try {
        io_context io;
        tcp::socket socket(io);
        tcp::resolver resolver(io);

        boost::system::error_code ec;
        connect(socket, resolver.resolve(argv[1], argv[2]), ec);
        if (ec) {
            std::cerr << "Connect error: " << ec.message() << "\n";
            return 1;
        }

        std::cout << "Connected to " << argv[1] << ":" << argv[2] << "\n";
        std::cout << "Enter numbers (type 'exit' to quit):\n";

        while (true) {
            std::string input;
            std::cout << "> ";
            std::getline(std::cin, input);
            if (input == "exit") break;

            input += "\n";
            write(socket, buffer(input), ec);
            if (ec) {
                std::cerr << "Write error: " << ec.message() << "\n";
                break;
            }

            streambuf response_buffer;
            size_t bytes = read_until(socket, response_buffer, '\n', ec);
            if (ec) {
                if (ec == boost::asio::error::eof) {
                    std::cerr << "Server closed the connection.\n";
                } else {
                    std::cerr << "Read error: " << ec.message() << "\n";
                }
                break;
            }

            std::istream stream(&response_buffer);
            std::string response;
            std::getline(stream, response);
            std::cout << "Result: " << response << "\n";

            response_buffer.consume(bytes); // Очистка буфера
        }

        socket.shutdown(tcp::socket::shutdown_both, ec);
        socket.close(ec);

    } catch (std::exception& e) {
        std::cerr << "Client error: " << e.what() << "\n";
    }
    return 0;
}