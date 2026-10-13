using System;
using System.Globalization;
using System.IO;
using System.Text;
using System.Threading;

namespace ParallaxSimulation
{
    // All distances are millimetres. LCD centre is the origin; x right, y up,
    // z toward the observer. Source plane is z = -Gap. No Unity dependency.
    [Serializable]
    public sealed class OpticalSettings
    {
        public LenticularSettings lens = new LenticularSettings();
        // False is used only by analytic regression fixtures, not by the application.
        public bool useLens = true;
        public void SyncLens() {
            if(!useLens) return;
            if(lens==null) lens=new LenticularSettings();
            lens.Validate();
            lensToPanel=lens.screenDistance;lensToImage=lens.ImageDistance;
            sourceWidth=lens.ImageWidth;sourcePeriod=(1+lens.ImageDistance/lens.EffectiveObjectDistance)*lens.HorizontalPitch;
            rawPhaseStep=lens.ImageDistance/lens.EffectiveObjectDistance*lens.ledPitch;
            sourceOffset=0;slope=lens.slope;phaseDirection=-1;
        }
        public double panelWidth = 528, panelHeight = 297;
        public int pixelsX = 1920, pixelsY = 1080;
        public double lensToPanel = 6.5, lensToImage = 1.0273348519362187;
        public double sourcePeriod = 0.742873527842714, sourceWidth = 0.020364464692482914;
        public double rawPhaseStep = 0.15273348519362187, sourceOffset = 0;
        public double slope = 1.0 / 3.0;
        public int phaseDirection = -1;
        public double headX = 0, headY = 0, headZ = 600, ipd = 64, yawDegrees = 0;
        public double Gap { get { return lensToPanel - lensToImage; } }
        public OpticalSettings Copy() { var c=(OpticalSettings)MemberwiseClone();c.lens=lens==null?new LenticularSettings():lens.Copy();return c; }
        public void Validate()
        {
            SyncLens();
            double[] values = {panelWidth,panelHeight,lensToPanel,lensToImage,sourcePeriod,
                sourceWidth,rawPhaseStep,sourceOffset,slope,headX,headY,headZ,ipd,yawDegrees};
            foreach (double v in values)
                if (double.IsNaN(v) || double.IsInfinity(v)) throw new ArgumentException("Non-finite parameter");
            if (panelWidth <= 0 || panelHeight <= 0 || pixelsX < 1 || pixelsY < 1 || pixelsX > 8192 || pixelsY > 8192)
                throw new ArgumentException("Invalid panel dimensions");
            if ((!useLens && Gap <= 0) || sourcePeriod <= 0 || sourceWidth <= 0 || sourceWidth > sourcePeriod || ipd <= 0)
                throw new ArgumentException("Need positive gap and 0 < source width <= period");
            if (phaseDirection != -1 && phaseDirection != 1) throw new ArgumentException("Invalid phase direction");
            if (Eye(false).z <= 0 || Eye(true).z <= 0) throw new ArgumentException("Eyes must be in front of LCD");
        }
        // Eyes in this synthetic pose are labelled left=-x, right=+x.
        // A future camera adapter must explicitly map subject left/right and camera axes.
        public Point3 Eye(bool right)
        {
            double a = yawDegrees * Math.PI / 180, sign = right ? 1 : -1;
            return new Point3(headX + sign * ipd * .5 * Math.Cos(a), headY,
                headZ + sign * ipd * .5 * Math.Sin(a));
        }
        public double Phase(int k) { return sourceOffset + phaseDirection * k * rawPhaseStep; }
    }
    public struct Point3
    {
        public double x,y,z;
        public Point3(double x, double y, double z) { this.x=x; this.y=y; this.z=z; }
    }
    public static class OpticalModel
    {
        public static double Wrap(double value, double period)
        { return value - period * Math.Floor(value / period + .5); }
        public static Point3 Project(Point3 source, Point3 eye, double gap)
        {
            double t = gap / (eye.z + gap);
            return new Point3(source.x + t*(eye.x-source.x), source.y + t*(eye.y-source.y), 0);
        }
        public static double SourceCoordinate(double x, double y, Point3 eye, OpticalSettings s)
        {
            if(s.useLens) {
                double panel=x+s.lens.slope*y;
                double theta=(eye.x+s.lens.slope*eye.y-panel)/eye.z;
                return s.lens.Backtrace(panel,theta);
            }
            double ratio = s.Gap / eye.z;
            return (1+ratio)*(x+s.slope*y) - ratio*(eye.x+s.slope*eye.y);
        }
        public static bool Visible(double sourceCoordinate, int phase, OpticalSettings s)
        { return s.useLens?s.lens.IsLit(sourceCoordinate,phase):Math.Abs(Wrap(sourceCoordinate-s.Phase(phase),s.sourcePeriod)) <= s.sourceWidth*.5; }
        // bit 0 = left eye receives light, bit 1 = right eye receives light.
        public static int Classify(double x, double y, int phase, OpticalSettings s)
        {
            return (Visible(SourceCoordinate(x,y,s.Eye(false),s),phase,s)?1:0)
                | (Visible(SourceCoordinate(x,y,s.Eye(true),s),phase,s)?2:0);
        }
        // Exists a slot in the four-phase cycle where ONLY the specified eye sees this centre.
        public static int ExclusiveCoverage(double x, double y, OpticalSettings s)
        {
            int mask=0;
            for(int k=0;k<4;k++) { int c=Classify(x,y,k,s); if(c==1) mask|=1; if(c==2) mask|=2; }
            return mask;
        }
        // One LCD drive value must be shared by both observers when their paths overlap.
        public static double Drive(int classification,double left,double right,int conflict)
        {
            if(classification==1) return left;
            if(classification==2) return right;
            if(classification!=3 || conflict==0) return 0;
            return conflict==1?(left+right)*.5:left;
        }
        public static double Received(double x,double y,OpticalSettings geometry,Point3 targetLeft,
            Point3 targetRight,Point3 observer,double left,double right,int conflict,int phase,bool cycle)
        {
            double a=SourceCoordinate(x,y,targetLeft,geometry),b=SourceCoordinate(x,y,targetRight,geometry);
            double seen=SourceCoordinate(x,y,observer,geometry),sum=0;
            for(int k=0;k<4;k++) {
                if(!cycle && k!=phase) continue;
                int c=(Visible(a,k,geometry)?1:0)|(Visible(b,k,geometry)?2:0);
                if(Visible(seen,k,geometry)) sum+=Drive(c,left,right,conflict);
            }
            return sum/(cycle?4:1);
        }
    }
    public sealed class CoverageReport
    {
        public long[,] phaseCounts = new long[4,4];
        public long[] coverage = new long[4];
        public long[] totalRgb = new long[3], leftRgb = new long[3], rightRgb = new long[3], bothRgb = new long[3];
        public long pixelsBothAllRgb;
        public long Total { get { return coverage[0]+coverage[1]+coverage[2]+coverage[3]; } }
        public double Percent(long value) { return 100.0*value/Math.Max(1,Total); }
        public string Summary()
        {
            return string.Format(CultureInfo.InvariantCulture,
                "Exclusive slots available (4 phases, RGB centres)\nLeft: {0:F2}%   Right: {1:F2}%\nBoth eyes: {2:F2}%   Neither: {3:F2}%\nAll RGB covered for both eyes: {4} / {5} pixels",
                Percent(coverage[1]+coverage[3]),Percent(coverage[2]+coverage[3]),Percent(coverage[3]),Percent(coverage[0]),
                pixelsBothAllRgb,Total/3);
        }
        public void WriteCsv(string path, OpticalSettings s)
        {
            var b=new StringBuilder("category,phase,classification,count,total\n");
            string[] names={"neither","left_only","right_only","both_overlap"};
            for(int k=0;k<4;k++) for(int c=0;c<4;c++) b.AppendFormat(CultureInfo.InvariantCulture,"phase,{0},{1},{2},{3}\n",k,names[c],phaseCounts[k,c],Total);
            string[] cov={"neither_exclusive","left_exclusive_only","right_exclusive_only","both_exclusive"};
            for(int c=0;c<4;c++) b.AppendFormat(CultureInfo.InvariantCulture,"cycle,all,{0},{1},{2}\n",cov[c],coverage[c],Total);
            for(int c=0;c<3;c++) b.AppendFormat(CultureInfo.InvariantCulture,"rgb,{0},left_exclusive,{1},{2}\nrgb,{0},right_exclusive,{3},{2}\nrgb,{0},both_exclusive,{4},{2}\n", "RGB"[c],leftRgb[c],totalRgb[c],rightRgb[c],bothRgb[c]);
            b.AppendFormat(CultureInfo.InvariantCulture,"pixel,all,both_eyes_all_rgb,{0},{1}\n",pixelsBothAllRgb,Total/3);
            File.WriteAllText(path,b.ToString());
        }
    }
    public static class CoverageAnalyzer
    {
        // Tests each RGB subpixel centre, not its aperture or optical PSF.
        public static CoverageReport Analyze(OpticalSettings s, Action<float> progress, CancellationToken cancel)
        {
            s.Validate(); var r=new CoverageReport(); Point3 left=s.Eye(false), right=s.Eye(true);
            int nx=s.pixelsX*3;
            for(int y=0;y<s.pixelsY;y++)
            {
                cancel.ThrowIfCancellationRequested();
                double py=((y+.5)/s.pixelsY-.5)*s.panelHeight;
                bool pixelComplete=true;
                for(int x=0;x<nx;x++)
                {
                    double px=((x+.5)/nx-.5)*s.panelWidth;
                    double a=OpticalModel.SourceCoordinate(px,py,left,s), b=OpticalModel.SourceCoordinate(px,py,right,s);
                    int mask=0;
                    for(int k=0;k<4;k++)
                    {
                        int c=(OpticalModel.Visible(a,k,s)?1:0)|(OpticalModel.Visible(b,k,s)?2:0);
                        r.phaseCounts[k,c]++; if(c==1) mask|=1; if(c==2) mask|=2;
                    }
                    int rgb=x%3; r.coverage[mask]++; r.totalRgb[rgb]++;
                    if((mask&1)!=0) r.leftRgb[rgb]++; if((mask&2)!=0) r.rightRgb[rgb]++;
                    if(mask==3) r.bothRgb[rgb]++;
                    pixelComplete &= mask==3;
                    if(rgb==2) { if(pixelComplete) r.pixelsBothAllRgb++; pixelComplete=true; }
                }
                if(progress!=null) progress((y+1f)/s.pixelsY);
            }
            return r;
        }
    }
}
