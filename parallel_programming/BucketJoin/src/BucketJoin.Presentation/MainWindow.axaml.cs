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
      var duration = await Task.Run(() => _bucketJoin.ExecuteBucketJoin());
      BucketJoinResult.Text = $"Время: {duration.TotalMilliseconds:F2} мс";
    }
    catch (Exception ex)
    {
      await ShowMessage($"Ошибка: {ex.Message}");
    }
  }

  private async void SqlJoin_Click(object sender, RoutedEventArgs e)
  {
    try
    {
      var duration = await Task.Run(() => _sqlJoin.ExecuteSqlJoin());
      SqlJoinResult.Text = $"Время: {duration.TotalMilliseconds:F2} мс";
    }
    catch (Exception ex)
    {
      await ShowMessage($"Ошибка: {ex.Message}");
    }
  }

  private async void Compare_Click(object sender, RoutedEventArgs e)
  {
    try
    {
      var bucketTime = await Task.Run(() => _bucketJoin.ExecuteBucketJoin());
      var sqlTime = await Task.Run(() => _sqlJoin.ExecuteSqlJoin());

      await ShowMessage(
        $"Bucket Join: {bucketTime.TotalMilliseconds:F2} мс\n"
          + $"SQL Join: {sqlTime.TotalMilliseconds:F2} мс\n"
          + $"Разница: {Math.Abs(bucketTime.TotalMilliseconds - sqlTime.TotalMilliseconds):F2} мс"
      );
    }
    catch (Exception ex)
    {
      await ShowMessage($"Ошибка: {ex.Message}");
    }
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
