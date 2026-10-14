using System;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace ParallaxSimulation
{
    // Windows player: no System.IO.Ports package or extra managed DLL is required.
    public sealed class ArduinoLink : IDisposable
    {
        IntPtr handle=new IntPtr(-1);
        public bool IsOpen { get { return handle!=new IntPtr(-1); } }
        [StructLayout(LayoutKind.Sequential)] struct Dcb {
            public uint length,baud,flags; public ushort reserved,xonLimit,xoffLimit;
            public byte byteSize,parity,stopBits; public sbyte xon,xoff,error,eof,eventChar;public ushort reserved1;
        }
        [StructLayout(LayoutKind.Sequential)] struct Timeouts { public uint interval,multiplier,constant,writeMultiplier,writeConstant; }
        [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr CreateFileW(string name,uint access,uint share,IntPtr security,uint creation,uint flags,IntPtr template);
        [DllImport("kernel32.dll",SetLastError=true)] static extern bool GetCommState(IntPtr h,ref Dcb d);
        [DllImport("kernel32.dll",SetLastError=true)] static extern bool SetCommState(IntPtr h,ref Dcb d);
        [DllImport("kernel32.dll",SetLastError=true)] static extern bool SetCommTimeouts(IntPtr h,ref Timeouts t);
        [DllImport("kernel32.dll",SetLastError=true)] static extern bool WriteFile(IntPtr h,byte[] data,uint count,out uint written,IntPtr overlapped);
        [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
        static void Check(bool ok) { if(!ok) throw new Win32Exception(Marshal.GetLastWin32Error()); }
        public void Open(string port)
        {
            Dispose();
            port=port.Trim().ToUpperInvariant();
            if(!System.Text.RegularExpressions.Regex.IsMatch(port,@"^COM[1-9][0-9]*$")) throw new ArgumentException("Use COM3, COM4, etc.");
            handle=CreateFileW(@"\\.\"+port,0xC0000000,0,IntPtr.Zero,3,0,IntPtr.Zero);
            Check(IsOpen);
            try {
                var d=new Dcb {length=(uint)Marshal.SizeOf(typeof(Dcb))};Check(GetCommState(handle,ref d));
                d.baud=115200;d.flags=1;d.byteSize=8;d.parity=0;d.stopBits=0;
                Check(SetCommState(handle,ref d));
                var t=new Timeouts {interval=uint.MaxValue,writeConstant=20};Check(SetCommTimeouts(handle,ref t));
            } catch { Dispose();throw; }
        }
        public void Send(byte command)
        {
            if(!IsOpen) throw new InvalidOperationException("Arduino is not connected.");
            uint written;Check(WriteFile(handle,new[]{command},1,out written,IntPtr.Zero));
            if(written!=1) throw new InvalidOperationException("Arduino write timed out.");
        }
        public void Dispose() { if(IsOpen) {CloseHandle(handle);handle=new IntPtr(-1);} }
    }
}
