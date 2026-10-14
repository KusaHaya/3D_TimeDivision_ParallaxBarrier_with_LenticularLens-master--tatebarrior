using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using ParallaxSimulation;

public static class HardwareChecks
{
    static void Assert(bool ok,string message) {if(!ok)throw new Exception("Hardware check: "+message);}
    static void Word(List<byte> data,string word) {data.AddRange(Encoding.ASCII.GetBytes(word));data.Add(0);while(data.Count%4!=0)data.Add(0);}
    static byte[] Message(bool right,float x,float y,float z)
    {
        var b=new List<byte>();Word(b,right?"/eye/right":"/eye/left");Word(b,",fff");
        foreach(float value in new[]{x,y,z}) {byte[] f=BitConverter.GetBytes(value);if(BitConverter.IsLittleEndian)Array.Reverse(f);b.AddRange(f);}
        return b.ToArray();
    }
    static byte[] Bundle(params byte[][] children)
    {
        var b=new List<byte>();Word(b,"#bundle");b.AddRange(new byte[8]);
        foreach(var child in children) {int n=child.Length;b.Add((byte)(n>>24));b.Add((byte)(n>>16));b.Add((byte)(n>>8));b.Add((byte)n);b.AddRange(child);}
        return b.ToArray();
    }
    public static void Run()
    {
        int count=0;double[] decoded=null;
        var left=Message(false,-.032f,.012f,.6f);var right=Message(true,.032f,.013f,.61f);
        Assert(EyeOscReceiver.Parse(left,0,left.Length,0,(r,v)=>{Assert(!r,"eye label");decoded=v;count++;}),"valid OSC");
        Assert(count==1 && Math.Abs(decoded[0]+.032)<1e-7 && Math.Abs(decoded[2]-.6)<1e-7,"big endian float decoding");
        var bundle=Bundle(left,Bundle(right));count=0;
        Assert(EyeOscReceiver.Parse(bundle,0,bundle.Length,0,(r,v)=>count++) && count==2,"nested bundle");
        var invalid=Message(false,0,0,float.NaN);
        Assert(!EyeOscReceiver.Parse(invalid,0,invalid.Length,0,(r,v)=>{}),"reject NaN");
        invalid=Message(false,0,0,.01f);
        Assert(!EyeOscReceiver.Parse(invalid,0,invalid.Length,0,(r,v)=>{}),"reject invalid depth");
        Assert(!EyeOscReceiver.Parse(left,0,left.Length-1,0,(r,v)=>{}),"truncated payload");
        invalid=Bundle(left);invalid[16]=127;
        Assert(!EyeOscReceiver.Parse(invalid,0,invalid.Length,0,(r,v)=>{}),"oversized bundle entry");
        invalid=left;for(int i=0;i<10;i++)invalid=Bundle(invalid);
        Assert(!EyeOscReceiver.Parse(invalid,0,invalid.Length,0,(r,v)=>{}),"nesting limit");
        // Exercise the real UDP pair latch: a new single eye must not alter a published pair.
        int port;using(var reserve=new UdpClient(new IPEndPoint(IPAddress.Loopback,0)))port=((IPEndPoint)reserve.Client.LocalEndPoint).Port;
        using(var receiver=new EyeOscReceiver())using(var sender=new UdpClient()) {
            receiver.Open(port);sender.Send(left,left.Length,new IPEndPoint(IPAddress.Loopback,port));
            System.Threading.Thread.Sleep(10);receiver.Poll();Assert(!receiver.Fresh,"one eye does not publish");
            sender.Send(right,right.Length,new IPEndPoint(IPAddress.Loopback,port));
            System.Threading.Thread.Sleep(10);receiver.Poll();Assert(receiver.Fresh,"complete pair publishes");
            double previous=receiver.Left[0];var next=Message(false,-.05f,.01f,.6f);
            sender.Send(next,next.Length,new IPEndPoint(IPAddress.Loopback,port));
            System.Threading.Thread.Sleep(10);receiver.Poll();Assert(receiver.Left[0]==previous,"pair snapshot is atomic");
            System.Threading.Thread.Sleep(1050);Assert(!receiver.Fresh,"stale pair expires");
        }
        int width,height;var ppm=new List<byte>(Encoding.ASCII.GetBytes("P6\n# comment\n1 2\n255\n"));
        ppm.AddRange(new byte[]{10,32,35,255,0,128});
        var rgb=PpmImage.Decode(ppm.ToArray(),out width,out height);
        Assert(width==1 && height==2 && rgb[0]==255 && rgb[3]==10 && rgb[4]==32 && rgb[5]==35,"PPM rows and binary whitespace preserved");
        bool rejected=false;try {PpmImage.Decode(Encoding.ASCII.GetBytes("P6\n1 1\n255\nXX"),out width,out height);}catch(FormatException){rejected=true;}
        Assert(rejected,"truncated PPM rejected");
        var optics=new OpticalSettings {useMeasuredEyes=true,measuredLeft=new Point3(-31,2,580),measuredRight=new Point3(32,3,620)};
        optics.Validate();Assert(optics.Eye(false).z==580 && optics.Eye(true).y==3,"independent eye XYZ");
        optics.measuredRight.z=-1;rejected=false;try {optics.Validate();}catch(ArgumentException){rejected=true;}
        Assert(rejected,"eye behind LCD rejected");
    }
}
