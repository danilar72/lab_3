#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <windows.h>

using boost::asio::ip::tcp;
using namespace boost::asio;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)), timer_(socket_.get_executor()) {}

    void start() {
        do_read();
    }

private:
    void do_read() {
        auto self = shared_from_this();
        async_read_until(
            socket_,
            buffer_,
            '\n',
            [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    std::istream stream(&buffer_);
                    std::string message;
                    std::getline(stream, message);
                    process_request(message);
                }
            });
    }

    void process_request(const std::string& message) {
        if (message.find("timer ") == 0) {
            // Обработка команды таймера
            int delay = std::stoi(message.substr(6));
            timer_.expires_after(boost::asio::chrono::seconds(delay));
            
            auto self = shared_from_this();
            timer_.async_wait([this, self, delay](const boost::system::error_code& ec) {
                if (!ec) {
                    std::string response = "Timer " + std::to_string(delay) + " sec expired!\n";
                    do_write(response);
                }
            });
        } else {
            // Ответ
            std::string response = "Message received: " + message + "\n";
            do_write(response);
        }
    }

    void do_write(const std::string& response) {
        auto self = shared_from_this();
        async_write(
            socket_,
            buffer(response),
            [this, self](boost::system::error_code ec, size_t) {
                if (!ec) do_read();
            });
    }

    tcp::socket socket_;
    streambuf buffer_;
    steady_timer timer_;
};

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "Timer server started\n";

        std::vector<std::thread> threads;
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&io] { io.run(); });
        }

        // Асинхронный прием 
        std::function<void()> accept_handler;
        accept_handler = [&] {
            auto socket = std::make_shared<tcp::socket>(io);
            acceptor.async_accept(*socket, [&, socket](boost::system::error_code ec) {
                if (!ec) std::make_shared<Session>(std::move(*socket))->start();
                accept_handler();
            });
        };
        accept_handler();

        for (auto& t : threads) t.join();

    } catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
    return 0;
}