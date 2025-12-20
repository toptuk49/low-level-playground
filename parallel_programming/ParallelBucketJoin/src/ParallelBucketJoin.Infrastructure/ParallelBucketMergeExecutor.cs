using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.Data.Sqlite;
using ParallelBucketJoin.Domain;

namespace ParallelBucketJoin.Infrastructure;

public class ParallelBucketMergeExecutor : IBucketMergeExecutor
{
  private string ConnectionString => DatabaseConfiguration.GetConnectionString();

  public TimeSpan ExecuteBucketMerge(int threadCount = 1)
  {
    var stopwatch = Stopwatch.StartNew();
    ClearResults();

    var masterData = LoadSortedMasterData();
    var slaveData = LoadSortedSlaveData();

    var uniqueKeys = GetUniqueKeys(masterData, slaveData);

    var workers = new BucketMergeWorker[threadCount];
    var tasks = new Task<List<JoinResult>>[threadCount];

    var keyGroups = SplitKeys(uniqueKeys, threadCount);

    for (int i = 0; i < threadCount; i++)
    {
      workers[i] = new BucketMergeWorker();
      int threadIndex = i;

      tasks[i] = Task.Run(() =>
        workers[threadIndex].ProcessKeyRange(masterData, slaveData, keyGroups[threadIndex])
      );
    }

    Task.WaitAll(tasks);

    var allResults = new List<JoinResult>();
    foreach (var task in tasks)
    {
      allResults.AddRange(task.Result);
    }

    allResults.Sort((a, b) => a.RKey.CompareTo(b.RKey));
    SaveResults(allResults);

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

  private HashSet<int> GetUniqueKeys(List<MasterRow> masterData, List<SlaveRow> slaveData)
  {
    var keys = new HashSet<int>();

    foreach (var row in masterData)
      keys.Add(row.MKey);
    foreach (var row in slaveData)
      keys.Add(row.SKey);

    return keys;
  }

  private List<int>[] SplitKeys(HashSet<int> keys, int threadCount)
  {
    var keyList = keys.ToList();
    keyList.Sort();

    var groups = new List<int>[threadCount];
    for (int i = 0; i < threadCount; i++)
    {
      groups[i] = new List<int>();
    }

    for (int i = 0; i < keyList.Count; i++)
    {
      int groupIndex = i % threadCount;
      groups[groupIndex].Add(keyList[i]);
    }

    return groups;
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

public class BucketMergeWorker : IBucketMergeWorker
{
  public List<JoinResult> ProcessKeyRange(
    List<MasterRow> masterData,
    List<SlaveRow> slaveData,
    List<int> keysToProcess
  )
  {
    var results = new List<JoinResult>();

    if (keysToProcess == null || keysToProcess.Count == 0)
      return results;

    var masterGroups = new Dictionary<int, List<MasterRow>>();
    var slaveGroups = new Dictionary<int, List<SlaveRow>>();

    foreach (var row in masterData)
    {
      if (keysToProcess.Contains(row.MKey))
      {
        if (!masterGroups.ContainsKey(row.MKey))
          masterGroups[row.MKey] = new List<MasterRow>();
        masterGroups[row.MKey].Add(row);
      }
    }

    foreach (var row in slaveData)
    {
      if (keysToProcess.Contains(row.SKey))
      {
        if (!slaveGroups.ContainsKey(row.SKey))
          slaveGroups[row.SKey] = new List<SlaveRow>();
        slaveGroups[row.SKey].Add(row);
      }
    }

    foreach (int key in keysToProcess)
    {
      if (!masterGroups.ContainsKey(key) || !slaveGroups.ContainsKey(key))
        continue;

      var masterRows = masterGroups[key];
      var slaveRows = slaveGroups[key];

      int totalTexts = 0;
      float totalSum = 0;

      foreach (var masterRow in masterRows)
      {
        foreach (var slaveRow in slaveRows)
        {
          string combinedText = masterRow.MText + slaveRow.SText;
          totalTexts++;
          totalSum += masterRow.Num * slaveRow.Num;
        }
      }

      if (totalTexts > 0)
      {
        results.Add(new JoinResult(RKey: key, RText: totalTexts, NUMAVG: totalSum / totalTexts));
      }
    }

    return results;
  }
}
