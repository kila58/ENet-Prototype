#include <chrono>

#define ENET_IMPLEMENTATION
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <enet/include/enet.h>

#include "network.h"

void mainloop()
{
	int tick = 0;
	int tickrate = 60;
	auto interval = 1000 / tickrate;
	auto curtime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	auto elapsedtime = curtime;

	json j;

	while (true)
	{
		while ((elapsedtime + interval) < curtime)
		{
			for (auto c : server.server.get_connected_clients())
			{
				j["tick"] = tick;

				server.Send(c->get_uid(), "TickPacket", j);
			}

			elapsedtime += interval;
			tick++;
			//std::cout << "tick" << "\n";
		}

		curtime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
}

int main()
{
	auto s = server.Init();
	auto c = client.Init();
	auto g = std::make_unique<std::thread>(&mainloop);

	client.callbacks.push_back(NetworkCallback("TickPacket", [&](json message) {
		std::cout << message["tick"] << "\n";
		}));

	client.callbacks.push_back(NetworkCallback("JoinMSGPacket", [&](json message){
		std::cout << std::string(message[0]) << "\n";
		}));

	s->join();
	c->join();
	g->join();
}