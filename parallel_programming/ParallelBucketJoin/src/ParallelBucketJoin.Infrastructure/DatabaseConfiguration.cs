namespace ParallelBucketJoin.Infrastructure;

public static class DatabaseConfiguration
{
  public static string GetConnectionString()
  {
    return "Data Source=bucketjoin.db";
  }
}
