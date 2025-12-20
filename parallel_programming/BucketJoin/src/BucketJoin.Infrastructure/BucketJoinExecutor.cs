using System;
using System.Collections.Generic;
using BucketJoin.Domain;
using Microsoft.Data.Sqlite;

namespace BucketJoin.Infrastructure;

public class BucketJoinExecutor : IBucketJoinExecutor
{
  private string ConnectionString => DatabaseConfiguration.GetConnectionString();

  private struct DataBucket
  {
    public string Key;
    public List<TableARow> Rows;
  }

  public TimeSpan ExecuteBucketJoin(int threadCount = 1)
  {
    if (threadCount > 1)
    {
      var parallelExecutor = new BucketJoinExecutorParallel();
      return parallelExecutor.ExecuteBucketJoin(threadCount);
    }

    ClearResults();
    var startTime = DateTime.Now;

    var tableAData = GetSortedTableDataA();
    var tableBData = GetSortedTableDataB();

    var joinResults = PerformBucketJoin(tableAData, tableBData);
    SaveJoinResults(joinResults);

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
          Value4: DateTime.Parse(reader.GetString(2)), // SQLite stores dates as text
          Status: reader.GetString(3)
        )
      );
    }

    return results;
  }

  private List<JoinResult> PerformBucketJoin(List<TableARow> tableA, List<TableBRow> tableB)
  {
    var results = new List<JoinResult>();
    var bucket = new DataBucket { Rows = new List<TableARow>() };

    int indexA = 0;
    int indexB = 0;

    while (indexA < tableA.Count && indexB < tableB.Count)
    {
      var rowA = tableA[indexA];
      var rowB = tableB[indexB];

      int comparison = string.Compare(rowA.KeyField, rowB.KeyField, StringComparison.Ordinal);

      if (comparison < 0)
      {
        indexA++;
      }
      else if (comparison > 0)
      {
        indexB++;
      }
      else
      {
        string currentKey = rowA.KeyField;

        bucket.Key = currentKey;
        bucket.Rows.Clear();

        while (indexA < tableA.Count && tableA[indexA].KeyField == currentKey)
        {
          bucket.Rows.Add(tableA[indexA]);
          indexA++;
        }

        while (indexB < tableB.Count && tableB[indexB].KeyField == currentKey)
        {
          foreach (var bucketRow in bucket.Rows)
          {
            var record = new JoinResult(
              KeyField: currentKey,
              Value1: bucketRow.Value1,
              Value3: rowB.Value3,
              TotalValue: bucketRow.Value1 * rowB.Value3,
              Description: bucketRow.Description,
              Status: rowB.Status
            );
            results.Add(record);
          }
          indexB++;
        }
      }
    }

    return results;
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
