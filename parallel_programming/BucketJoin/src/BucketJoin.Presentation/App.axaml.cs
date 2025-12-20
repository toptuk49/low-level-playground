using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using BucketJoin.Domain;
using BucketJoin.Infrastructure;

namespace BucketJoin.Presentation;

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
      IBucketJoinExecutor bucketJoin = new BucketJoinExecutor();
      ISqlJoinExecutor sqlJoin = new SqlJoinExecutor();

      desktop.MainWindow = new MainWindow(dataGenerator, bucketJoin, sqlJoin);
    }

    base.OnFrameworkInitializationCompleted();
  }
}
