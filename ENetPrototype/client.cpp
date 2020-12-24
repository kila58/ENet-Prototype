#include "network.h"

void clientthreadfunc()
{
	client.Thread();
}

std::unique_ptr<std::thread> Client::Init()
{
	return std::make_unique<std::thread>(&clientthreadfunc);
}

void Client::Thread()
{
	enetpp::global_state::get().initialize();

	trace_handler = [&](const std::string& msg)
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::cout << "client: " << msg << "\n";
	};

	client.set_trace_handler(trace_handler);

	Connect("localhost", 27017);

	Invoke();

	std::terminate();
}

void Client::Connect(const std::string& hostname, enet_uint16 port)
{
	client.connect(enetpp::client_connect_params()
		.set_channel_count(1)
		.set_server_host_name_and_port(hostname.c_str(), port));
}

void Client::Disconnect()
{
	//connected = false;

	client.disconnect();
}

void Client::Send(const std::string& to, json message)
{
	queue.push_back(json{ to, message });
}

void Client::Send(const std::string& to, const std::string& message)
{
	queue.push_back(json{ to, message });
}

void Client::Consume()
{
	static auto on_connected = [this]()
	{
		trace_handler("on_connected");

		//connected = true;
	};

	static auto on_disconnected = [this]()
	{
		trace_handler("on_disconnected");
	};

	static auto on_data_received = [this](const enet_uint8* data, size_t data_size)
	{
		try
		{
			json j_from_cbor = json::from_cbor(data, data + data_size);

			std::string callback_name = j_from_cbor[0];

			json message = j_from_cbor[1];

			for (auto& callback : callbacks)
			{
				if (callback.name == callback_name)
					callback.callback(message);
			}
		}
		catch (std::exception& e)
		{
			MessageBoxA(NULL, e.what(), e.what(), NULL);
		}
	};

	client.consume_events(on_connected, on_disconnected, on_data_received);
}

// on sync will wait til it receives some sort of response (blocking)
void Client::OnSync(const std::string& name, std::function<void(json)> function)
{
	std::promise<json> promise;

	callbacks.push_back(NetworkCallback(name, [&](json message) { promise.set_value(message); }));

	std::future<json> future = promise.get_future();

	function(future.get());
}

void Client::Invoke()
{
	while (client.is_connecting_or_connected())
	{
		//if (connected)
		{
			for (auto& msg : queue)
			{
				auto cbor = json::to_cbor(msg);

				client.send_packet(0, reinterpret_cast<const enet_uint8*>(cbor.data()), cbor.size(), ENET_PACKET_FLAG_RELIABLE);
			}

			queue.clear();
		}

		Consume();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

Client client;