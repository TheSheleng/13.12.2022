#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>

#include <Windows.h>
#include <exception>
#include <list>
#include <string>
#include <thread>
#include <iostream>

using namespace std;

#define PORT 22000

#define TABLES_COUNT 30

namespace UDP
{
    const unsigned MSG_MAX = 1024;

    struct Msg
    {
        sockaddr_in aFrom;
        int szaFrom = sizeof(aFrom);

        string msg;
    };

    class Server
    {
        SOCKET _socket;
        sockaddr_in addr;

    public:
        Server(string ip, unsigned port)
        {
            _socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (_socket == INVALID_SOCKET)
            {
                throw exception("Socket failed with error: ");
            }

            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);
        }
        ~Server()
        {
            closesocket(_socket);
        }
        void Bind()
        {
            if (bind(_socket, (SOCKADDR*)&addr, sizeof(addr)) != NO_ERROR)
            {
                throw exception("Bind failed with error: ");
            }
        }
        Msg Recv() const
        {
            Msg _msg;

            char str[MSG_MAX];
            int bytesReceived = recvfrom(_socket, str, MSG_MAX, 0, (SOCKADDR*)&_msg.aFrom, &_msg.szaFrom);
            if (bytesReceived == SOCKET_ERROR)
            {
                throw exception("Recvfrom failed with error: ");
            }

            _msg.msg = str;
            return _msg;
        }
        void Send(const Msg& _msg) const
        {
            char str[MSG_MAX]{};
            strcpy_s(str, MSG_MAX, _msg.msg.data());

            int sendResult = sendto(_socket, str, _msg.msg.length() + 1, 0, (SOCKADDR*)&_msg.aFrom, _msg.szaFrom);
            if (sendResult == SOCKET_ERROR)
            {
                throw exception("Sendto failed with error: ");
            }
        }
    };
}

namespace Menu
{
    const pair<string, unsigned> Contain[] = {
       {"hamburger", 15 },
       {"sprite", 1},
       {"potato", 5},
    };

    const unsigned Size = sizeof(Contain) / (sizeof(string) + sizeof(unsigned));
};

struct Order
{
    sockaddr_in Customer;

    unsigned Num;
    unsigned Time;
    unsigned Content[Menu::Size]{};
};

void TakingOrders(const UDP::Server&);

list<Order> _orders;

int main()
{
    WSADATA WSAData;
    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != NO_ERROR)
    {
        cout << "WSAStartup failked with error;" << endl;
        return 1;
    }

    try
    {
        //Создание сервера
        string MyAdres;
        cout << "Enter server address: ";
        getline(cin, MyAdres, '\n');

        UDP::Server _socket(MyAdres, PORT);
        _socket.Bind();

        cout << "Server create correct;" << endl << endl;

        //Обмен сообщениями

        //Приём заказав
        thread _to(TakingOrders, _socket);
        _to.detach();

        //Инфо о приготовлении и её вывод серверу и ожд. клиентам
        while (true)
        {
            //Поиск готовых
            for (auto i = _orders.begin(); i != _orders.end(); i++)
            {
                if (i->Time <= 0)
                {
                    UDP::Msg send;
                    send.aFrom = i->Customer;
                    send.msg = "Your order is ready. Enjoy your meal.";
                    _socket.Send(send);

                    _orders.pop_front();
                    if ((i = _orders.begin()) == _orders.end()) break;
                }
            }

            //Собщение о ожидании
            string QueueInfo;
            QueueInfo += "Number\tTime\n";
            for (auto i = _orders.begin(); i != _orders.end(); i++)
            {
                QueueInfo += to_string(i->Num);
                QueueInfo += "\t"; 
                QueueInfo += to_string(i->Time);
                QueueInfo += "\n";
            }

            for (auto i = _orders.begin(); i != _orders.end(); i++)
            {
                UDP::Msg PersMsg;

                PersMsg.msg = "Your num " + to_string(i->Num) + ";\n";
                PersMsg.msg += QueueInfo;

                PersMsg.aFrom = i->Customer;
                _socket.Send(PersMsg);
            }

            //Готовка
            auto cooking = _orders.begin();
            if (cooking != _orders.end()) cooking->Time--;
            Sleep(1000);
        }
    }
    catch (const exception& ex)
    {
        cout << "Error: " << ex.what() << WSAGetLastError() << endl;
        return false;
    }

    WSACleanup();

    return true;
}

void TakingOrders(const UDP::Server& _socket)
{
    unsigned AllClients = 0;

    while (true)
    {
        try
        {
            UDP::Msg _msg = _socket.Recv();

            //Проверка на заполненность заведения
            if (_orders.size() > TABLES_COUNT)
            {
                UDP::Msg send;
                send.aFrom = _msg.aFrom;
                send.msg = "The place is full, please try again later.";
                _socket.Send(send);

                continue;
            }

            //Парсинг
            Order Temp;
            Temp.Customer = _msg.aFrom;
            Temp.Time = 0;

            for (int i = 0; i < Menu::Size; i++)
            {
                int pos = -1;

                //Содержание заказа
                while (true)
                {
                    pos = _msg.msg.find(Menu::Contain[i].first, pos + 1);
                    if (pos == string::npos) break;
                    else ++Temp.Content[i];
                };

                //Сколько времени займёт котовка
                Temp.Time += Temp.Content[i] * Menu::Contain[i].second;
            }

            //Добавление в очередь
            if (Temp.Time > 0)
            {
                Temp.Num = AllClients;
                ++AllClients;
                _orders.push_back(Temp);

                cout << "New order (" << Temp.Num << "): " << endl;
                for (unsigned i = 0; i < Menu::Size; i++)
                {
                    cout << Menu::Contain[i].first << " : " << Temp.Content[i] << endl;
                }
                cout << endl;
            }
            else
            {
                UDP::Msg send;
                send.aFrom = _msg.aFrom;
                send.msg = "Please, Place an order correctly.";
                _socket.Send(send);
            }
        }
        catch (const exception& ex)
        {
            cout << "Error: " << ex.what() << WSAGetLastError() << endl;
        }
    }
}
