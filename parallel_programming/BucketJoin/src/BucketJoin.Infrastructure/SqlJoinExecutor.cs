using System;
using BucketJoin.Domain;
using Microsoft.Data.Sqlite;

namespace BucketJoin.Infrastructure;

public class SqlJoinExecutor : ISqlJoinExecutor
{
  private string ConnectionString => DatabaseConfiguration.GetConnectionString();

  public TimeSpan ExecuteSqlJoin()
  {
    ClearResults();
    var startTime = DateTime.Now;

    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var command = new SqliteCommand(
        @"INSERT INTO JoinResult (KeyField, Value1, Value3, TotalValue, Description, Status)
              SELECT 
                  a.KeyField,
                  a.Value1,
                  b.Value3,
                  a.Value1 * b.Value3 as TotalValue,
                  a.Description,
                  b.Status
              FROM TableA_srt a
              INNER JOIN TableB_srt b ON a.KeyField = b.KeyField
              ORDER BY a.KeyField",
        connection
    );

    command.ExecuteNonQuery();

    return DateTime.Now - startTime;
  }

  private void ClearResults()
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var command = new SqliteCommand("DELETE FROM JoinResult", connection);
    command.ExecuteNonQuery();
  }
}
