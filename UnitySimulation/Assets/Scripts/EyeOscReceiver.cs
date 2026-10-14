using System;
using System.Net;
using System.Net.Sockets;
using System.Diagnostics;
using System.Text;

namespace ParallaxSimulation
{
    public sealed class EyeOscReceiver : IDisposable
    {
        UdpClient client;
        readonly Stopwatch clock=Stopwatch.StartNew();
        double leftTime=-10,rightTime=-10;
        bool leftDirty,rightDirty;
        double[] pendingLeft,pendingRight;
        public double[] Left { get; private set; }
        public double[] Right { get; private set; }
        public double PairTime { get; private set; } = -10;
        public bool Fresh { get { return clock.Elapsed.TotalSeconds-PairTime<1; } }
        public void Open(int port) { Dispose();client=new UdpClient(new IPEndPoint(IPAddress.Loopback,port));client.Client.Blocking=false; }
        public void Poll()
        {
            if(client==null) return;
            for(int n=0;n<128 && client.Available>0;n++) {
                IPEndPoint remote=null;byte[] data=client.Receive(ref remote);
                var samples=new System.Collections.Generic.List<Tuple<bool,double[]>>();
                if(!Parse(data,0,data.Length,0,(right,value)=>samples.Add(Tuple.Create(right,value)))) continue;
                double now=clock.Elapsed.TotalSeconds;
                foreach(var sample in samples) {
                    if(sample.Item1) {pendingRight=sample.Item2;rightTime=now;rightDirty=true;}
                    else {pendingLeft=sample.Item2;leftTime=now;leftDirty=true;}
                }
                if(leftDirty && rightDirty && Math.Abs(leftTime-rightTime)<=.1) {
                    Left=pendingLeft;Right=pendingRight;PairTime=now;leftDirty=rightDirty=false;
                }
            }
        }
        static string ReadString(byte[] data,ref int at,int end)
        {
            int start=at;while(at<end && data[at]!=0) at++;
            if(at==end) throw new FormatException();
            string s=Encoding.ASCII.GetString(data,start,at-start);at++;
            while((at&3)!=0) {if(at>=end || data[at++]!=0) throw new FormatException();}
            return s;
        }
        static int Int32BE(byte[] d,int at) {return (d[at]<<24)|(d[at+1]<<16)|(d[at+2]<<8)|d[at+3];}
        public static bool Parse(byte[] data,int start,int length,int depth,Action<bool,double[]> sample)
        {
            if(depth>8 || start<0 || length<4 || start>data.Length-length || (start&3)!=0) return false;
            int end=start+length,at=start;
            try {
                string address=ReadString(data,ref at,end);
                if(address=="#bundle") {
                    if(end-at<8) return false;at+=8;
                    while(at<end) {if(end-at<4)return false;int count=Int32BE(data,at);at+=4;
                        if(count<=0 || count>end-at || !Parse(data,at,count,depth+1,sample))return false;at+=count;}
                    return true;
                }
                if(address!="/eye/left" && address!="/eye/right") return false;
                if(ReadString(data,ref at,end)!=",fff" || end-at!=12) return false;
                var values=new double[3];
                for(int i=0;i<3;i++) {byte[] b=BitConverter.GetBytes(Int32BE(data,at));at+=4;values[i]=BitConverter.ToSingle(b,0);
                    if(double.IsNaN(values[i]) || double.IsInfinity(values[i]) || Math.Abs(values[i])>10)return false;}
                if(values[2]<.1)return false;
                sample(address=="/eye/right",values);return true;
            } catch(FormatException) {return false;}
        }
        public void Dispose() {if(client!=null)client.Close();client=null;Left=Right=null;PairTime=leftTime=rightTime=-10;leftDirty=rightDirty=false;}
    }
}
