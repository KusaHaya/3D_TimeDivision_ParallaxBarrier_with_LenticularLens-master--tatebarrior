using System;
namespace ParallaxSimulation
{
    // Paraxial cylindrical thin-lens model, horizontal section through slanted bars.
    [Serializable] public sealed class LenticularSettings
    {
        public double ledWidth=3,ledPitch=22.5,ledDistance=150,screenDistance=6.5;
        public double physicalPitch=.7,slope=1.0/3,focalLength=6.10;
        public int phase=0,groupsEachSide=4;
        public double HorizontalPitch {get{return physicalPitch*Math.Sqrt(1+slope*slope);}}
        public double Magnification {get{return screenDistance/ledDistance;}}
        public double Defocus {get{return 1+screenDistance/ledDistance-screenDistance/focalLength;}}
        public double ImageDistance {get{return ledDistance*focalLength/(ledDistance-focalLength);}}
        public double ImageWidth {get{return ImageDistance/ledDistance*ledWidth;}}
        public double ScreenWidth {get{return Magnification*ledWidth+Math.Abs(Defocus)*HorizontalPitch;}}
        public double ScreenPeriod {get{return (1+Magnification)*HorizontalPitch;}}
        public double Center(double lensCenter,double ledCenter) {return (1+Magnification)*lensCenter-Magnification*ledCenter;}
        public double Ray(double lensCenter,double apertureOffset,double ledPosition,double z)
        {return lensCenter+apertureOffset+z*((lensCenter+apertureOffset-ledPosition)/ledDistance-apertureOffset/focalLength);}
        public void Validate() {
            if(ledDistance<=focalLength || focalLength<=0 || screenDistance<=0 || ledWidth<=0 || ledPitch<=0 || physicalPitch<=0)
                throw new ArgumentException("Need LED distance > focal length and positive dimensions.");
        }
        // Convolution of two uniform intervals. Integral = 1 for each lens/LED pair.
        public static double Density(double x,double a,double b) {
            a=Math.Abs(a);b=Math.Abs(b);
            if(a<1e-10) return Math.Abs(x)<=b/2?1/b:0;
            if(b<1e-10) return Math.Abs(x)<=a/2?1/a:0;
            return Math.Max(0,Math.Min(a/2,x+b/2)-Math.Max(-a/2,x-b/2))/(a*b);
        }
        public double[] Profile(int count,double span) {
            Validate();var result=new double[count];double dx=span/count;
            double a=Math.Abs(Defocus)*HorizontalPitch,b=Magnification*ledWidth;
            for(int group=-groupsEachSide;group<=groupsEachSide;group++) {
                double led=(phase+4*group)*ledPitch;
                int first=(int)Math.Floor((-span/2-ScreenWidth/2+Magnification*led)/ScreenPeriod);
                int last=(int)Math.Ceiling((span/2+ScreenWidth/2+Magnification*led)/ScreenPeriod);
                for(int n=first;n<=last;n++) {
                    double center=Center(n*HorizontalPitch,led);
                    int lo=Math.Max(0,(int)Math.Floor((center-ScreenWidth/2+span/2)/dx));
                    int hi=Math.Min(count-1,(int)Math.Ceiling((center+ScreenWidth/2+span/2)/dx));
                    for(int i=lo;i<=hi;i++) {
                        // Four aperture samples per graph bin reduce raster aliasing.
                        for(int j=0;j<4;j++)result[i]+=.25*Density(-span/2+(i+(j+.5)/4)*dx-center,a,b);
                    }
                }
            }
            return result;
        }
    }
}
