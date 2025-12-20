using System.Diagnostics;

namespace ParallelBucketJoin.Domain;

public record JoinResult(int RKey, int RText, float NUMAVG);

public record MasterRow(int MKey, string MText, int Num);

public record SlaveRow(int SKey, string SText, float Num);

public interface IDataGenerator
{
  TimeSpan GenerateTestData(int keyCount, int minRecordsPerKey, int maxRecordsPerKey);
}

public interface IBucketMergeExecutor
{
  TimeSpan ExecuteBucketMerge(int threadCount = 1);
}

public interface IBucketMergeWorker
{
  List<JoinResult> ProcessKeyRange(
    List<MasterRow> masterData,
    List<SlaveRow> slaveData,
    List<int> keysToProcess
  );
}
