using System;
using System.ComponentModel;

namespace BucketJoin.Presentation;

public class MainWindowViewModel : INotifyPropertyChanged
{
  private int _processorCount;
  private string _optimalThreads;
  private string _statisticsText = "Запустите операции для отображения статистики";

  public int ProcessorCount
  {
    get => _processorCount;
    set
    {
      if (_processorCount != value)
      {
        _processorCount = value;
        OnPropertyChanged(nameof(ProcessorCount));
      }
    }
  }

  public string OptimalThreads
  {
    get => _optimalThreads;
    set
    {
      if (_optimalThreads != value)
      {
        _optimalThreads = value;
        OnPropertyChanged(nameof(OptimalThreads));
      }
    }
  }

  public string StatisticsText
  {
    get => _statisticsText;
    set
    {
      if (_statisticsText != value)
      {
        _statisticsText = value;
        OnPropertyChanged(nameof(StatisticsText));
      }
    }
  }

  public MainWindowViewModel()
  {
    ProcessorCount = Environment.ProcessorCount;
    OptimalThreads = $"Рекомендуется: 1-{ProcessorCount}";
  }

  public event PropertyChangedEventHandler? PropertyChanged;

  protected virtual void OnPropertyChanged(string propertyName)
  {
    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
  }
}
