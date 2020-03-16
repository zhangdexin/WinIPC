# WinIPC
## 简介

仅适用于windows的进程间通信，利用完成端口（IOCP）形式建立tcp的服务器，客户端利用阻塞式的模型。测试可以作为进程间通信，应该也可以作为服务器开发的原型。

## 示例

### server:

```C++
// init server
IocpServer server(IocpServer::ListenType_IPC, "12345");

// run, recv, send and stop
server.Run([&server](const LSocket& socket, const char* data, DWORD size) {
    std::string dataStr = std::string(data, size);
    std::cout << "server recv:" << dataStr << std::endl;

    server.SendMsg(socket, dataStr);

    static int t = 0;
    t++;
    if (t == 10) {
        server.Stop();
    }
});
```

### client:

```C++
try {
    // init client
    Client client(Client::ConnectType_IPC, "127.0.0.1", 12345);

    // send msg
    std::thread thd([&]() {
        while (1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            Client::Status s = client.GetStatus();
            if (s == Client::Status_Running) {
                std::string str = "123456";
                bool rc = client.SendSync(str.c_str(), str.size());
                if (!rc) {
                    std::cout << "send msg failed" << std::endl;
                    return;
                }
            }
            else if (s == Client::Ststus_Stopped) {
                return;
            }
        }
    });

    // run and recv msg
    client.Run([](const char* buffer, int size) {
        std::cout << "client recv:" << std::string(buffer, size) << std::endl;
    });

    thd.join();
}
catch (std::exception& e) {
    std::cout << e.what() << std::endl;
}
```

