using System;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using ParallelBucketJoin.Domain;

namespace ParallelBucketJoin.Presentation;

public partial class MainWindow : Window
{
  private readonly IDataGenerator _dataGenerator;
  private readonly IBucketMergeExecutor _bucketMerge;

  public MainWindow(IDataGenerator dataGenerator, IBucketMergeExecutor bucketMerge)
  {
    InitializeComponent();
    _dataGenerator = dataGenerator;
    _bucketMerge = bucketMerge;

    KeyCountBox.Text = "100";
    MinRecordsBox.Text = "5";
    MaxRecordsBox.Text = "15";
    ThreadCountBox.Text = Environment.ProcessorCount.ToString();

    ProcessorInfo.Text = $"Процессор: {Environment.ProcessorCount} потоков";
  }

  private async void GenerateData_Click(object sender, RoutedEventArgs e)
  {
    try
    {
      int keyCount = int.Parse(KeyCountBox.Text);
      int minRecords = int.Parse(MinRecordsBox.Text);
      int maxRecords = int.Parse(MaxRecordsBox.Text);

      if (minRecords > maxRecords)
      {
        await ShowMessage("Ошибка: Min должно быть меньше или равно Max");
        return;
      }

      GenerateStatus.Text = "Генерация данных...";

      var generationTime = await Task.Run(() =>
        _dataGenerator.GenerateTestData(keyCount, minRecords, maxRecords)
      );

      GenerateStatus.Text = $"Сгенерировано за {generationTime.TotalMilliseconds:F2} мс";
      await ShowMessage(
        $"Данные успешно сгенерированы!\nКлючей: {keyCount}\nЗаписей на ключ: {minRecords}-{maxRecords}"
      );
    }
    catch (Exception ex)
    {
      await ShowMessage($"Ошибка генерации: {ex.Message}");
    }
  }

  private async void ExecuteMerge_Click(object sender, RoutedEventArgs e)
  {
    try
    {
      int threadCount = GetThreadCount();

      MergeStatus.Text = "Выполнение Bucket Merge...";

      var executionTime = await Task.Run(() => _bucketMerge.ExecuteBucketMerge(threadCount));

      MergeStatus.Text =
        $"Выполнено ({threadCount} потоков): {executionTime.TotalMilliseconds:F2} мс";
      await ShowMessage(
        $"Bucket Merge выполнен успешно!\nПотоков: {threadCount}\nВремя: {executionTime.TotalMilliseconds:F2} мс"
      );
    }
    catch (Exception ex)
    {
      await ShowMessage($"Ошибка выполнения: {ex.Message}");
    }
  }

  private async void CompareParallel_Click(object sender, RoutedEventArgs e)
  {
    try
    {
      var threadCounts = new[] { 1, 2, 4, Environment.ProcessorCount };
      var results = new System.Text.StringBuilder();
      results.AppendLine("Сравнение производительности:");

      foreach (int threads in threadCounts)
      {
        MergeStatus.Text = $"Выполнение ({threads} потоков)...";

        var executionTime = await Task.Run(() => _bucketMerge.ExecuteBucketMerge(threads));

        results.AppendLine($"{threads} потоков: {executionTime.TotalMilliseconds:F2} мс");
      }

      MergeStatus.Text = "Сравнение завершено";
      await ShowMessage(results.ToString());
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
