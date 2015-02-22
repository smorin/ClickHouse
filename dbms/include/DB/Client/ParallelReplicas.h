#pragma once

#include <DB/Common/Throttler.h>
#include <DB/Client/Connection.h>
#include <DB/Client/ConnectionPool.h>


namespace DB
{


/** Для получения данных сразу из нескольких реплик (соединений) в рамках одного потока.
  * В качестве вырожденного случая, может также работать с одним соединением.
  *
  * Интерфейс почти совпадает с Connection.
  */
class ParallelReplicas final : private boost::noncopyable
{
public:
	/// Принимает готовое соединение.
	ParallelReplicas(Connection * connection_, Settings * settings_, ThrottlerPtr throttler_);

	/// Принимает пул, из которого нужно будет достать одно или несколько соединений.
	ParallelReplicas(IConnectionPool * pool_, Settings * settings_, ThrottlerPtr throttler_);

	/// Отправить на реплики всё содержимое внешних таблиц.
	void sendExternalTablesData(std::vector<ExternalTablesData> & data);

	/// Отправить запрос на реплики.
	void sendQuery(const String & query, const String & query_id = "",
					UInt64 stage = QueryProcessingStage::Complete, bool with_pending_data = false);

	/// Получить пакет от какой-нибудь реплики.
	Connection::Packet receivePacket();

	/// Разорвать все действующие соединения.
	void disconnect();

	/// Отменить запросы к репликам
	void sendCancel();

	/** На каждой реплике читать и пропускать все пакеты до EndOfStream или Exception.
	  * Возвращает EndOfStream, если не было получено никакого исключения. В противном
	  * случае возвращает последний полученный пакет типа Exception.
	  */
	Connection::Packet drain();

	/// Получить адреса реплик в виде строки.
	std::string dumpAddresses() const;

	/// Возвращает количесто реплик.
	size_t size() const { return replica_map.size(); }

	/// Проверить, есть ли действительные реплики.
	bool hasActiveReplicas() const { return active_replica_count > 0; }

private:
	/// Реплики хэшированные по id сокета
	using ReplicaMap = std::unordered_map<int, Connection *>;


	/// Зарегистрировать реплику.
	void registerReplica(Connection * connection);

	/// Получить реплику, на которой можно прочитать данные.
	ReplicaMap::iterator getReplicaForReading();

	/** Проверить, есть ли данные, которые можно прочитать на каких-нибудь репликах.
		* Возвращает одну такую реплику, если она найдётся.
		*/
	ReplicaMap::iterator waitForReadEvent();

	/// Пометить реплику как недействительную.
	void invalidateReplica(ReplicaMap::iterator it);


	Settings * settings;
	ReplicaMap replica_map;

	/// Если не nullptr, то используется, чтобы ограничить сетевой трафик.
	ThrottlerPtr throttler;

	std::vector<ConnectionPool::Entry> pool_entries;
	ConnectionPool::Entry pool_entry;

	/// Текущее количество действительных соединений к репликам.
	size_t active_replica_count;
	/// Запрос выполняется параллельно на нескольких репликах.
	bool supports_parallel_execution;
	/// Отправили запрос
	bool sent_query = false;
	/// Отменили запрос
	bool cancelled = false;
};

}