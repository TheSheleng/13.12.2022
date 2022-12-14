#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>

#include <Windows.h>
#include <exception>
#include <string>
#include <iostream>

using namespace std;

#define PORT 22000

namespace UDP
{
    const unsigned MSG_MAX = 1024;

    class Client
    {
        SOCKET _socket;
        sockaddr_in addr;

    public:
        Client(string ip, unsigned port)
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
        ~Client()
        {
            closesocket(_socket);
        }
        string Recv() const
        {
            char msg[MSG_MAX]{};

            int bytesReceived = recvfrom(_socket, msg, MSG_MAX, 0, NULL, NULL);
            if (bytesReceived == SOCKET_ERROR)
            {
                throw exception("Recvfrom failed with error: ");
            }

            return msg;
        }
        void Send(const string msg) const
        {
            char str[MSG_MAX]{};
            strcpy_s(str, MSG_MAX, msg.data());

            int sendResult = sendto(_socket, msg.data(), msg.length() + 1, 0, (SOCKADDR*)&addr, sizeof(addr));
            if (sendResult == SOCKET_ERROR)
            {
                throw exception("Sendto failed with error: ");
            }
        }
    };
}

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
        //Создание клиента
        string MyAdres;
        cout << "Enter server address: ";
        getline(cin, MyAdres, '\n');

        UDP::Client _socket(MyAdres, PORT);

        cout << "Client create correct;" << endl << endl;

        //Обмен сообщениями
        cout << "Enter your order: ";
        while (true)
        {
            string order;
            getline(cin, order, '\n');

            _socket.Send(order);

            string msg = _socket.Recv();
            if (msg != "The place is full, please try again later." &&
                msg != "Please, Place an order correctly.") break;

            cout << msg << endl;
            cout << "Try again: ";
        }

        //Инфо о приготовлении и её вывод серверу и ожд. клинтам
        while (true)
        {
            string msg = _socket.Recv();
            system("cls");
            cout << msg << endl;

            if (msg == "Your order is ready. Enjoy your meal.") break;
        }

        system("pause");
    }
    catch (const exception& ex)
    {
        cout << "Error: " << ex.what() << WSAGetLastError() << endl;
        return false;
    }

    WSACleanup();

    return true;
}