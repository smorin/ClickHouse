#pragma once

#include <DB/Parsers/ASTSelectQuery.h>
#include <DB/Storages/IStorage.h>


namespace DB
{

class StorageView : public IStorage {

public:
	static StoragePtr create(const String & table_name_, const String & database_name_,
		Context & context_,	ASTPtr & query_, NamesAndTypesListPtr columns_);

	virtual std::string getName() const { return "View"; }
	virtual std::string getTableName() const { return table_name; }
	const NamesAndTypesList & getColumnsList() const { return *columns; }
	DB::ASTPtr getInnerQuery() const { return inner_query.clone(); };

	virtual BlockInputStreams read(
		const Names & column_names,
		ASTPtr query,
		const Settings & settings,
		QueryProcessingStage::Enum & processed_stage,
		size_t max_block_size = DEFAULT_BLOCK_SIZE,
		unsigned threads = 1);

	virtual void dropImpl();

protected:
	String select_database_name;
	String select_table_name;
	String table_name;
	String database_name;
	ASTSelectQuery inner_query;
	Context & context;
	NamesAndTypesListPtr columns;

	StorageView(const String & table_name_, const String & database_name_,
		Context & context_,	ASTPtr & query_, NamesAndTypesListPtr columns_);
};

}