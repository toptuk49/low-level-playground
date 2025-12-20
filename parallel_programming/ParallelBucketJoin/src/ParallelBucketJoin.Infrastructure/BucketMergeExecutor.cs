using System;
using System.Collections.Generic;
using System.Diagnostics;
using Microsoft.Data.Sqlite;
using ParallelBucketJoin.Domain;

namespace ParallelBucketJoin.Infrastructure;

public class BucketMergeExecutor : IBucketMergeExecutor
{
  private string ConnectionString => DatabaseConfiguration.GetConnectionString();

  // Структура для хранения "ведра" записей Master с одинаковым ключом
  private struct MasterBucket
  {
    public int Key;
    public List<string> Texts;
    public List<int> Nums;
  }

  public TimeSpan ExecuteBucketMerge(int threadCount = 1)
  {
    if (threadCount > 1)
    {
      var parallelExecutor = new ParallelBucketMergeExecutor();
      return parallelExecutor.ExecuteBucketMerge(threadCount);
    }

    ClearResults();
    var stopwatch = Stopwatch.StartNew();

    var masterData = LoadSortedMasterData();
    var slaveData = LoadSortedSlaveData();

    var results = PerformSequentialBucketMerge(masterData, slaveData);
    SaveResults(results);

    stopwatch.Stop();
    return stopwatch.Elapsed;
  }

  private List<MasterRow> LoadSortedMasterData()
  {
    var results = new List<MasterRow>();

    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var command = new SqliteCommand(
      "SELECT MKey, MText, Num FROM Master_srt ORDER BY MKey",
      connection
    );

    using var reader = command.ExecuteReader();

    while (reader.Read())
    {
      results.Add(
        new MasterRow(MKey: reader.GetInt32(0), MText: reader.GetString(1), Num: reader.GetInt32(2))
      );
    }

    return results;
  }

  private List<SlaveRow> LoadSortedSlaveData()
  {
    var results = new List<SlaveRow>();

    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var command = new SqliteCommand(
      "SELECT SKey, SText, Num FROM Slave_srt ORDER BY SKey",
      connection
    );

    using var reader = command.ExecuteReader();

    while (reader.Read())
    {
      results.Add(
        new SlaveRow(
          SKey: reader.GetInt32(0),
          SText: reader.GetString(1),
          Num: (float)reader.GetDouble(2)
        )
      );
    }

    return results;
  }

  private List<JoinResult> PerformSequentialBucketMerge(
    List<MasterRow> masterData,
    List<SlaveRow> slaveData
  )
  {
    var results = new List<JoinResult>();
    var masterBucket = new MasterBucket { Texts = new List<string>(), Nums = new List<int>() };

    int masterIndex = 0;
    int slaveIndex = 0;

    while (masterIndex < masterData.Count && slaveIndex < slaveData.Count)
    {
      var masterRow = masterData[masterIndex];
      var slaveRow = slaveData[slaveIndex];

      int comparison = masterRow.MKey.CompareTo(slaveRow.SKey);

      if (comparison < 0)
      {
        masterIndex++;
      }
      else if (comparison > 0)
      {
        slaveIndex++;
      }
      else
      {
        int currentKey = masterRow.MKey;

        masterBucket.Key = currentKey;
        masterBucket.Texts.Clear();
        masterBucket.Nums.Clear();

        while (masterIndex < masterData.Count && masterData[masterIndex].MKey == currentKey)
        {
          masterBucket.Texts.Add(masterData[masterIndex].MText);
          masterBucket.Nums.Add(masterData[masterIndex].Num);
          masterIndex++;
        }

        int totalTexts = 0;
        float totalSum = 0;

        while (slaveIndex < slaveData.Count && slaveData[slaveIndex].SKey == currentKey)
        {
          var currentSlave = slaveData[slaveIndex];

          for (int i = 0; i < masterBucket.Texts.Count; i++)
          {
            string combinedText = masterBucket.Texts[i] + currentSlave.SText;
            totalTexts++;
            totalSum += masterBucket.Nums[i] * currentSlave.Num;
          }

          slaveIndex++;
        }

        if (totalTexts > 0)
        {
          results.Add(
            new JoinResult(RKey: currentKey, RText: totalTexts, NUMAVG: totalSum / totalTexts)
          );
        }
      }
    }

    return results;
  }

  private void SaveResults(List<JoinResult> results)
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var transaction = connection.BeginTransaction();

    foreach (var result in results)
    {
      using var cmd = new SqliteCommand(
        "INSERT INTO Result (RKey, RText, NUMAVG) VALUES (@key, @text, @num)",
        connection,
        transaction
      );

      cmd.Parameters.AddWithValue("@key", result.RKey);
      cmd.Parameters.AddWithValue("@text", result.RText);
      cmd.Parameters.AddWithValue("@num", result.NUMAVG);
      cmd.ExecuteNonQuery();
    }

    transaction.Commit();
  }

  private void ClearResults()
  {
    using var connection = new SqliteConnection(ConnectionString);
    connection.Open();

    using var command = new SqliteCommand("DELETE FROM Result", connection);
    command.ExecuteNonQuery();
  }
}
