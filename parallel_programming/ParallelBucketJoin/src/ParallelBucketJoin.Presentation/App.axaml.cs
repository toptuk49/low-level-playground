using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using ParallelBucketJoin.Domain;
using ParallelBucketJoin.Infrastructure;

namespace ParallelBucketJoin.Presentation;

public partial class App : Application
{
  public override void Initialize()
  {
    AvaloniaXamlLoader.Load(this);
  }

  public override void OnFrameworkInitializationCompleted()
  {
    if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
    {
      IDataGenerator dataGenerator = new DataGenerator();
      IBucketMergeExecutor bucketMerge = new BucketMergeExecutor();

      desktop.MainWindow = new MainWindow(dataGenerator, bucketMerge);
    }

    base.OnFrameworkInitializationCompleted();
  }
}
