using System;
using System.Threading;
using ParallaxSimulation;

public static class ModelChecks
{
    static void Check(bool condition,string message) { if(!condition) throw new Exception(message); }
    static void Near(double a,double b,string message) { Check(Math.Abs(a-b)<1e-8,message+": "+a+" != "+b); }
    public static void Run()
    {
        var s=new OpticalSettings();s.Validate();Near(s.Gap,.141,"Plane spacing");
        // Independent analytic example: line from (10,20,-2) to (30,60,8)
        // reaches z=0 at one fifth of its length, at (14,28,0).
        var p=OpticalModel.Project(new Point3(10,20,-2),new Point3(30,60,8),2);
        Near(p.x,14,"Ray plane x");Near(p.y,28,"Ray plane y");
        var eye=s.Eye(false);var src=new Point3(.34,-.7,-s.Gap);
        p=OpticalModel.Project(src,eye,s.Gap);
        Near(OpticalModel.SourceCoordinate(p.x,p.y,eye,s),src.x+s.slope*src.y,"Projection/back-projection");
        Near(OpticalModel.Wrap(-.8,.7),-.1,"Negative modulo");
        Near(OpticalModel.Wrap(7.0,.7),0,"Repeated periods");
        Near(s.Phase(1)-s.Phase(0),-.96227,"Inverted image motion");
        Check(OpticalModel.Visible(s.Phase(2),2,s),"Band centre must be visible");
        Check(!OpticalModel.Visible(s.Phase(2)+s.sourceWidth*.6,2,s),"Outside band must be dark");
        // Coincident viewpoints cannot receive different images from one subpixel in any phase.
        s.ipd=.00000001;s.pixelsX=12;s.pixelsY=7;
        var r=CoverageAnalyzer.Analyze(s,null,CancellationToken.None);
        Check(r.Total==12*7*3,"All RGB centres counted");
        Check(r.coverage[3]==0,"Coincident eyes must not be counted as exclusive stereo");
        for(int k=0;k<4;k++) {long sum=0;for(int c=0;c<4;c++) sum+=r.phaseCounts[k,c];Check(sum==r.Total,"Phase totals");}
        // Designed ideal case: four equal-width phases, eyes separated by half a source period.
        // Every sampled centre then has independent left/right slots across the cycle.
        s=new OpticalSettings {lensToImage=6,lensToPanel=7,headZ=100,sourcePeriod=1,sourceWidth=.25,rawPhaseStep=.25,ipd=50,pixelsX=41,pixelsY=17,panelWidth=4.13,panelHeight=1.71};
        r=CoverageAnalyzer.Analyze(s,null,CancellationToken.None);
        Check(r.coverage[3]==r.Total,"Constructed full-resolution case");
        Check(r.pixelsBothAllRgb==s.pixelsX*s.pixelsY,"Full RGB pixels");
        // Same LCD drive is used for both eyes; a physically overlapping pixel cannot
        // independently provide different source values to each eye.
        Near(OpticalModel.Drive(3,1,0,0),0,"Overlap blackout");
        Near(OpticalModel.Drive(3,1,0,1),.5,"Shared overlap mixture");
        Near(OpticalModel.Drive(3,1,0,2),1,"Left priority shared drive");
        Near(OpticalModel.Drive(0,1,1,1),0,"No-ray drive");
        Near(OpticalModel.Received(.043,.078,s,s.Eye(false),s.Eye(true),s.Eye(false),.2,.8,0,0,true),.2/4,"Ideal left image delivery");
        Near(OpticalModel.Received(.043,.078,s,s.Eye(false),s.Eye(true),s.Eye(true),.2,.8,0,0,true),.8/4,"Ideal right image delivery");
        var actual=new OpticalSettings();
        Near(OpticalModel.Received(0,0,actual,actual.Eye(false),actual.Eye(true),actual.Eye(false),1,0,1,0,true),.125,"Overlap left received mean");
        Near(OpticalModel.Received(0,0,actual,actual.Eye(false),actual.Eye(true),actual.Eye(true),1,0,1,0,true),.125,"Overlap leaks into black right input");
        Near(OpticalModel.Received(0,0,actual,actual.Eye(false),actual.Eye(true),actual.Eye(true),1,0,0,0,true),0,"Blackout prevents this leak");
        Near(OpticalModel.Received(0,0,actual,actual.Eye(false),actual.Eye(true),actual.Eye(true),1,0,1,0,false),.5,"Single-phase brightness");
        var cancel=new CancellationTokenSource();cancel.Cancel();bool cancelled=false;
        try { CoverageAnalyzer.Analyze(s,null,cancel.Token); } catch(OperationCanceledException) {cancelled=true;}
        Check(cancelled,"Analysis cancellation");
        s.lensToPanel=s.lensToImage;bool rejected=false;
        try {s.Validate();} catch(ArgumentException) {rejected=true;}
        Check(rejected,"Reject nonpositive gap");
    }
}
