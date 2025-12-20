using System;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using BucketJoin.Domain;

namespace BucketJoin.Presentation;

public partial class MainWindow : Window
{
  private readonly IDataGenerator _dataGenerator;
  private readonly IBucketJoinExecutor _bucketJoin;
  private readonly ISqlJoinExecutor _sqlJoin;

  private TimeSpan? _lastGenerationTime;
  private TimeSpan? _lastBucketJoinTime;
  private TimeSpan? _lastSqlJoinTime;
  private int _lastThreadCount = 1;

  public MainWindow(
    IDataGenerator dataGenerator,
    IBucketJoinExecutor bucketJoin,
    ISqlJoinExecutor sqlJoin
  )
  {
    InitializeComponent();
    _dataGenerator = dataGenerator;
    _bucketJoin = bucketJoin;
    _sqlJoin = sqlJoin;

    OptimalThreads.Text = $"Доступно потоков процессора: 1-{Environment.ProcessorCount}";
    DataContext = new { ProcessorCount = Environment.ProcessorCount };
  }

  private async void GenerateData_Click(object sender, RoutedEventArgs e)
  {
    try
    {
      int recordCount = int.Parse(RecordCountBox.Text);
      int keyLength = int.Parse(KeyLengthBox.Text);

      GenerateTimeResult.Text = "Генерация данных...";

      var generationTime = await Task.Run(() =>
        _dataGenerator.GenerateTestData(recordCount, keyLength)
      );

      _lastGenerationTime = generationTime;
      GenerateTimeResult.Text = $"Время генерации: {generationTime.TotalMilliseconds:F2} мс";

      await ShowMessage("Данные успешно сгенерированы!");
    }
    catch (Exception ex)
    {
      await ShowMessage($"Ошибка: {ex.Message}");
    }
  }

  private async void BucketJoin_Click(object sender, RoutedEventArgs e)
  {
    try
    {
      int threadCount = GetThreadCount();
      _lastThreadCount = threadCount;

      BucketJoinResult.Text = "Выполнение...";

      var duration = await Task.Run(() => _bucketJoin.ExecuteBucketJoin(threadCount));

      _lastBucketJoinTime = duration;
      BucketJoinResult.Text = $"Время ({threadCount} потоков): {duration.TotalMilliseconds:F2} мс";
    }
    catch (Exception ex)
    {
      await ShowMessage($"Ошибка Bucket Join: {ex.Message}");
      BucketJoinResult.Text = "Ошибка";
    }
  }

  private async void SqlJoin_Click(object sender, RoutedEventArgs e)
  {
    try
    {
      SqlJoinResult.Text = "Выполнение...";

      var duration = await Task.Run(() => _sqlJoin.ExecuteSqlJoin());

      _lastSqlJoinTime = duration;
      SqlJoinResult.Text = $"Время: {duration.TotalMilliseconds:F2} мс";
    }
    catch (Exception ex)
    {
      await ShowMessage($"Ошибка SQL Join: {ex.Message}");
      SqlJoinResult.Text = "Ошибка";
    }
  }

  private async void Compare_Click(object sender, RoutedEventArgs e)
  {
    try
    {
      int threadCount = GetThreadCount();
      _lastThreadCount = threadCount;

      // Сбрасываем предыдущие результаты
      ComparisonResult.Text = "";

      var bucketTime = await Task.Run(() => _bucketJoin.ExecuteBucketJoin(threadCount));
      var sqlTime = await Task.Run(() => _sqlJoin.ExecuteSqlJoin());

      _lastBucketJoinTime = bucketTime;
      _lastSqlJoinTime = sqlTime;

      BucketJoinResult.Text =
        $"Время ({threadCount} потоков): {bucketTime.TotalMilliseconds:F2} мс";
      SqlJoinResult.Text = $"Время: {sqlTime.TotalMilliseconds:F2} мс";

      var difference = Math.Abs(bucketTime.TotalMilliseconds - sqlTime.TotalMilliseconds);
      var fasterBy = bucketTime < sqlTime ? "Bucket Join" : "SQL Join";
      var speedup =
        Math.Max(bucketTime.TotalMilliseconds, sqlTime.TotalMilliseconds)
        / Math.Min(bucketTime.TotalMilliseconds, sqlTime.TotalMilliseconds);

      ComparisonResult.Text =
        $"Bucket Join быстрее в {speedup:F2}x\n" + $"Разница: {difference:F2} мс";

      await ShowMessage(
        $"Результаты сравнения (потоков: {threadCount})\n"
          + $"Bucket Join: {bucketTime.TotalMilliseconds:F2} мс\n"
          + $"SQL Join:    {sqlTime.TotalMilliseconds:F2} мс\n"
          + $"Разница:     {difference:F2} мс\n"
          + $"Быстрее:     {fasterBy} ({speedup:F2}x)"
      );
    }
    catch (Exception ex)
    {
      await ShowMessage($"Ошибка сравнения: {ex.Message}");
    }
  }

  private int GetThreadCount()
  {
    if (int.TryParse(ThreadCountBox.Text, out int threadCount) && threadCount > 0)
    {
      return Math.Min(threadCount, Environment.ProcessorCount * 2);
    }
    return 1;
  }

  private async Task ShowMessage(string message)
  {
    var dialog = new Window
    {
      Title = "Сообщение",
      Content = new TextBlock { Text = message, Margin = new Thickness(20) },
      SizeToContent = SizeToContent.WidthAndHeight,
      WindowStartupLocation = WindowStartupLocation.CenterOwner,
      CanResize = false,
    };

    await dialog.ShowDialog(this);
  }
}
