#include <thread>

#include "args_tokenizer.h"
#include "conn_producer.h"
#include "conn_consumer.h"

using std::thread;

ConnProducer *producer = nullptr;

void sigintHandler(int signal) {
    if (producer != nullptr)
        producer->Drop();
    cout << endl;
    cout << "SIGINT!\nlisten port fd and epoll instance have been destroyed" << endl;
}

int main(int argc, char *argv[]) {

    // chdir("/tmp/tmp.LHC2rqj3pK/");
    // parse arguments to get port and site base directory
    ArgsTokenizer tokenizer;
    tokenizer.Parse(argc, argv);
    string argValue;
    if (tokenizer.FindArg("-h", &argValue) || tokenizer.FindArg("--help", &argValue)) {
        cout << "Usage " << argv[0] << " --port [PORT] --dir [BASE_DIR]" << endl;
        exit(EXIT_SUCCESS);
    }
    if (!tokenizer.FindArg("-p", &argValue) && !tokenizer.FindArg("--port", &argValue)) {
        cout << "Usage " << argv[0] << " --port [PORT] --dir [BASE_DIR]" << endl;
        exit(EXIT_SUCCESS);
    }
    string port = argValue;
    if (!tokenizer.FindArg("--dir", &argValue)) {
        cout << "Usage " << argv[0] << " --port [PORT] --dir [BASE_DIR]" << endl;
        exit(EXIT_SUCCESS);
    }
    string baseDir = argValue;

    int listenFD;

    if ((listenFD = OpenListenFD(port.c_str())) < 0)
        app_error("Open Listen File Descriptor Failed");
    Signal(SIGINT, sigintHandler);

    ConcurrentQueue queue;
    vector<thread> consumer_pool(MAX_CONSUMER_NUM);
    for (int i = 0; i < MAX_CONSUMER_NUM; i++)
        consumer_pool.push_back(thread(&ConnConsumer::Execute, ConnConsumer(&queue, baseDir)));
    producer = new ConnProducer(listenFD, &queue, baseDir);
    producer->Execute();

    return 0;
}