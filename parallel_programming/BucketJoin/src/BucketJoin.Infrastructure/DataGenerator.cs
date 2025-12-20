using System;
using System.Data;
using BucketJoin.Domain;
using Microsoft.Data.Sqlite;

namespace BucketJoin.Infrastructure;

public class DataGenerator : IDataGenerator
{
  private string ConnectionString => DatabaseConfiguration.GetConnectionString();

  public void GenerateTestData(int recordCount, int keyLength = 2)
  {
    EnsureDatabaseExists();
    ClearTables();

    GenerateTableData(recordCount, "TableA", keyLength);
    GenerateTableData(recordCount, "TableB", keyLength);
    SortTables();
  }

  private void EnsureDatabaseExists()
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    var createTables =
        @"
        CREATE TABLE IF NOT EXISTS TableA (
            KeyField TEXT,
            Value1 INTEGER,
            Value2 REAL,
            Description TEXT
        );
        
        CREATE TABLE IF NOT EXISTS TableB (
            KeyField TEXT,
            Value3 INTEGER,
            Value4 TEXT,
            Status TEXT
        );
        
        CREATE TABLE IF NOT EXISTS TableA_srt (
            KeyField TEXT,
            Value1 INTEGER,
            Value2 REAL,
            Description TEXT
        );
        
        CREATE TABLE IF NOT EXISTS TableB_srt (
            KeyField TEXT,
            Value3 INTEGER,
            Value4 TEXT,
            Status TEXT
        );
        
        CREATE TABLE IF NOT EXISTS JoinResult (
            KeyField TEXT,
            Value1 INTEGER,
            Value3 INTEGER,
            TotalValue INTEGER,
            Description TEXT,
            Status TEXT
        );
        
        CREATE INDEX IF NOT EXISTS idx_tablea_key ON TableA(KeyField);
        CREATE INDEX IF NOT EXISTS idx_tableb_key ON TableB(KeyField);
        CREATE INDEX IF NOT EXISTS idx_tablea_srt_key ON TableA_srt(KeyField);
        CREATE INDEX IF NOT EXISTS idx_tableb_srt_key ON TableB_srt(KeyField);
        ";

    using var command = new SqliteCommand(createTables, connection);
    command.ExecuteNonQuery();
  }

  private void GenerateTableData(int count, string tableName, int keyLength)
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var clearCmd = new SqliteCommand($"DELETE FROM {tableName}", connection);
    clearCmd.ExecuteNonQuery();

    var random = new Random();
    string[] statuses = { "Active", "Inactive", "Pending", "Completed" };
    string[] descriptions = { "Item A", "Item B", "Item C", "Item D", "Item E" };

    using var transaction = connection.BeginTransaction();

    for (int i = 0; i < count; i++)
    {
      char firstChar = (char)('A' + random.Next(0, 5));
      char secondChar = (char)('A' + random.Next(0, 5));
      string key = $"{firstChar}{secondChar}";

      if (tableName == "TableA")
      {
        using var cmd = new SqliteCommand(
            "INSERT INTO TableA (KeyField, Value1, Value2, Description) VALUES (@key, @value1, @value2, @desc)",
            connection,
            transaction
        );

        cmd.Parameters.AddWithValue("@key", key);
        cmd.Parameters.AddWithValue("@value1", random.Next(1, 100));
        cmd.Parameters.AddWithValue("@value2", random.NextDouble() * 100);
        cmd.Parameters.AddWithValue(
            "@desc",
            descriptions[random.Next(descriptions.Length)]
        );
        cmd.ExecuteNonQuery();
      }
      else // TableB
      {
        using var cmd = new SqliteCommand(
            "INSERT INTO TableB (KeyField, Value3, Value4, Status) VALUES (@key, @value3, @value4, @status)",
            connection,
            transaction
        );

        cmd.Parameters.AddWithValue("@key", key);
        cmd.Parameters.AddWithValue("@value3", random.Next(1, 100));
        cmd.Parameters.AddWithValue(
            "@value4",
            DateTime.Now.AddDays(-random.Next(0, 365)).ToString("yyyy-MM-dd HH:mm:ss")
        );
        cmd.Parameters.AddWithValue("@status", statuses[random.Next(statuses.Length)]);
        cmd.ExecuteNonQuery();
      }
    }

    transaction.Commit();
  }

  private void SortTables()
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var clearA = new SqliteCommand("DELETE FROM TableA_srt", connection);
    using var clearB = new SqliteCommand("DELETE FROM TableB_srt", connection);
    clearA.ExecuteNonQuery();
    clearB.ExecuteNonQuery();

    using var sortA = new SqliteCommand(
        "INSERT INTO TableA_srt SELECT * FROM TableA ORDER BY KeyField",
        connection
    );
    using var sortB = new SqliteCommand(
        "INSERT INTO TableB_srt SELECT * FROM TableB ORDER BY KeyField",
        connection
    );

    sortA.ExecuteNonQuery();
    sortB.ExecuteNonQuery();
  }

  private void ClearTables()
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    var clearCommand = new SqliteCommand(
        @"DELETE FROM TableA;
              DELETE FROM TableB;
              DELETE FROM TableA_srt;
              DELETE FROM TableB_srt;
              DELETE FROM JoinResult;",
        connection
    );
    clearCommand.ExecuteNonQuery();
  }
}
