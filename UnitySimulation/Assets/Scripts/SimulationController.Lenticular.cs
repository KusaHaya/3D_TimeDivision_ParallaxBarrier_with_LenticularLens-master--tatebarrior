using System;
using System.IO;
using System.Globalization;
using System.Text;
using UnityEngine;
namespace ParallaxSimulation
{
    public sealed partial class SimulationController
    {
        LenticularSettings lensTest=new LenticularSettings();
        Texture2D illumination;
        double[] lightProfile;
        double lensViewWidth=8,lensGain=.05;
        string lensKey="",lensMessage="",shownLensSummary="";
        double profileSpan;
        string exactD="150",exactT="6.5";
        void LensControls() {
            GUILayout.Label("LED / LENTICULAR TEST",heading);
            GUILayout.Label("Horizontal widths. 22.5 mm is LED centre pitch (not edge gap).",small);
            Slider("LED width (mm)",ref lensTest.ledWidth,.2f,10);
            Slider("LED centre pitch (mm)",ref lensTest.ledPitch,5,35);
            Slider("LED -> entrance FLAT face (mm)",ref lensTest.ledDistance,20,500,"F1");
            Slider("CONVEX VERTEX -> display (mm)",ref lensTest.screenDistance,.1f,20);
            GUILayout.Label("PMMA: flat face toward LED; convex face toward display.",small);
            Slider("PMMA index (assumed)",ref lensTest.refractiveIndex,1.45f,1.55f);
            Slider("Flat face -> vertex thickness (mm)",ref lensTest.thickness,1,3);
            GUILayout.Label("Radius 0.500 mm / physical pitch 0.700 mm / slope 1:3",small);
            GUILayout.Label("Calculated EFL: "+lensTest.FocalLength.ToString("F4")+" mm (not surface spacing)",small);
            GUILayout.Label("Exact entry: LED-to-flat / vertex-to-display (mm)",small);
            exactD=GUILayout.TextField(exactD);exactT=GUILayout.TextField(exactT);
            if(GUILayout.Button("Apply exact distances")) {
                double d,t;
                if(double.TryParse(exactD,NumberStyles.Float,CultureInfo.InvariantCulture,out d)
                    && double.TryParse(exactT,NumberStyles.Float,CultureInfo.InvariantCulture,out t)
                    && d>=20 && d<=500 && t>=.1 && t<=20) {
                    lensTest.ledDistance=d;lensTest.screenDistance=t;lensMessage="Exact distances applied.";
                }else lensMessage="Use D=20..500, display=0.1..20 mm (decimal point).";
            }
            GUILayout.Label("n=1.49 and 2 mm vertex thickness are modelling assumptions; no manufacturer focal-distance guarantee.",small);


            GUILayout.Label("Phase: only every fourth LED is lit",heading);
            lensTest.phase=GUILayout.Toolbar(lensTest.phase,new[]{"0","1","2","3"});
            GUILayout.Label("Lit LED groups each side: "+lensTest.groupsEachSide,small);
            lensTest.groupsEachSide=Mathf.RoundToInt(GUILayout.HorizontalSlider(lensTest.groupsEachSide,0,8));
            GUILayout.Label("0 = one LED through many adjacent lenslets",small);
            if(GUILayout.Button("Place display at calculated image plane")) lensTest.screenDistance=lensTest.ImageDistance;
            if(GUILayout.Button("Reset: LED 3 / 22.5 mm, D=150 mm")) lensTest=new LenticularSettings();
            Slider("View width (mm)",ref lensViewWidth,2,30,"F1");
            Slider("Fixed brightness gain",ref lensGain,.001f,.2f);
            if(GUILayout.Button("Export all 4 illumination phases")) ExportLensTest();
            GUILayout.Space(10);
            GUILayout.Label("Independent illumination test: no LCD image, observer, Arduino or old gap assumption. Thick plano-convex paraxial optics; uniform source and full lens aperture. No diffraction, aberration, scattering or absolute brightness.",small);
            GUILayout.Label(lensMessage,small);
        }
        void UpdateLensTest() {
            string key=JsonUtility.ToJson(lensTest)+"/"+lensViewWidth+"/"+lensGain;
            if(key==lensKey) return;
            lensTest.Validate();
            const int w=1000,h=340;
            double height=lensViewWidth*h/w;
            profileSpan=lensViewWidth+Math.Abs(lensTest.slope)*height;
            lightProfile=lensTest.Profile(8192,profileSpan);
            if(illumination==null) {illumination=new Texture2D(w,h,TextureFormat.RGB24,false,true);illumination.filterMode=FilterMode.Point;}
            var colors=new Color32[w*h];
            for(int y=0;y<h;y++)for(int x=0;x<w;x++) {
                double s=((x+.5)/w-.5)*lensViewWidth+lensTest.slope*((y+.5)/h-.5)*height;
                int index=Math.Max(0,Math.Min(lightProfile.Length-1,(int)((s/profileSpan+.5)*lightProfile.Length)));
                byte v=(byte)(255*Math.Min(1,lightProfile[index]*lensGain));
                colors[y*w+x]=new Color32(v,v,v,255);
            }
            illumination.SetPixels32(colors);illumination.Apply();lensKey=key;
            shownLensSummary=string.Format(CultureInfo.InvariantCulture,
                "Calculated image plane: {0:F4} mm from CONVEX VERTEX   |   image width: {1:F4} mm\nVertex -> display: {2:F4} mm   |   one LED / one lens footprint: {3:F4} mm\nSame-LED image centre period: {4:F5} mm   |   raw shift per LED: {5:F5} mm",
                lensTest.ImageDistance,lensTest.ImageWidth,lensTest.screenDistance,lensTest.ScreenWidth,lensTest.ScreenPeriod,lensTest.Magnification*lensTest.ledPitch);
        }
        void PlotLine(Vector2 a,Vector2 b,Color color,float width=1) {
            var matrix=GUI.matrix;var old=GUI.color;GUI.color=color;
            GUIUtility.RotateAroundPivot(Mathf.Atan2(b.y-a.y,b.x-a.x)*Mathf.Rad2Deg,a);
            GUI.DrawTexture(new Rect(a.x,a.y-width/2,(b-a).magnitude,width),Texture2D.whiteTexture);
            GUI.matrix=matrix;GUI.color=old;
        }
        void DrawLensPanels() {
            try {UpdateLensTest();}catch(Exception e){GUI.Label(new Rect(390,150,1000,80),e.Message);return;}
            GUI.Label(new Rect(390,130,1010,30),"DISPLAY ILLUMINATION / PHASE "+lensTest.phase+" / width "+lensViewWidth.ToString("F1")+" mm",heading);
            GUI.DrawTexture(new Rect(390,165,1000,340),illumination,ScaleMode.StretchToFill,false);
            GUI.Label(new Rect(390,510,1010,72),shownLensSummary,small);
            Rect plot=new Rect(390,620,1000,160);GUI.Box(plot,"");
            double peak=0;foreach(double v in lightProfile)peak=Math.Max(peak,v);
            GUI.Label(new Rect(390,587,1010,30),"HORIZONTAL PROFILE at y=0 / relative density / graph maximum "+peak.ToString("F2"),heading);
            Vector2 previous=Vector2.zero;
            for(int i=0;i<1000;i++) {
                double x=(i/999.0-.5)*lensViewWidth;
                int index=Math.Max(0,Math.Min(lightProfile.Length-1,(int)((x/profileSpan+.5)*lightProfile.Length)));
                var point=new Vector2(plot.x+i,plot.yMax-(float)(lightProfile[index]/Math.Max(peak,1e-9))*plot.height);
                if(i>0)PlotLine(previous,point,new Color(.3f,1,.5f));previous=point;
            }
            GUI.Label(new Rect(390,785,1010,32),"Profile edges: -"+(lensViewWidth/2).ToString("F2")+" to +"+(lensViewWidth/2).ToString("F2")+" mm. Image uses fixed gain; graph rescales its vertical axis.",small);
            GUI.Label(new Rect(390,830,1010,95),"Widths are horizontal FULL geometric support, not FWHM. Multiple lens/LED footprints can overlap. Relative density is normalized to unit integral per LED/lens pair: it is not measured irradiance and cannot predict absolute power when distances change. Index and vertex thickness are assumed. Large off-axis angles exceed the accuracy of this paraxial model.",small);
        }
        void ExportLensTest() {
            int saved=lensTest.phase;
            try {
                Directory.CreateDirectory(OutputDirectory);
                string prefix=Path.Combine(OutputDirectory,DateTime.Now.ToString("yyyyMMdd-HHmmss")+"-lenticular");
                for(int k=0;k<4;k++) {
                    lensTest.phase=k;UpdateLensTest();
                    File.WriteAllBytes(prefix+"-phase"+k+".png",illumination.EncodeToPNG());
                    var csv=new StringBuilder("horizontal_oblique_coordinate_mm,relative_density\n");
                    for(int i=0;i<lightProfile.Length;i++)csv.AppendFormat(CultureInfo.InvariantCulture,"{0:R},{1:R}\n",((i+.5)/lightProfile.Length-.5)*profileSpan,lightProfile[i]);
                    File.WriteAllText(prefix+"-phase"+k+".csv",csv.ToString());
                }
                lensTest.phase=saved;
                File.WriteAllText(prefix+"-settings.json",JsonUtility.ToJson(lensTest,true));
                File.WriteAllText(prefix+"-readme.txt","Horizontal coordinate, mm. Image width="+lensViewWidth+" mm; height=image width*0.34. Fixed brightness gain="+lensGain+". Relative density normalized per LED/lens pair; not absolute irradiance. Flat entrance toward LED; convex vertex toward display. n and thickness assumed.\n"+shownLensSummary);
                lensMessage="Saved: "+OutputDirectory;
            }catch(Exception e){lensMessage=e.Message;}
            finally{lensTest.phase=saved;lensKey="";}
        }
    }
}
