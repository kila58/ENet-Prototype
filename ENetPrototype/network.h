#pragma once

#include <iostream>
#include <future>
#include <ppl.h>
#include <concurrent_vector.h>

#include <enetpp/include/server.h>
#include <enetpp/include/client.h>

#include "../3rdparty/json/json.h"
#include "../3rdparty/json/fifo_map.h"

template<class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;

using json = nlohmann::basic_json<my_workaround_fifo_map>;

struct NetworkCallback
{
	NetworkCallback(std::string name, std::function<void(json)> callback) : name(name), callback(callback) {}

	std::string name;
	std::function<void(json)> callback;
};

struct server_client
{
	unsigned int _uid;
	unsigned int get_uid() const { return _uid; } //MUST return globally unique value here
};

class Server
{
public:
	enetpp::trace_handler trace_handler;
	enetpp::server<server_client> server;
	std::mutex mutex;
	concurrency::concurrent_vector<json> queue;
	concurrency::concurrent_vector<NetworkCallback> callbacks;

public:
	std::unique_ptr<std::thread> Init();
	void Thread();
	void StartListening(const std::string& hostname, enet_uint16 port);
	void StopListening();
	void Send(int id, const std::string& to, json message);
	void Send(int id, const std::string& to, const std::string& message);
	void Invoke();
	void OnSync(const std::string& name, std::function<void(json)> function);
	void Consume();
};
extern Server server;

class Client
{
public:
	enetpp::trace_handler trace_handler;
	enetpp::client client;
	std::mutex mutex;
	concurrency::concurrent_vector<json> queue;
	concurrency::concurrent_vector<NetworkCallback> callbacks;

public:
	std::unique_ptr<std::thread> Init();
	void Thread();
	void Connect(const std::string& hostname, enet_uint16 port);
	void Disconnect();
	void Send(const std::string& to, json message);
	void Send(const std::string& to, const std::string& message);
	void Invoke();
	void OnSync(const std::string& name, std::function<void(json)> function);
	void Consume();
};
extern Client client;