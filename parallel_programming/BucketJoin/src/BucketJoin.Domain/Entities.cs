using System.Diagnostics;

namespace BucketJoin.Domain;

public record JoinResult(
  string KeyField,
  int Value1,
  int Value3,
  int TotalValue,
  string Description,
  string Status
);

public record TableARow(string KeyField, int Value1, double Value2, string Description);

public record TableBRow(string KeyField, int Value3, DateTime Value4, string Status);

public interface IDataGenerator
{
  TimeSpan GenerateTestData(int recordCount, int keyLength = 2);
}

public interface IBucketJoinExecutor
{
  TimeSpan ExecuteBucketJoin();
}

public interface ISqlJoinExecutor
{
  TimeSpan ExecuteSqlJoin();
}
