using System;
using System.Data;
using System.Diagnostics;
using Microsoft.Data.Sqlite;
using ParallelBucketJoin.Domain;

namespace ParallelBucketJoin.Infrastructure;

public class DataGenerator : IDataGenerator
{
  private string ConnectionString => DatabaseConfiguration.GetConnectionString();

  public TimeSpan GenerateTestData(int keyCount, int minRecordsPerKey, int maxRecordsPerKey)
  {
    var stopwatch = Stopwatch.StartNew();

    EnsureDatabaseExists();
    ClearTables();

    GenerateMasterData(keyCount, minRecordsPerKey, maxRecordsPerKey);
    GenerateSlaveData(keyCount, minRecordsPerKey, maxRecordsPerKey);
    SortTables();

    stopwatch.Stop();
    return stopwatch.Elapsed;
  }

  private void EnsureDatabaseExists()
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    var createTables =
      @"
            CREATE TABLE IF NOT EXISTS Master (
                MKey INTEGER,
                MText TEXT,
                Num INTEGER
            );
            
            CREATE TABLE IF NOT EXISTS Slave (
                SKey INTEGER,
                SText TEXT,
                Num REAL
            );
            
            CREATE TABLE IF NOT EXISTS Master_srt (
                MKey INTEGER,
                MText TEXT,
                Num INTEGER
            );
            
            CREATE TABLE IF NOT EXISTS Slave_srt (
                SKey INTEGER,
                SText TEXT,
                Num REAL
            );
            
            CREATE TABLE IF NOT EXISTS Result (
                RKey INTEGER,
                RText INTEGER,
                NUMAVG REAL
            );
            
            CREATE INDEX IF NOT EXISTS idx_master_key ON Master(MKey);
            CREATE INDEX IF NOT EXISTS idx_slave_key ON Slave(SKey);
            CREATE INDEX IF NOT EXISTS idx_master_srt_key ON Master_srt(MKey);
            CREATE INDEX IF NOT EXISTS idx_slave_srt_key ON Slave_srt(SKey);
        ";

    using var command = new SqliteCommand(createTables, connection);
    command.ExecuteNonQuery();
  }

  private void GenerateMasterData(int keyCount, int minRecords, int maxRecords)
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var clearCmd = new SqliteCommand("DELETE FROM Master", connection);
    clearCmd.ExecuteNonQuery();

    var random = new Random();
    var letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    using var transaction = connection.BeginTransaction();

    for (int key = 1; key <= keyCount; key++)
    {
      int recordsCount = minRecords + random.Next(maxRecords - minRecords + 1);

      for (int i = 0; i < recordsCount; i++)
      {
        using var cmd = new SqliteCommand(
          "INSERT INTO Master (MKey, MText, Num) VALUES (@key, @text, @num)",
          connection,
          transaction
        );

        cmd.Parameters.AddWithValue("@key", key);
        cmd.Parameters.AddWithValue("@text", letters[random.Next(letters.Length)].ToString());
        cmd.Parameters.AddWithValue("@num", random.Next(1, 100));
        cmd.ExecuteNonQuery();
      }
    }

    transaction.Commit();
  }

  private void GenerateSlaveData(int keyCount, int minRecords, int maxRecords)
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var clearCmd = new SqliteCommand("DELETE FROM Slave", connection);
    clearCmd.ExecuteNonQuery();

    var random = new Random();
    var letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    using var transaction = connection.BeginTransaction();

    for (int key = 1; key <= keyCount; key++)
    {
      int recordsCount = minRecords + random.Next(maxRecords - minRecords + 1);

      for (int i = 0; i < recordsCount; i++)
      {
        using var cmd = new SqliteCommand(
          "INSERT INTO Slave (SKey, SText, Num) VALUES (@key, @text, @num)",
          connection,
          transaction
        );

        cmd.Parameters.AddWithValue("@key", key);
        cmd.Parameters.AddWithValue("@text", letters[random.Next(letters.Length)].ToString());
        cmd.Parameters.AddWithValue("@num", random.NextDouble() * 100);
        cmd.ExecuteNonQuery();
      }
    }

    transaction.Commit();
  }

  private void SortTables()
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var clearMaster = new SqliteCommand("DELETE FROM Master_srt", connection);
    using var clearSlave = new SqliteCommand("DELETE FROM Slave_srt", connection);
    clearMaster.ExecuteNonQuery();
    clearSlave.ExecuteNonQuery();

    using var sortMaster = new SqliteCommand(
      "INSERT INTO Master_srt SELECT * FROM Master ORDER BY MKey",
      connection
    );
    using var sortSlave = new SqliteCommand(
      "INSERT INTO Slave_srt SELECT * FROM Slave ORDER BY SKey",
      connection
    );

    sortMaster.ExecuteNonQuery();
    sortSlave.ExecuteNonQuery();
  }

  private void ClearTables()
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    var clearCommand = new SqliteCommand(
      @"DELETE FROM Master;
              DELETE FROM Slave;
              DELETE FROM Master_srt;
              DELETE FROM Slave_srt;
              DELETE FROM Result;",
      connection
    );
    clearCommand.ExecuteNonQuery();
  }
}
