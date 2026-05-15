/**
 * This is a simple TCP client that connects to the main program and continue sending text.
 * The text length is randomly within a range, sent with a fixed interval.
 * It's for debugging text receiving (via TCP socket) and subtitle rendering function of the main program.
 */
#include <sockpp/tcp_connector.h>
#include <string>
#include <random>
#include <thread>

#define PORT 18781
#define SEND_INTERVAL std::chrono::seconds(1)
#define MIN_LEN_PER_SEND 5
#define MAX_LEN_PER_SEND 15

// ref: https://www.lipsum.com/
static const std::string SAMPLE = "On the other hand, we denounce with righteous indignation and dislike men who are so beguiled and "
                                  "demoralized by the charms of pleasure of the moment,\n"
                                  "so blinded by desire, that they cannot foresee the pain and trouble that are bound to ensue;\n"
                                  "and equal blame belongs to those who fail in their duty through weakness of will, which is the same as "
                                  "saying through shrinking from toil and pain.\n"
                                  "These cases are perfectly simple and easy to distinguish.\n"
                                  "In a free hour, when our power of choice is untrammelled and when nothing prevents our being able to do "
                                  "what we like best, every pleasure is to be welcomed and every pain avoided.\n"
                                  "But in certain circumstances and owing to the claims of duty or the obligations of business it will "
                                  "frequently occur that pleasures have to be repudiated and annoyances accepted.\n"
                                  "The wise man therefore always holds in these matters to this principle of selection: he rejects "
                                  "pleasures to secure other greater pleasures, or else he endures pains to avoid worse pains.\n";
static const unsigned long long SAMPLE_LEN = SAMPLE.length();

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution len_dist(MIN_LEN_PER_SEND, MAX_LEN_PER_SEND);

static std::string random_text()
{
    static int pos = 0;
    int const len = len_dist(gen);
    std::string ret;
    if (const int remain_len = SAMPLE_LEN - pos; len > remain_len)
    {
        const std::string tail = SAMPLE.substr(pos, remain_len);
        pos = 0;
        ret = tail + SAMPLE.substr(pos, len - remain_len);
    }
    else
    {
        ret = SAMPLE.substr(pos, len);
    }
    pos += len;
    return ret;
}

/**
 * ref: https://github.com/fpagliughi/sockpp/blob/master/examples/tcp/tcpecho.cpp
 */
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    sockpp::initialize();

    constexpr std::chrono::seconds timeout(1);

    sockpp::tcp_connector conn;
    if (const auto res = conn.connect("localhost", PORT, timeout); !res)
    {
        std::cerr << "Error connecting to [localhost:" << PORT << "]: " << res.error_message() << '\n';
        return 1;
    }
    std::cout << "Created a connection to [" << conn.address() << "]" << '\n';

    while (true)
    {
        std::this_thread::sleep_for(SEND_INTERVAL);
        auto s = random_text();
        std::cout << "Send [" << s << "]" << '\n';

        if (auto res = conn.write(s); res != s.length())
        {
            std::cerr << "Error writing to the TCP stream: " << res.error_message() << '\n';
            conn.close();
            return 1;
        }
    }
}
