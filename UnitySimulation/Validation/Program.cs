using System;
using System.Threading;
using ParallaxSimulation;
ModelChecks.Run();
Console.WriteLine("PASS: model checks");
var s=new OpticalSettings();
var r=CoverageAnalyzer.Analyze(s,null,CancellationToken.None);
Console.WriteLine(r.Summary());
r.WriteCsv("default-coverage.csv",s);
