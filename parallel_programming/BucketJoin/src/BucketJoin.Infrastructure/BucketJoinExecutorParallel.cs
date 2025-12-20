using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Threading.Tasks;
using BucketJoin.Domain;
using Microsoft.Data.Sqlite;

namespace BucketJoin.Infrastructure;

public class BucketJoinExecutorParallel : IBucketJoinExecutor
{
  private string ConnectionString => DatabaseConfiguration.GetConnectionString();

  public TimeSpan ExecuteBucketJoin()
  {
    ClearResults();
    var startTime = DateTime.Now;

    var tableAData = GetSortedTableDataA();
    var tableBData = GetSortedTableDataB();

    var results = PerformParallelBucketJoin(tableAData, tableBData);
    SaveJoinResults(results);

    return DateTime.Now - startTime;
  }

  private List<TableARow> GetSortedTableDataA()
  {
    var results = new List<TableARow>();

    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var command = new SqliteCommand(
        "SELECT KeyField, Value1, Value2, Description FROM TableA_srt ORDER BY KeyField",
        connection
    );

    using var reader = command.ExecuteReader();

    while (reader.Read())
    {
      results.Add(
          new TableARow(
              KeyField: reader.GetString(0),
              Value1: reader.GetInt32(1),
              Value2: reader.GetDouble(2),
              Description: reader.GetString(3)
          )
      );
    }

    return results;
  }

  private List<TableBRow> GetSortedTableDataB()
  {
    var results = new List<TableBRow>();

    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var command = new SqliteCommand(
        "SELECT KeyField, Value3, Value4, Status FROM TableB_srt ORDER BY KeyField",
        connection
    );

    using var reader = command.ExecuteReader();

    while (reader.Read())
    {
      results.Add(
          new TableBRow(
              KeyField: reader.GetString(0),
              Value3: reader.GetInt32(1),
              Value4: DateTime.Parse(reader.GetString(2)),
              Status: reader.GetString(3)
          )
      );
    }

    return results;
  }

  private List<JoinResult> PerformParallelBucketJoin(
      List<TableARow> tableA,
      List<TableBRow> tableB
  )
  {
    var results = new ConcurrentBag<JoinResult>();
    var uniqueKeys = GetUniqueKeys(tableA, tableB);

    Parallel.ForEach(
        uniqueKeys,
        key =>
        {
          var bucketA = tableA.FindAll(x => x.KeyField == key);
          var bucketB = tableB.FindAll(x => x.KeyField == key);

          foreach (var rowA in bucketA)
          {
            foreach (var rowB in bucketB)
            {
              var record = new JoinResult(
                      KeyField: key,
                      Value1: rowA.Value1,
                      Value3: rowB.Value3,
                      TotalValue: rowA.Value1 * rowB.Value3,
                      Description: rowA.Description,
                      Status: rowB.Status
                  );
              results.Add(record);
            }
          }
        }
    );

    return new List<JoinResult>(results);
  }

  private HashSet<string> GetUniqueKeys(List<TableARow> tableA, List<TableBRow> tableB)
  {
    var keys = new HashSet<string>();

    foreach (var row in tableA)
      keys.Add(row.KeyField);
    foreach (var row in tableB)
      keys.Add(row.KeyField);

    return keys;
  }

  private void SaveJoinResults(List<JoinResult> results)
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var transaction = connection.BeginTransaction();

    foreach (var record in results)
    {
      using var cmd = new SqliteCommand(
          "INSERT INTO JoinResult (KeyField, Value1, Value3, TotalValue, Description, Status) "
              + "VALUES (@key, @v1, @v3, @total, @desc, @status)",
          connection,
          transaction
      );

      cmd.Parameters.AddWithValue("@key", record.KeyField);
      cmd.Parameters.AddWithValue("@v1", record.Value1);
      cmd.Parameters.AddWithValue("@v3", record.Value3);
      cmd.Parameters.AddWithValue("@total", record.TotalValue);
      cmd.Parameters.AddWithValue("@desc", record.Description);
      cmd.Parameters.AddWithValue("@status", record.Status);
      cmd.ExecuteNonQuery();
    }

    transaction.Commit();
  }

  private void ClearResults()
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var command = new SqliteCommand("DELETE FROM JoinResult", connection);
    command.ExecuteNonQuery();
  }
}
