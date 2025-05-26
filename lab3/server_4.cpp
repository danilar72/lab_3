#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <sstream>
#include <windows.h>

using namespace boost::asio;
using namespace boost::asio::ip;

class Session : public std::enable_shared_from_this<Session> {
public:
    static std::shared_ptr<Session> create(tcp::socket socket,
                                          strand<io_context::executor_type> strand,
                                          std::shared_ptr<std::vector<std::string>> log) {
        return std::shared_ptr<Session>(new Session(std::move(socket), strand, log));
    }

    void start() {
        post(strand_, [self = shared_from_this()] {
            self->do_read();
        });
    }

    ~Session() {  
        boost::system::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        socket_.close(ec);
        std::cout << "Session destroyed\n";
    }

private:
    Session(tcp::socket socket,
           strand<io_context::executor_type> strand,
           std::shared_ptr<std::vector<std::string>> log)
        : socket_(std::move(socket)),
          strand_(strand),
          log_(log) {
        std::cout << "Session created\n";
    }

    void do_read() {
        auto self = shared_from_this();
        async_read_until(
            socket_,
            buffer_,
            '\n',
            bind_executor(
                strand_,
                [this, self](boost::system::error_code ec, size_t length) {
                    if (ec) {
                        if (ec != boost::asio::error::operation_aborted) {
                            std::cerr << "Read error: " << ec.message() << "\n";
                        }
                        return;
                    }

                    std::istream stream(&buffer_);
                    std::string message;
                    std::getline(stream, message);
                    buffer_.consume(length);

                    process_request(message);
                }
            ));
    }

    void process_request(const std::string& message) {
        auto self = shared_from_this();
        post(strand_, [this, self, message] {
            log_->push_back("Request: " + message);
        });

        post(socket_.get_executor(), [this, self, message] {
            try {
                int number = std::stoi(message);
                uint64_t result = 1;
                for (int i = 2; i <= number; ++i) result *= i;
                
                std::string response = std::to_string(result) + "\n";
                do_write(response);

                post(strand_, [this, self, response] {
                    log_->push_back("Response: " + response);
                });
            } catch (...) {
                do_write("Error: invalid input\n");
            }
        });
    }

    void do_write(const std::string& response) {
        auto self = shared_from_this();
        async_write(
            socket_,
            buffer(response),
            bind_executor(
                strand_,
                [this, self](boost::system::error_code ec, size_t) {
                    if (ec) {
                        std::cerr << "Write error: " << ec.message() << "\n";
                        return;
                    }
                    do_read();
                }
            ));
    }

    tcp::socket socket_;
    streambuf buffer_;
    strand<io_context::executor_type> strand_;
    std::shared_ptr<std::vector<std::string>> log_;
};

class Server {
public:
    Server(io_context& io, short port, int thread_pool_size)
        : acceptor_(io, tcp::endpoint(tcp::v4(), port)),
          strand_(make_strand(io)),
          log_(std::make_shared<std::vector<std::string>>()) {
        do_accept();
        
        for (int i = 0; i < thread_pool_size; ++i) {
            threads_.emplace_back([&io] { io.run(); });
        }
    }

    ~Server() {
        acceptor_.close();
        for (auto& t : threads_) if (t.joinable()) t.join();
    }

private:
    void do_accept() {
        auto socket = std::make_shared<tcp::socket>(acceptor_.get_executor());
        acceptor_.async_accept(
            *socket,
            bind_executor(
                strand_,
                [this, socket](boost::system::error_code ec) {
                    if (!ec) {
                        auto session = Session::create(std::move(*socket), strand_, log_);
                        session->start();
                    }
                    do_accept();
                }
            ));
    }

    tcp::acceptor acceptor_;
    strand<io_context::executor_type> strand_;
    std::shared_ptr<std::vector<std::string>> log_;
    std::vector<std::thread> threads_;
};

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        if (argc != 3) {
            std::cerr << "Usage: server <port> <threads>\n";
            return 1;
        }

        io_context io;
        Server server(io, std::stoi(argv[1]), std::stoi(argv[2]));
        std::cout << "Server started. Port: " << argv[1] 
                  << ", Threads: " << argv[2] << "\n";
        io.run();

    } catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }
    return 0;
}
