#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <windows.h>

using boost::asio::ip::tcp;

uint64_t calculate_factorial(int n) {
    return (n <= 1) ? 1 : n * calculate_factorial(n - 1);
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "Factorial server started at 127.0.0.1:12345\n";

        tcp::socket socket(io_context);
        acceptor.accept(socket);
        std::cout << "Client connected\n";

        auto buffer = std::make_shared<boost::asio::streambuf>();

        boost::asio::async_read_until(socket, *buffer, '\n',
            [buffer, &socket](const boost::system::error_code& error, std::size_t) {
                if (!error) {
                    std::istream stream(buffer.get());
                    std::string input;
                    std::getline(stream, input);

                    try {
                        int number = std::stoi(input);
                        uint64_t result = calculate_factorial(number);
                        std::string response = std::to_string(result) + "\n";

                        boost::asio::async_write(socket, boost::asio::buffer(response),
                            [](const boost::system::error_code&, std::size_t) {
                                std::cout << "Response sent\n";
                            });
                    } catch (...) {
                        std::string error_msg = "Error: invalid input\n";
                        boost::asio::write(socket, boost::asio::buffer(error_msg));
                    }
                } else {
                    std::cerr << "Read error: " << error.message() << "\n";
                }
            });

        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }

    return 0;
}